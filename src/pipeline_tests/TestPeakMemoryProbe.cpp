// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

// Opt-in tools for the memory run of Phase 6 (D-08). Neither case runs under ctest: both are disabled by default
// and only run when named, see the comment above each case.

#include <BackgroundTask.h>
#include <DefaultParams.h>
#include <DefaultParamsProfileManager.h>
#include <DefaultParamsProvider.h>
#include <Dpi.h>
#include <FileNameDisambiguator.h>
#include <FilterResult.h>
#include <ImageId.h>
#include <ImageInfo.h>
#include <ImageMetadata.h>
#include <LoadFileTask.h>
#include <MemoryBudget.h>
#include <OutputFileNameGenerator.h>
#include <PageGenerator.h>
#include <PageInfo.h>
#include <PageSequence.h>
#include <ProcessMemory.h>
#include <ProjectPages.h>
#include <ThumbnailPixmapCache.h>
#include <WorkerThreadPool.h>
#include <filters/deskew/Params.h>
#include <filters/deskew/Task.h>
#include <filters/deskew/Utils.h>
#include <filters/fix_orientation/Task.h>
#include <filters/fix_orientation/Utils.h>
#include <filters/output/DewarpingOptions.h>
#include <filters/output/ImageViewTab.h>
#include <filters/output/Params.h>
#include <filters/output/Task.h>
#include <filters/output/Utils.h>
#include <filters/page_layout/Params.h>
#include <filters/page_layout/Task.h>
#include <filters/page_layout/Utils.h>
#include <filters/page_split/LayoutType.h>
#include <filters/page_split/Task.h>
#include <filters/page_split/Utils.h>
#include <filters/select_content/Params.h>
#include <filters/select_content/Task.h>
#include <filters/select_content/Utils.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <QByteArray>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "HeadlessPipeline.h"

