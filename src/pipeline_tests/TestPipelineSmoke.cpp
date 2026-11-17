// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Dpi.h>
#include <ForgedImages.h>
#include <ImageLoader.h>
#include <PageGenerator.h>
#include <TiffWriter.h>
#include <TruthMetrics.h>
#include <filters/output/DewarpingOptions.h>
#include <filters/output/OutputFileParams.h>
#include <filters/output/OutputParams.h>
#include <filters/output/Params.h>
#include <filters/page_split/LayoutType.h>

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRgb>
#include <QString>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>

#include "HeadlessPipeline.h"

namespace {

// Wide limits: these are smoke tests, the scenario suites bound the results against the truth.
test_support::SanityExpectation pageExpectation(const bool portrait) {
  test_support::SanityExpectation expectation;
  expectation.minSize = QSize(400, 400);
  expectation.maxSize = QSize(5000, 6000);
  expectation.minInk = 0.005;
  expectation.maxInk = 0.5;
  expectation.portrait = portrait;
  return expectation;
}

test_support::GeneratedPage generate(const test_support::PageKind kind, const double skewDeg) {
  test_support::PageSpec spec;
  spec.kind = kind;
  spec.dpi = 300;
  spec.seed = 1;
  spec.skewDeg = skewDeg;
  const test_support::GeneratedPage page = test_support::generatePage(spec);
  BOOST_REQUIRE_MESSAGE(!page.image.isNull(), "the generator returned a null page");
  return page;
}

long colourfulPixels(const QImage& image) {
  const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
  long count = 0;
  for (int y = 0; y < rgb.height(); ++y) {
    const QRgb* const line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    for (int x = 0; x < rgb.width(); ++x) {
      const int hi = std::max(qRed(line[x]), std::max(qGreen(line[x]), qBlue(line[x])));
      const int lo = std::min(qRed(line[x]), std::min(qGreen(line[x]), qBlue(line[x])));
      count += (hi - lo > 30) ? 1 : 0;
    }
  }
  return count;
}

// Replaces the file by \p bytes and pads it with zero bytes up to its old size. The output stage's cache check
// compares the stored byte size only, and TIFF readers ignore bytes after the last directory, so the replaced file
// reaches the loader instead of being rejected by the cache check.
void replaceKeepingSize(const QString& path, const QByteArray& bytes) {
  const qint64 oldSize = QFileInfo(path).size();
  BOOST_REQUIRE_MESSAGE(QFileInfo::exists(path), "the output file does not exist");
  BOOST_REQUIRE_LT(qint64(bytes.size()), oldSize);

  QFile file(path);
  BOOST_REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  BOOST_REQUIRE_EQUAL(file.write(bytes), qint64(bytes.size()));
  BOOST_REQUIRE_EQUAL(file.write(QByteArray(int(oldSize) - bytes.size(), '\0')), oldSize - bytes.size());
  file.close();
  BOOST_REQUIRE_EQUAL(QFileInfo(path).size(), oldSize);
}

// The replaced file must pass the cache check, so that the second run gets as far as reading it.
void requireCacheAccepts(StageSettings& settings, const PageId& pageId, const QString& path) {
  const std::unique_ptr<output::OutputParams> stored = settings.output->getOutputParams(pageId);
  BOOST_REQUIRE(stored != nullptr);
  BOOST_REQUIRE(stored->outputFileParams().matches(output::OutputFileParams(QFileInfo(path))));
}

}  // namespace

BOOST_AUTO_TEST_SUITE(PipelineSmokeTestSuite)

BOOST_AUTO_TEST_CASE(test_spread_yields_two_pages) {
  const test_support::GeneratedPage page = generate(test_support::PageKind::SPREAD, 1.0);

  std::vector<int> hookedSubPages;
  PipelineOptions options;
  options.outputDpi = 600;
  options.configure = [&hookedSubPages](StageSettings&, const PageId& pageId) {
    hookedSubPages.push_back(static_cast<int>(pageId.subPage()));
  };
  const PipelineResult result = runPipeline(page.image, options);

  BOOST_CHECK_EQUAL(result.layoutType, 2);  // TWO_PAGES
  BOOST_REQUIRE_EQUAL(hookedSubPages.size(), 2u);
  BOOST_CHECK_EQUAL(hookedSubPages[0], static_cast<int>(PageId::LEFT_PAGE));
  BOOST_CHECK_EQUAL(hookedSubPages[1], static_cast<int>(PageId::RIGHT_PAGE));
  BOOST_CHECK_EQUAL(result.cutters.size(), 1u);
  BOOST_REQUIRE_EQUAL(result.pages.size(), 2u);
  BOOST_CHECK_EQUAL(static_cast<int>(result.pages[0].pageId.subPage()), static_cast<int>(PageId::LEFT_PAGE));
  BOOST_CHECK_EQUAL(static_cast<int>(result.pages[1].pageId.subPage()), static_cast<int>(PageId::RIGHT_PAGE));
  for (const SubPageResult& sub : result.pages) {
    const test_support::SanityCheck sanity = test_support::outputSanity(sub.output, pageExpectation(true));
    BOOST_CHECK_MESSAGE(
        sanity.ok, "sub-page " << sub.pageId.subPageAsString().toStdString() << ": " << sanity.reason.toStdString());
    BOOST_CHECK_EQUAL(sub.outputDpiX, 600);
  }
  const QByteArray left = test_support::fingerprint(result.pages[0].output);
  const QByteArray right = test_support::fingerprint(result.pages[1].output);
  BOOST_CHECK_MESSAGE(left != right, "the two sub-pages produced identical output");
}

