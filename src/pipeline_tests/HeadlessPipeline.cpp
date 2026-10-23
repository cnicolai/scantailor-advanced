// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "HeadlessPipeline.h"

#include <DefaultParams.h>
#include <DefaultParamsProfileManager.h>
#include <DefaultParamsProvider.h>
#include <Dpi.h>
#include <Dpm.h>
#include <FileNameDisambiguator.h>
#include <FilterData.h>
#include <ImageId.h>
#include <ImageInfo.h>
#include <ImageLoader.h>
#include <ImageMetadata.h>
#include <Margins.h>
#include <NullTaskStatus.h>
#include <OutputFileNameGenerator.h>
#include <PageInfo.h>
#include <ProjectPages.h>
#include <ThumbnailPixmapCache.h>
#include <filters/deskew/Params.h>
#include <filters/deskew/Task.h>
#include <filters/deskew/Utils.h>
#include <filters/fix_orientation/Task.h>
#include <filters/fix_orientation/Utils.h>
#include <filters/output/ImageViewTab.h>
#include <filters/output/Params.h>
#include <filters/output/Task.h>
#include <filters/output/Utils.h>
#include <filters/page_layout/Params.h>
#include <filters/page_layout/Task.h>
#include <filters/page_layout/Utils.h>
#include <filters/page_split/PageLayout.h>
#include <filters/page_split/Params.h>
#include <filters/page_split/Task.h>
#include <filters/page_split/Utils.h>
#include <filters/select_content/Params.h>
#include <filters/select_content/Task.h>
#include <filters/select_content/Utils.h>

#include <QDir>
#include <QString>
#include <QTemporaryDir>
#include <stdexcept>