namespace {

constexpr qint64 kMiB = qint64(1) << 20;
constexpr int kSampleIntervalMs = 2000;
constexpr int kDeadlineMs = 3600 * 1000;

int envInt(const char* name, const int fallback) {
  bool ok = false;
  const int value = qgetenv(name).toInt(&ok);
  return (ok && value > 0) ? value : fallback;
}

// The MIXED page of the probes at the given dpi, enlarged by an integer factor the way the research harness does it:
// the pixels are repeated and the dots per meter multiplied, so the page keeps its look at a much larger size.
QImage mixedPage(const int dpi, const int scale) {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::MIXED;
  spec.dpi = dpi;
  spec.seed = 1;
  QImage image = test_support::generatePage(spec).image;
  BOOST_REQUIRE_MESSAGE(!image.isNull(), "the generator refuses dpi " << dpi);
  if (scale > 1) {
    const int dpmX = image.dotsPerMeterX();
    const int dpmY = image.dotsPerMeterY();
    image = image.scaled(image.width() * scale, image.height() * scale, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    image.setDotsPerMeterX(dpmX * scale);
    image.setDotsPerMeterY(dpmY * scale);
  }
  return image;
}

// Swap in use in bytes (vm.swapusage), -1 if the system does not tell.
qint64 swapUsedBytes() {
  struct xsw_usage usage;
  size_t size = sizeof(usage);
  if (sysctlbyname("vm.swapusage", &usage, &size, nullptr, 0) != 0) {
    return -1;
  }
  return qint64(usage.xsu_used);
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

// What Filter::loadDefaultSettings does for a page of the batch, then the options of the peak probe: MIXED colour
// mode, no dewarping, output at the source dpi. Runs on the main thread, as startBatchProcessing does.
void loadPageDefaults(const StageSettings& s, const PageId& pageId, const int dpiValue) {
  const Dpi dpi(dpiValue, dpiValue);
  s.fixOrientation->applyRotation(pageId.imageId(), fix_orientation::Utils::getDefaultOrthogonalRotation());
  page_split::Settings::UpdateAction splitUpdate = page_split::Utils::buildDefaultUpdateAction();
  splitUpdate.setLayoutType(page_split::SINGLE_PAGE_UNCUT);
  s.pageSplit->updatePage(pageId.imageId(), splitUpdate);

  s.deskew->setPageParams(pageId, deskew::Utils::buildDefaultParams());
  s.selectContent->setPageParams(pageId, select_content::Utils::buildDefaultParams(dpi));
  s.pageLayout->setPageParams(pageId, page_layout::Utils::buildDefaultParams(dpi));
  s.output->setParams(pageId, output::Utils::buildDefaultParams());

  output::ColorParams colorParams = s.output->getParams(pageId).colorParams();
  colorParams.setColorMode(output::MIXED);
  s.output->setColorParams(pageId, colorParams);
  s.output->setDespeckleLevel(pageId, 1.0);
  s.output->setDewarpingOptions(pageId, output::DewarpingOptions(output::OFF));
  s.output->setDpi(pageId, dpi);
}

// MainWindow::createCompositeTask for the batch: the six stage tasks last to first (with no Filter objects, as in
// the headless runner), wrapped in the LoadFileTask that loads the file and starts the chain.
BackgroundTaskPtr createCompositeTask(const PageInfo& page,
                                      const StageSettings& s,
                                      const std::shared_ptr<ThumbnailPixmapCache>& thumbnailCache,
                                      const std::shared_ptr<ProjectPages>& pages,
                                      const OutputFileNameGenerator& outFileNameGen) {
  const PageId& pageId = page.id();
  const bool batch = true;
  const bool debug = false;

  auto outputTask = std::make_shared<output::Task>(std::shared_ptr<output::Filter>(), s.output, thumbnailCache, pageId,
                                                   outFileNameGen, output::TAB_OUTPUT, batch, debug);
  auto pageLayoutTask = std::make_shared<page_layout::Task>(std::shared_ptr<page_layout::Filter>(), outputTask,
                                                            s.pageLayout, pageId, batch, debug);
  auto selectContentTask = std::make_shared<select_content::Task>(
      std::shared_ptr<select_content::Filter>(), pageLayoutTask, s.selectContent, pageId, batch, debug);
  auto deskewTask = std::make_shared<deskew::Task>(std::shared_ptr<deskew::Filter>(), s.deskew, s.imageSettings,
                                                   selectContentTask, pageId, batch, debug);
  auto pageSplitTask = std::make_shared<page_split::Task>(std::shared_ptr<page_split::Filter>(), s.pageSplit, pages,
                                                          deskewTask, page, batch, debug);
  auto fixOrientationTask = std::make_shared<fix_orientation::Task>(
      pageId, std::shared_ptr<fix_orientation::Filter>(), s.fixOrientation, s.imageSettings, pageSplitTask, batch);
  return std::make_shared<LoadFileTask>(BackgroundTask::BATCH, page, thumbnailCache, pages, fixOrientationTask);
}

// What the counting wrapper tells the main thread.
struct BatchState {
  std::atomic<int> running{0};
  std::atomic<int> maxRunning{0};
  std::atomic<int> failed{0};
  std::mutex mutex;
  std::vector<std::string> failures;

  void fail(const QString& message) {
    ++failed;
    const std::lock_guard<std::mutex> lock(mutex);
    failures.push_back(message.toStdString());
  }
};

class NoopResult : public FilterResult {
 public:
  void updateUI(FilterUiInterface*) override {}

  std::shared_ptr<AbstractFilter> filter() override { return nullptr; }
};

// Runs the page's composite task, counts the pages that run at once, checks that the output file exists and removes
// it, so that only the running pages' outputs are on disk. Always returns a result, so the pool posts one per page.
class CountingTask : public BackgroundTask {
 public:
  CountingTask(BackgroundTaskPtr inner, QString outputPath, BatchState& state)
      : BackgroundTask(BATCH), m_inner(std::move(inner)), m_outputPath(std::move(outputPath)), m_state(state) {}

  FilterResultPtr operator()() override {
    const int now = ++m_state.running;
    int seen = m_state.maxRunning.load();
    while (now > seen && !m_state.maxRunning.compare_exchange_weak(seen, now)) {
    }

    try {
      (*m_inner)();
      // A file that failed to load gives an error result and no exception; the output file tells.
      if (!QFile::exists(m_outputPath)) {
        m_state.fail("no output file " + m_outputPath);
      }
    } catch (const std::exception& e) {
      // Includes std::bad_alloc.
      m_state.fail(QString("%1: %2").arg(m_outputPath, QString::fromUtf8(e.what())));
    }
    QFile::remove(m_outputPath);

    --m_state.running;
    return std::make_shared<NoopResult>();
  }

 private:
  BackgroundTaskPtr m_inner;
  QString m_outputPath;
  BatchState& m_state;
};

}  // namespace

BOOST_AUTO_TEST_SUITE(PeakMemoryProbeTestSuite)

// A tool for the D-08 run, never run by ctest. One MIXED page through all six stages, then one PROBE line. Run it
// with --run_test=PeakMemoryProbeTestSuite/test_print_peak_memory; ST_PROBE_DPI (default 300) and ST_PROBE_SCALE
// (default 1) choose the page.
BOOST_AUTO_TEST_CASE(test_print_peak_memory, *boost::unit_test::disabled()) {
  const int dpi = envInt("ST_PROBE_DPI", 300);
  const int scale = envInt("ST_PROBE_SCALE", 1);
  const int effectiveDpi = dpi * scale;

  const qint64 peakBefore = test_support::peakPhysFootprintBytes();
  BOOST_REQUIRE_GE(peakBefore, 0);

  QImage page = mixedPage(dpi, scale);
  const QSize size = page.size();

  PipelineOptions options;
  options.layout = page_split::SINGLE_PAGE_UNCUT;
  options.colorMode = output::MIXED;
  options.dewarping = output::OFF;
  options.outputDpi = effectiveDpi;
  const PipelineResult result = runPipeline(page, options);
  BOOST_CHECK_EQUAL(result.pages.size(), 1u);
  page = QImage();

  const qint64 growth = std::max<qint64>(0, test_support::peakPhysFootprintBytes() - peakBefore);
  const qint64 pixels = MemoryBudget::pagePixelEstimate(ImageMetadata(size, Dpi(effectiveDpi, effectiveDpi)));
  const qint64 estimate = MemoryBudget::kBytesPerPagePixel * pixels;

  std::printf("PROBE pixels=%lld estimate_mb=%lld peak_above_start_mb=%lld cap16=%d cap64=%d cap_here=%d\n",
              static_cast<long long>(pixels), static_cast<long long>(estimate / kMiB),
              static_cast<long long>(growth / kMiB), MemoryBudget::threadsForMemory(qint64(16) * 1024 * kMiB, pixels),
              MemoryBudget::threadsForMemory(qint64(64) * 1024 * kMiB, pixels),
              MemoryBudget::threadsForMemory(MemoryBudget::physicalMemoryBytes(), pixels));
  std::fflush(stdout);

  BOOST_CHECK_LE(growth, estimate);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(BatchMemoryProbeTestSuite)

// The D-08 run of plan 06-16 drives large synthetic pages through the application's worker pool with the
// application's memory cap; never run by ctest. Run it with
// --run_test=BatchMemoryProbeTestSuite/test_batch_through_worker_pool; ST_PROBE_DPI (default 300), ST_PROBE_SCALE
// (default 1) and ST_PROBE_PAGES (default 4) choose the batch. It prints SAMPLE lines every 2 s and one BATCH line.
BOOST_AUTO_TEST_CASE(test_batch_through_worker_pool, *boost::unit_test::disabled()) {
  const int dpi = envInt("ST_PROBE_DPI", 300);
  const int scale = envInt("ST_PROBE_SCALE", 1);
  const int numPages = envInt("ST_PROBE_PAGES", 4);
  const int effectiveDpi = dpi * scale;

  const qint64 swapBefore = swapUsedBytes();

  QTemporaryDir tmp;
  BOOST_REQUIRE(tmp.isValid());
  BOOST_REQUIRE_MESSAGE(tmp.path().startsWith(QString::fromUtf8(TEST_TMP_ROOT)), tmp.path().toStdString());
  const QString outDir = tmp.filePath("out");
  BOOST_REQUIRE(QDir().mkpath(outDir + "/cache"));

  // One input file, the other pages are hard links of it: the inputs cost the disk space of one.
  QSize pageSize;
  const auto inputPath
      = [&tmp](const int number) { return tmp.filePath(QString("page-%1.png").arg(number, 2, 10, QChar('0'))); };
  {
    const QImage page = mixedPage(dpi, scale);
    pageSize = page.size();
    BOOST_REQUIRE(page.save(inputPath(1), "PNG"));
  }
  for (int i = 2; i <= numPages; ++i) {
    BOOST_REQUIRE_MESSAGE(::link(inputPath(1).toLocal8Bit().constData(), inputPath(i).toLocal8Bit().constData()) == 0,
                          "cannot link " << inputPath(i).toStdString());
  }

  // What MainWindow holds for a project, shared by all pages.
  const ImageMetadata metadata(pageSize, Dpi(effectiveDpi, effectiveDpi));
  std::vector<ImageInfo> images;
  for (int i = 1; i <= numPages; ++i) {
    images.push_back(ImageInfo(ImageId(inputPath(i)), metadata, 1, false, false));
  }
  const auto pages = std::make_shared<ProjectPages>(images, Qt::LeftToRight);
  const auto thumbnailCache = std::make_shared<ThumbnailPixmapCache>(tmp.filePath("thumbs"), QSize(200, 200), 40, 5);
  const OutputFileNameGenerator outFileNameGen(std::make_shared<FileNameDisambiguator>(), outDir, Qt::LeftToRight);
  const StageSettings settings = createSettings();

  // Once, on the main thread: concurrent setParams calls would race.
  DefaultParamsProvider::getInstance().setParams(DefaultParamsProfileManager().createDefaultProfile(),
                                                 QStringLiteral("Default"));

  BatchState state;
  PageSequence sequence;
  std::vector<BackgroundTaskPtr> tasks;
  for (int i = 1; i <= numPages; ++i) {
    const PageId pageId(ImageId(inputPath(i)), PageId::SINGLE_PAGE);
    const PageInfo page(pageId, metadata, 1, false, false);
    loadPageDefaults(settings, pageId, effectiveDpi);
    sequence.append(page);
    tasks.push_back(
        std::make_shared<CountingTask>(createCompositeTask(page, settings, thumbnailCache, pages, outFileNameGen),
                                       outFileNameGen.filePathFor(pageId), state));
  }

  // The cap of the application, from this Mac's real physical memory.
  const int cap = MemoryBudget::batchMemoryCap(sequence);
  WorkerThreadPool pool;
  pool.setMemoryCap(cap);
  const int threads = pool.maxThreadCount();
  const int ideal = QThread::idealThreadCount();
  const int slider = WorkerThreadPool::storedThreadCount(QSettings(), ideal);
  const qint64 startFootprint = test_support::physFootprintBytes();

  QEventLoop loop;
  QElapsedTimer clock;
  size_t next = 0;
  int finished = 0;
  bool timedOut = false;
  qint64 swapMax = swapBefore;

  const auto takeForProcessing = [&]() -> BackgroundTaskPtr { return next < tasks.size() ? tasks[next++] : nullptr; };

  // The refill of MainWindow::filterResult.
  QObject::connect(&pool, &WorkerThreadPool::taskResult, &pool, [&](const BackgroundTaskPtr&, const FilterResultPtr&) {
    if (++finished == numPages) {
      loop.quit();
      return;
    }
    if (timedOut) {
      return;
    }
    do {
      const BackgroundTaskPtr task = takeForProcessing();
      if (!task) {
        break;
      }
      pool.submitTask(task);
    } while (pool.hasSpareCapacity());
  });

  QTimer sampler;
  QObject::connect(&sampler, &QTimer::timeout, &sampler, [&]() {
    const qint64 swap = swapUsedBytes();
    swapMax = std::max(swapMax, swap);
    std::printf("SAMPLE elapsed_ms=%lld running=%d footprint_mb=%lld swap_used_bytes=%lld\n",
                static_cast<long long>(clock.elapsed()), state.running.load(),
                static_cast<long long>(test_support::physFootprintBytes() / kMiB), static_cast<long long>(swap));
    std::fflush(stdout);
  });
  QTimer deadline;
  deadline.setSingleShot(true);
  QObject::connect(&deadline, &QTimer::timeout, &deadline, [&]() {
    timedOut = true;
    loop.quit();
  });

  clock.start();
  sampler.start(kSampleIntervalMs);
  deadline.start(kDeadlineMs);

  // The first submit loop of MainWindow::startBatchProcessing.
  BackgroundTaskPtr task = takeForProcessing();
  if (task) {
    do {
      pool.submitTask(task);
      if (!pool.hasSpareCapacity()) {
        break;
      }
    } while ((task = takeForProcessing()));
  }
  loop.exec();
  sampler.stop();
  deadline.stop();
  pool.shutdown();

  const qint64 peak = test_support::peakPhysFootprintBytes();
  const qint64 swapAfter = swapUsedBytes();
  swapMax = std::max(swapMax, swapAfter);
  const qint64 growth = std::max<qint64>(0, peak - startFootprint);
  const qint64 pixels = MemoryBudget::pagePixelEstimate(metadata);
  const qint64 budget = MemoryBudget::physicalMemoryBytes() / 2;

  std::printf(
      "BATCH pages=%d pixels=%lld estimate_mb=%lld budget_mb=%lld ideal=%d slider=%d cap=%d threads=%d max_running=%d "
      "done=%d failed=%d peak_above_start_mb=%lld swap_before_bytes=%lld swap_max_bytes=%lld swap_after_bytes=%lld "
      "elapsed_s=%lld\n",
      numPages, static_cast<long long>(pixels),
      static_cast<long long>(MemoryBudget::kBytesPerPagePixel * pixels / kMiB), static_cast<long long>(budget / kMiB),
      ideal, slider, cap, threads, state.maxRunning.load(), finished, state.failed.load(),
      static_cast<long long>(growth / kMiB), static_cast<long long>(swapBefore), static_cast<long long>(swapMax),
      static_cast<long long>(swapAfter), static_cast<long long>(clock.elapsed() / 1000));
  for (const std::string& failure : state.failures) {
    std::printf("FAILURE %s\n", failure.c_str());
  }
  std::fflush(stdout);

  BOOST_CHECK_MESSAGE(!timedOut, "the batch did not finish within " << kDeadlineMs / 1000 << " s");
  BOOST_CHECK_EQUAL(finished, numPages);
  BOOST_CHECK_EQUAL(state.failed.load(), 0);
  BOOST_CHECK_EQUAL(threads, MemoryBudget::effectiveThreads(ideal, slider, cap));
  BOOST_CHECK_LE(state.maxRunning.load(), threads);
  BOOST_CHECK_LE(growth, budget);
  if (swapBefore >= 0) {
    BOOST_CHECK_LE(swapMax, swapBefore);
    BOOST_CHECK_LE(swapAfter, swapBefore);
  }
}

BOOST_AUTO_TEST_SUITE_END()