BOOST_AUTO_TEST_CASE(test_curved_auto_dewarp_runs) {
  const test_support::GeneratedPage page = generate(test_support::PageKind::CURVED, 0.0);

  PipelineOptions options;
  options.layout = page_split::SINGLE_PAGE_UNCUT;
  options.dewarping = output::AUTO;
  options.outputDpi = 600;
  const PipelineResult result = runPipeline(page.image, options);

  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  const SubPageResult& sub = result.pages.front();
  const test_support::SanityCheck sanity = test_support::outputSanity(sub.output, pageExpectation(false));
  BOOST_CHECK_MESSAGE(sanity.ok, sanity.reason.toStdString());

  const output::Params params = result.settings.output->getParams(sub.pageId);
  BOOST_CHECK_EQUAL(static_cast<int>(params.dewarpingOptions().dewarpingMode()), static_cast<int>(output::AUTO));
  BOOST_CHECK_MESSAGE(params.distortionModel().isValid(), "AUTO dewarping left no valid distortion model");
}

BOOST_AUTO_TEST_CASE(test_mixed_mode_keeps_colour) {
  const test_support::GeneratedPage page = generate(test_support::PageKind::MIXED, 0.0);

  PipelineOptions options;
  options.layout = page_split::SINGLE_PAGE_UNCUT;
  options.colorMode = output::MIXED;
  options.outputDpi = 600;
  const PipelineResult result = runPipeline(page.image, options);

  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  const SubPageResult& sub = result.pages.front();
  const test_support::SanityCheck sanity = test_support::outputSanity(sub.output, pageExpectation(false));
  BOOST_CHECK_MESSAGE(sanity.ok, sanity.reason.toStdString());
  const long colourful = colourfulPixels(sub.output);
  BOOST_TEST_MESSAGE("MIXED output has " << colourful << " pixels with chroma > 30");
  BOOST_CHECK_MESSAGE(colourful > 0, "MIXED output is bi-level: the picture lost its colour");
}

BOOST_AUTO_TEST_CASE(test_configure_hook_runs) {
  const test_support::GeneratedPage page = generate(test_support::PageKind::FLAT, 1.5);

  int calls = 0;
  PipelineOptions options;
  options.outputDpi = 600;
  options.configure = [&calls](StageSettings& settings, const PageId& pageId) {
    ++calls;
    settings.output->setDpi(pageId, Dpi(400, 400));
  };
  const PipelineResult result = runPipeline(page.image, options);

  BOOST_CHECK_EQUAL(calls, 1);
  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  BOOST_CHECK_EQUAL(result.pages.front().outputDpiX, 400);
  BOOST_CHECK_EQUAL(result.pages.front().outputDpiY, 400);
}

BOOST_AUTO_TEST_CASE(test_forged_cached_output_is_regenerated) {
  // The second run reads the forged file through the own-file loader, the own-file ceiling refuses its header, and
  // the null image makes the output stage write the page again. runPipeline reads the output back with the same
  // loader and would throw on the forged file, so a normal return with the first output means the page was
  // regenerated. This case carries no footprint bound on purpose: the loader-level cases of the size-limit suite
  // measure the allocation, and a run without the ceiling would decode a 1.25 GB image here.
  const test_support::GeneratedPage page = generate(test_support::PageKind::FLAT, 1.5);

  int calls = 0;
  QImage firstOutput;
  PipelineOptions options;
  options.outputDpi = 300;
  options.beforeOutputRerun
      = [&calls, &firstOutput](StageSettings& settings, const PageId& pageId, const QString& path) {
          ++calls;
          firstOutput = ImageLoader::loadOwnFile(path, 0);
          BOOST_REQUIRE(!firstOutput.isNull());
          replaceKeepingSize(path, test_support::forgedTiff({{100000, 100000, 1, 1, false}}));
          requireCacheAccepts(settings, pageId, path);
        };
  const PipelineResult result = runPipeline(page.image, options);

  BOOST_CHECK_EQUAL(calls, 1);
  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  BOOST_CHECK(result.pages.front().output == firstOutput);
}

BOOST_AUTO_TEST_CASE(test_valid_cached_output_is_read_back) {
  // The second run takes the cached file when the loader accepts it, so the regeneration in the forged case comes
  // from the refused load and not from a mismatch in the cache check.
  const test_support::GeneratedPage page = generate(test_support::PageKind::FLAT, 1.5);

  QImage tiny(16, 16, QImage::Format_Mono);
  tiny.fill(1);
  QByteArray tinyTiff;
  {
    QBuffer buffer(&tinyTiff);
    BOOST_REQUIRE(buffer.open(QIODevice::WriteOnly));
    BOOST_REQUIRE(TiffWriter::writeImage(buffer, tiny));
  }

  int calls = 0;
  PipelineOptions options;
  options.outputDpi = 300;
  options.beforeOutputRerun = [&calls, &tinyTiff](StageSettings& settings, const PageId& pageId, const QString& path) {
    ++calls;
    replaceKeepingSize(path, tinyTiff);
    requireCacheAccepts(settings, pageId, path);
  };
  const PipelineResult result = runPipeline(page.image, options);

  BOOST_CHECK_EQUAL(calls, 1);
  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  BOOST_CHECK(result.pages.front().outputSize == QSize(16, 16));
}

BOOST_AUTO_TEST_SUITE_END()