namespace {

// How far down the chain a call goes.
enum class ChainEnd { PAGE_SPLIT, DESKEW, OUTPUT };

struct RunContext {
  QImage image;
  ImageId imageId;
  ImageMetadata metadata;
  StageSettings settings;
  std::shared_ptr<ProjectPages> pages;
  std::shared_ptr<ThumbnailPixmapCache> thumbnailCache;
  OutputFileNameGenerator outFileNameGen;
};

[[noreturn]] void fail(const QString& message) {
  throw std::runtime_error(message.toStdString());
}

StageSettings createSettings() {
  StageSettings settings;
  settings.imageSettings = std::make_shared<ImageSettings>();
  settings.fixOrientation = std::make_shared<fix_orientation::Settings>();
  settings.pageSplit = std::make_shared<page_split::Settings>();
  settings.deskew = std::make_shared<deskew::Settings>();
  settings.selectContent = std::make_shared<select_content::Settings>();
  settings.pageLayout = std::make_shared<page_layout::Settings>();
  settings.output = std::make_shared<output::Settings>();
  return settings;
}

// Builds the Tasks last stage to first, like MainWindow::createCompositeTask, with no Filter objects
// (filter = nullptr), and pushes the image through them.
void runChain(const RunContext& ctx, const PageInfo& page, const ChainEnd end) {
  const PageId& pageId = page.id();
  const StageSettings& s = ctx.settings;
  const bool batch = true;
  const bool debug = false;

  std::shared_ptr<output::Task> outputTask;
  std::shared_ptr<page_layout::Task> pageLayoutTask;
  std::shared_ptr<select_content::Task> selectContentTask;
  std::shared_ptr<deskew::Task> deskewTask;

  if (end == ChainEnd::OUTPUT) {
    outputTask = std::make_shared<output::Task>(std::shared_ptr<output::Filter>(), s.output, ctx.thumbnailCache, pageId,
                                                ctx.outFileNameGen, output::TAB_OUTPUT, batch, debug);
    pageLayoutTask = std::make_shared<page_layout::Task>(std::shared_ptr<page_layout::Filter>(), outputTask,
                                                         s.pageLayout, pageId, batch, debug);
    selectContentTask = std::make_shared<select_content::Task>(std::shared_ptr<select_content::Filter>(),
                                                               pageLayoutTask, s.selectContent, pageId, batch, debug);
  }
  if (end == ChainEnd::OUTPUT || end == ChainEnd::DESKEW) {
    deskewTask = std::make_shared<deskew::Task>(std::shared_ptr<deskew::Filter>(), s.deskew, s.imageSettings,
                                                selectContentTask, pageId, batch, debug);
  }
  auto pageSplitTask = std::make_shared<page_split::Task>(std::shared_ptr<page_split::Filter>(), s.pageSplit,
                                                          ctx.pages, deskewTask, page, batch, debug);
  auto fixOrientationTask = std::make_shared<fix_orientation::Task>(
      pageId, std::shared_ptr<fix_orientation::Filter>(), s.fixOrientation, s.imageSettings, pageSplitTask, batch);

  fixOrientationTask->process(NullTaskStatus(), FilterData(ctx.image));
}

// Loads every stage's defaults for the page, as each Filter::loadDefaultSettings does.  Without them
// select_content computes nothing.
void loadStageDefaults(const RunContext& ctx, const PageId& pageId) {
  const Dpi dpi(Dpm(ctx.image));
  const StageSettings& s = ctx.settings;
  s.deskew->setPageParams(pageId, deskew::Utils::buildDefaultParams());
  s.selectContent->setPageParams(pageId, select_content::Utils::buildDefaultParams(dpi));
  s.pageLayout->setPageParams(pageId, page_layout::Utils::buildDefaultParams(dpi));
  s.output->setParams(pageId, output::Utils::buildDefaultParams());
}

void applyOptions(const RunContext& ctx, const PageId& pageId, const PipelineOptions& options) {
  output::Settings& out = *ctx.settings.output;
  output::ColorParams colorParams = out.getParams(pageId).colorParams();
  colorParams.setColorMode(options.colorMode);
  out.setColorParams(pageId, colorParams);
  out.setDespeckleLevel(pageId, options.despeckleLevel);
  out.setDewarpingOptions(pageId, output::DewarpingOptions(options.dewarping));
  out.setDpi(pageId, Dpi(options.outputDpi, options.outputDpi));
}

SubPageResult readResults(const RunContext& ctx, const PageId& pageId, const ChainEnd end) {
  const StageSettings& s = ctx.settings;
  SubPageResult result;
  result.pageId = pageId;

  const std::unique_ptr<deskew::Params> deskewParams = s.deskew->getPageParams(pageId);
  if (!deskewParams) {
    fail("deskew left no parameters");
  }
  result.deskewAngleDeg = deskewParams->deskewAngle();
  if (end != ChainEnd::OUTPUT) {
    return result;
  }

  const std::unique_ptr<select_content::Params> contentParams = s.selectContent->getPageParams(pageId);
  if (!contentParams) {
    fail("select_content left no parameters");
  }
  result.contentRect = contentParams->contentRect().toRect();
  result.pageRect = contentParams->pageRect();

  const Margins margins = s.pageLayout->getHardMarginsMM(pageId);
  result.hardMarginLeftMM = margins.left();
  result.hardMarginTopMM = margins.top();
  result.hardMarginRightMM = margins.right();
  result.hardMarginBottomMM = margins.bottom();

  result.output = ImageLoader::load(ctx.outFileNameGen.filePathFor(pageId), 0);
  if (result.output.isNull()) {
    fail("the output stage wrote no readable file for " + ctx.outFileNameGen.filePathFor(pageId));
  }
  result.outputSize = result.output.size();
  const Dpi outputDpi{Dpm(result.output)};
  result.outputDpiX = outputDpi.horizontal();
  result.outputDpiY = outputDpi.vertical();
  return result;
}

}  // namespace

PipelineResult runPipeline(const QImage& input, const PipelineOptions& options) {
  // A test that installed another profile must not change later scenarios.
  DefaultParamsProvider::getInstance().setParams(DefaultParamsProfileManager().createDefaultProfile(),
                                                 QStringLiteral("Default"));

  // Resolves under TMPDIR, which the test environment points into the build tree.  Declared first so that
  // it is removed last.
  QTemporaryDir runDir;
  if (!runDir.isValid()) {
    fail("cannot create a temporary project directory");
  }
  const QString outDir = runDir.filePath("out");
  if (!QDir().mkpath(outDir + "/cache")) {
    fail("cannot create " + outDir + "/cache");
  }

  const QString inputPath = runDir.filePath("in.png");
  if (!input.save(inputPath, "PNG")) {
    fail("cannot write " + inputPath);
  }

  RunContext ctx;
  ctx.imageId = ImageId(inputPath);
  // Read the input back the way the application does, which also takes the dpi from the file metadata.
  ctx.image = ImageLoader::load(ctx.imageId);
  if (ctx.image.isNull() || ctx.image.size() != input.size()) {
    fail("cannot read back " + inputPath);
  }
  ctx.metadata = ImageMetadata(ctx.image.size(), Dpi(Dpm(ctx.image)));
  ctx.settings = createSettings();
  const std::vector<ImageInfo> images{ImageInfo(ctx.imageId, ctx.metadata, 1, false, false)};
  ctx.pages = std::make_shared<ProjectPages>(images, Qt::LeftToRight);
  ctx.thumbnailCache = std::make_shared<ThumbnailPixmapCache>(runDir.filePath("thumbs"), QSize(200, 200), 40, 5);
  ctx.outFileNameGen = OutputFileNameGenerator(std::make_shared<FileNameDisambiguator>(), outDir, Qt::LeftToRight);

  // Image-wide defaults (fix_orientation rotation, page_split layout type), then the requested layout.
  ctx.settings.fixOrientation->applyRotation(ctx.imageId, fix_orientation::Utils::getDefaultOrthogonalRotation());
  page_split::Settings::UpdateAction splitUpdate = page_split::Utils::buildDefaultUpdateAction();
  if (options.layout != page_split::AUTO_LAYOUT_TYPE) {
    splitUpdate.setLayoutType(options.layout);
  }
  ctx.settings.pageSplit->updatePage(ctx.imageId, splitUpdate);

  // The layout decides how many sub-pages there are, so run up to page_split first.
  const PageId firstPageId(ctx.imageId, PageId::SINGLE_PAGE);
  runChain(ctx, PageInfo(firstPageId, ctx.metadata, 1, false, false), ChainEnd::PAGE_SPLIT);

  PipelineResult result;
  result.settings = ctx.settings;

  const page_split::Settings::Record record = ctx.settings.pageSplit->getPageRecord(ctx.imageId);
  if (record.params() == nullptr) {
    fail("page_split left no parameters");
  }
  const page_split::PageLayout& layout = record.params()->pageLayout();
  result.layoutType = static_cast<int>(layout.type());
  for (int i = 0; i < layout.numCutters(); ++i) {
    result.cutters.push_back(layout.cutterLine(i));
  }

  std::vector<PageId> pageIds;
  int numSubPages = 1;
  if (layout.type() == page_split::PageLayout::TWO_PAGES) {
    pageIds.push_back(PageId(ctx.imageId, PageId::LEFT_PAGE));
    pageIds.push_back(PageId(ctx.imageId, PageId::RIGHT_PAGE));
    numSubPages = 2;
  } else {
    pageIds.push_back(firstPageId);
  }

  const ChainEnd end = (options.lastStage == LastStage::DESKEW) ? ChainEnd::DESKEW : ChainEnd::OUTPUT;
  for (const PageId& pageId : pageIds) {
    loadStageDefaults(ctx, pageId);
    applyOptions(ctx, pageId, options);
    if (options.configure) {
      options.configure(ctx.settings, pageId);
    }
    runChain(ctx, PageInfo(pageId, ctx.metadata, numSubPages, false, false), end);
    result.pages.push_back(readResults(ctx, pageId, end));
  }
  return result;
}
