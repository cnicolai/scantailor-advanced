// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include <Dpi.h>
#include <FileNameDisambiguator.h>
#include <FilterData.h>
#include <ImageId.h>
#include <ImageInfo.h>
#include <ImageLoader.h>
#include <ImageMetadata.h>
#include <Margins.h>
#include <NullTaskStatus.h>
#include <OrthogonalRotation.h>
#include <OutputFileNameGenerator.h>
#include <PageId.h>
#include <PageInfo.h>
#include <PageRange.h>
#include <PageSelectionAccessor.h>
#include <PageSelectionProvider.h>
#include <PageSequence.h>
#include <PageView.h>
#include <ProjectPages.h>
#include <ProjectReader.h>
#include <ProjectWriter.h>
#include <SelectedPage.h>
#include <StageSequence.h>
#include <filters/fix_orientation/Filter.h>
#include <filters/fix_orientation/Task.h>
#include <filters/output/ColorParams.h>
#include <filters/output/DepthPerception.h>
#include <filters/output/DewarpingOptions.h>
#include <filters/page_layout/Alignment.h>
#include <filters/page_split/Filter.h>
#include <filters/page_split/LayoutType.h>
#include <filters/page_split/Task.h>

#include <QByteArray>
#include <QChar>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRgb>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextStream>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace {

const char* const kProfileName = "RoundTrip";

// Base names of the five pages, in creation order.  APFS treats the NFC and the NFD form of one name as the same
// file, so the two umlaut names differ in their suffix and never collide.
QString nameWithMarkup() {
  return QStringLiteral("a&b <c> \"d\" 'e'.png");
}

QString nameNfc() {
  return QString::fromUtf16(u"Seite_ä_NFC.png");  // U+00E4, one code point
}

QString nameNfd() {
  return QString::fromUtf16(u"Seite_ä_NFD.png");  // U+0061 U+0308, two code points
}

QString nameOneCharacter() {
  return QStringLiteral("x.png");
}

QString nameTwoHundredBytes() {
  return QString(100, QChar(0x00dc)) + QStringLiteral(".png");  // 100 x U+00DC = 200 bytes of UTF-8, plus ".png"
}

QString inputDirName() {
  return QStringLiteral("in & <dir> \"q\" 'a'");
}

QString outputDirName() {
  return QStringLiteral("out & <x> \"y\"");
}

// All pages are selectable, the first one is selected.
class AllPagesProvider : public PageSelectionProvider {
 public:
  explicit AllPagesProvider(std::shared_ptr<ProjectPages> pages) : m_pages(std::move(pages)) {}

  PageSequence allPages() const override { return m_pages->toPageSequence(PAGE_VIEW); }

  std::set<PageId> selectedPages() const override {
    std::set<PageId> selected;
    const PageSequence all = allPages();
    if (all.numPages() > 0) {
      selected.insert(all.pageAt(0).id());
    }
    return selected;
  }

  std::vector<PageRange> selectedRanges() const override {
    PageRange range;
    const PageSequence all = allPages();
    for (const PageInfo& page : all) {
      range.pages.push_back(page.id());
    }
    return std::vector<PageRange>{range};
  }

 private:
  std::shared_ptr<ProjectPages> m_pages;
};

// Parameters that differ from DefaultParams() in every stage.  Every number is one the UI can enter (at most two
// decimals).  Recorded in records/round-trip.md, section "Non-default values per stage".
std::unique_ptr<DefaultParams> makeNonDefaultParams() {
  auto params = std::make_unique<DefaultParams>();

  OrthogonalRotation rotation;
  rotation.nextClockwiseDirection();  // 90 degrees clockwise
  params->setFixOrientationParams(DefaultParams::FixOrientationParams(rotation));

  params->setPageSplitParams(DefaultParams::PageSplitParams(page_split::SINGLE_PAGE_UNCUT));

  params->setDeskewParams(DefaultParams::DeskewParams(1.25, MODE_MANUAL, true));

  // Letter size in millimetres (the default is 210 x 297), page detection on with fine-tuned corners, content
  // detection off.
  params->setSelectContentParams(DefaultParams::SelectContentParams(QSizeF(215.9, 279.4), false, MODE_AUTO, true));

  params->setPageLayoutParams(DefaultParams::PageLayoutParams(
      Margins(7.5, 8.25, 9.0, 10.5), page_layout::Alignment(page_layout::Alignment::TOP, page_layout::Alignment::LEFT),
      false));

  output::ColorParams colorParams;
  colorParams.setColorMode(output::MIXED);
  output::DewarpingOptions dewarping(output::AUTO, true);
  params->setOutputParams(DefaultParams::OutputParams(Dpi(400, 400), colorParams, output::SplittingOptions(),
                                                      output::PictureShapeOptions(), output::DepthPerception(2.5),
                                                      dewarping, 2.0));
  return params;
}

// Saves the process-wide default parameters and puts them back, so no profile of this suite outlives it (research P8).
class RoundTripFixture {
 public:
  RoundTripFixture()
      : m_tempDir(QDir::tempPath() + QStringLiteral("/project_round_trip-XXXXXX")),
        m_originalParams(std::make_unique<DefaultParams>(DefaultParamsProvider::getInstance().getParams())),
        m_originalProfileName(DefaultParamsProvider::getInstance().getProfileName()) {
    BOOST_REQUIRE(m_tempDir.isValid());
    BOOST_REQUIRE(m_tempDir.path().startsWith(QString::fromUtf8(TEST_TMP_ROOT)));

    const QDir root(m_tempDir.path());
    BOOST_REQUIRE(root.mkpath(inputDirName()));
    BOOST_REQUIRE(root.mkpath(outputDirName()));

    const QStringList names{nameWithMarkup(), nameNfc(), nameNfd(), nameOneCharacter(), nameTwoHundredBytes()};
    int index = 0;
    for (const QString& name : names) {
      QImage image(64, 64, QImage::Format_RGB32);
      image.fill(qRgb(40 * index, 255 - 40 * index, 128));
      image.setDotsPerMeterX(11811);  // 300 dpi
      image.setDotsPerMeterY(11811);
      const QString path = inputPath(name);
      BOOST_REQUIRE_MESSAGE(image.save(path, "PNG"), "cannot write " << path.toStdString());
      m_inputPaths.push_back(path);
      ++index;
    }
  }

  ~RoundTripFixture() {
    DefaultParamsProvider::getInstance().setParams(std::move(m_originalParams), m_originalProfileName);
    BOOST_CHECK_EQUAL(DefaultParamsProvider::getInstance().getProfileName().toStdString(),
                      m_originalProfileName.toStdString());
  }

  QString root() const { return m_tempDir.path(); }

  QString inputPath(const QString& name) const { return QDir(root()).filePath(inputDirName() + "/" + name); }

  QString outputDir() const { return QDir(root()).filePath(outputDirName()); }

  QString projectPath(const QString& name) const { return QDir(root()).filePath(name); }

  const std::vector<QString>& inputPaths() const { return m_inputPaths; }

  // Writes a 64 x 64 PNG at a path relative to the fixture root, creating its folder, and returns the absolute path.
  QString writePng(const QString& relativePath) const {
    const QString path = QDir(root()).filePath(relativePath);
    BOOST_REQUIRE_MESSAGE(QDir().mkpath(QFileInfo(path).absolutePath()),
                          "cannot create the folder of " << relativePath.toStdString());
    QImage image(64, 64, QImage::Format_RGB32);
    image.fill(qRgb(10, 120, 200));
    image.setDotsPerMeterX(11811);  // 300 dpi
    image.setDotsPerMeterY(11811);
    BOOST_REQUIRE_MESSAGE(image.save(path, "PNG"), "cannot write " << relativePath.toStdString());
    return path;
  }

 private:
  QTemporaryDir m_tempDir;
  std::unique_ptr<DefaultParams> m_originalParams;
  QString m_originalProfileName;
  std::vector<QString> m_inputPaths;
};

QByteArray readFile(const QString& path) {
  QFile file(path);
  BOOST_REQUIRE_MESSAGE(file.open(QIODevice::ReadOnly), "cannot read " << path.toStdString());
  return file.readAll();
}

QDomDocument parse(const QByteArray& bytes) {
  QDomDocument doc;
  const auto result = doc.setContent(bytes);
  BOOST_REQUIRE_MESSAGE(static_cast<bool>(result),
                        "project file is not well-formed XML: " << result.errorMessage.toStdString());
  return doc;
}

QDomElement filterElement(const QDomDocument& doc, const QString& name) {
  const QDomElement filters = doc.documentElement().namedItem("filters").toElement();
  BOOST_REQUIRE(!filters.isNull());
  const QDomElement filter = filters.namedItem(name).toElement();
  BOOST_REQUIRE_MESSAGE(!filter.isNull(), "no <" << name.toStdString() << "> under <filters>");
  return filter;
}

QString serialize(const QDomElement& element) {
  QString text;
  QTextStream stream(&text);
  element.save(stream, 2);
  return text;
}

// DefaultParams alone puts nothing of the page_split stage into a project file: Filter::writeImageSettings appends an
// <image> element only for a page whose split parameters exist, and those come from running the stage (finding
// F-08-1, records/round-trip.md).  So the stage is run for every page, with the real filters, fix_orientation first
// like the application's task chain.  With the layout type SINGLE_PAGE_UNCUT it needs no content detection.
void runPageSplit(StageSequence& stages, const PageSequence& sequence) {
  for (const PageInfo& page : sequence) {
    const auto pageSplitTask
        = stages.pageSplitFilter()->createTask(page, nullptr, /*batchProcessing=*/true, /*debug=*/false);
    const auto fixOrientationTask
        = stages.fixOrientationFilter()->createTask(page.id(), pageSplitTask, /*batchProcessing=*/true);
    fixOrientationTask->process(NullTaskStatus(), FilterData(ImageLoader::loadSource(page.id().imageId())));
  }
}

// The result of write -> read -> write.
struct RoundTripResult {
  QByteArray firstFile;
  QByteArray secondFile;
  bool readSucceeded = false;
  QString readOutputDirectory;
  std::vector<QString> readPagePaths;
};

RoundTripResult roundTrip(const RoundTripFixture& fixture,
                          const std::vector<QString>& files,
                          const bool nonDefault,
                          const QString& firstName,
                          const QString& secondName,
                          const QString& outputDirOverride = QString()) {
  DefaultParamsProvider::getInstance().setParams(
      nonDefault ? makeNonDefaultParams() : std::make_unique<DefaultParams>(),
      nonDefault ? QString(kProfileName) : QString("Default"));

  std::vector<ImageInfo> images;
  for (const QString& path : files) {
    images.emplace_back(ImageId(path), ImageMetadata(QSize(64, 64), Dpi(300, 300)), 1, false, false);
  }
  const auto pages = std::make_shared<ProjectPages>(images, Qt::LeftToRight);
  const auto provider = std::make_shared<AllPagesProvider>(pages);
  StageSequence stages(pages, PageSelectionAccessor(provider));

  const PageSequence sequence = pages->toPageSequence(PAGE_VIEW);
  for (const PageInfo& page : sequence) {
    for (const StageSequence::FilterPtr& filter : stages.filters()) {
      filter->loadDefaultSettings(page);
    }
  }

  if (nonDefault) {
    runPageSplit(stages, sequence);
  }

  const auto disambiguator = std::make_shared<FileNameDisambiguator>();
  const QString outDir = outputDirOverride.isEmpty() ? fixture.outputDir() : outputDirOverride;
  const OutputFileNameGenerator nameGenerator(disambiguator, outDir, Qt::LeftToRight);
  const QString firstPath = fixture.projectPath(firstName);
  const ProjectWriter writer(pages, SelectedPage(sequence.pageAt(0).id(), PAGE_VIEW), nameGenerator);
  BOOST_REQUIRE(writer.write(firstPath, stages.filters()));

  RoundTripResult result;
  result.firstFile = readFile(firstPath);

  const QDomDocument doc = parse(result.firstFile);
  {
    // The read path of MainWindow::openProject: setContent from a QFile, not from bytes.
    QFile file(firstPath);
    BOOST_REQUIRE(file.open(QIODevice::ReadOnly));
    QDomDocument fromFile;
    const auto fileResult = fromFile.setContent(&file);
    BOOST_REQUIRE_MESSAGE(static_cast<bool>(fileResult),
                          "project file does not load from a QFile: " << fileResult.errorMessage.toStdString());
  }
  const ProjectReader reader(doc, firstPath);
  result.readSucceeded = reader.success();
  BOOST_REQUIRE(result.readSucceeded);
  result.readOutputDirectory = reader.outputDirectory();

  const auto readProvider = std::make_shared<AllPagesProvider>(reader.pages());
  StageSequence readStages(reader.pages(), PageSelectionAccessor(readProvider));
  reader.readFilterSettings(readStages.filters());

  const PageSequence readSequence = reader.pages()->toPageSequence(PAGE_VIEW);
  for (const PageInfo& page : readSequence) {
    result.readPagePaths.push_back(page.id().imageId().filePath());
  }

  const OutputFileNameGenerator readNameGenerator(reader.namingDisambiguator(), reader.outputDirectory(),
                                                  reader.pages()->layoutDirection());
  const QString secondPath = fixture.projectPath(secondName);
  const ProjectWriter secondWriter(reader.pages(), reader.selectedPage(), readNameGenerator);
  BOOST_REQUIRE(secondWriter.write(secondPath, readStages.filters()));
  result.secondFile = readFile(secondPath);
  return result;
}

}  // namespace

BOOST_FIXTURE_TEST_SUITE(ProjectRoundTripTestSuite, RoundTripFixture)

BOOST_AUTO_TEST_CASE(test_round_trip_is_byte_identical) {
  const RoundTripResult result = roundTrip(*this, inputPaths(), true, "first.ScanTailor", "second.ScanTailor");
  BOOST_CHECK(!result.firstFile.isEmpty());
  BOOST_CHECK_MESSAGE(result.firstFile == result.secondFile,
                      "written, read and written again differ: " << result.firstFile.size() << " vs "
                                                                 << result.secondFile.size() << " bytes");
}

BOOST_AUTO_TEST_CASE(test_every_stage_carries_non_default_values) {
  const RoundTripResult nonDefault
      = roundTrip(*this, inputPaths(), true, "non_default.ScanTailor", "non_default_2.ScanTailor");
  const RoundTripResult plain = roundTrip(*this, inputPaths(), false, "default.ScanTailor", "default_2.ScanTailor");

  const QDomDocument nonDefaultDoc = parse(nonDefault.firstFile);
  const QDomDocument plainDoc = parse(plain.firstFile);
  const char* const stages[] = {"fix-orientation", "page-split", "deskew", "select-content", "page-layout", "output"};
  for (const char* stage : stages) {
    const QString name = QString::fromLatin1(stage);
    BOOST_CHECK_MESSAGE(serialize(filterElement(nonDefaultDoc, name)) != serialize(filterElement(plainDoc, name)),
                        "stage <" << stage << "> is the same with non-default and with default parameters");
  }

  // For page-split the default project carries no <image> at all (F-08-1), so the serialised elements above differ by
  // presence alone.  The value itself is asserted here, against the file-format string users' project files contain.
  BOOST_REQUIRE(makeNonDefaultParams()->getPageSplitParams().getLayoutType() == page_split::SINGLE_PAGE_UNCUT);
  BOOST_CHECK_MESSAGE(filterElement(plainDoc, "page-split").elementsByTagName("image").isEmpty(),
                      "the default project has a <page-split> <image>");
  const std::pair<const char*, const QByteArray*> written[]
      = {{"first", &nonDefault.firstFile}, {"second", &nonDefault.secondFile}};
  for (const auto& file : written) {
    const QDomDocument doc = parse(*file.second);
    const QDomNodeList images = filterElement(doc, "page-split").elementsByTagName("image");
    BOOST_REQUIRE_EQUAL(static_cast<size_t>(images.size()), inputPaths().size());
    for (int i = 0; i < images.size(); ++i) {
      const QDomElement image = images.at(i).toElement();
      const QString layoutType = image.attribute("layoutType");
      BOOST_CHECK_MESSAGE(layoutType == "single-uncut", "page-split image " << i << " of the " << file.first
                                                                            << " file has layoutType=\""
                                                                            << layoutType.toStdString() << "\"");
      const QString pagesType = image.firstChildElement("params").firstChildElement("pages").attribute("type");
      BOOST_CHECK_MESSAGE(pagesType == "single-uncut", "page-split image " << i << " of the " << file.first
                                                                           << " file has pages type=\""
                                                                           << pagesType.toStdString() << "\"");
    }
  }
}

BOOST_AUTO_TEST_CASE(test_names_and_order_survive) {
  const RoundTripResult result = roundTrip(*this, inputPaths(), true, "names.ScanTailor", "names_2.ScanTailor");

  BOOST_REQUIRE_EQUAL(result.readPagePaths.size(), inputPaths().size());
  for (size_t i = 0; i < inputPaths().size(); ++i) {
    // Plain QString equality is a comparison of UTF-16 code units: no normalisation, so NFC stays NFC, NFD stays NFD.
    BOOST_CHECK_MESSAGE(result.readPagePaths[i] == inputPaths()[i],
                        "page " << i << " came back as " << result.readPagePaths[i].toStdString() << ", was "
                                << inputPaths()[i].toStdString());
  }

  // Both forms are written as they are, in the UTF-8 bytes of the file.
  BOOST_CHECK(nameNfc() != nameNfd());
  BOOST_CHECK(nameNfc().toUtf8() != nameNfd().toUtf8());
  BOOST_CHECK(result.firstFile.contains(nameNfc().toUtf8()));
  BOOST_CHECK(result.firstFile.contains(nameNfd().toUtf8()));
  BOOST_CHECK(result.firstFile.contains(nameTwoHundredBytes().toUtf8()));
  const std::set<QString> distinct(result.readPagePaths.begin(), result.readPagePaths.end());
  BOOST_CHECK_EQUAL(distinct.size(), inputPaths().size());

  BOOST_CHECK_MESSAGE(result.readOutputDirectory == outputDir(), "output directory came back as "
                                                                     << result.readOutputDirectory.toStdString()
                                                                     << ", was " << outputDir().toStdString());
}

BOOST_AUTO_TEST_CASE(test_numeric_values_parse_back_exactly) {
  const RoundTripResult result = roundTrip(*this, inputPaths(), true, "numbers.ScanTailor", "numbers_2.ScanTailor");
  const QDomDocument doc = parse(result.firstFile);
  const size_t numPages = inputPaths().size();

  // The values the project was built from.
  const std::unique_ptr<DefaultParams> set = makeNonDefaultParams();
  const Margins& margins = set->getPageLayoutParams().getHardMargins();

  // Each value is checked against the text in the file: it must be a number, equal to the value set, and one the UI can
  // enter (at most two decimals).
  const auto checkAttribute = [&](const QDomElement& element, const char* attribute, const double expected) {
    bool ok = false;
    const double parsed = element.attribute(QString::fromLatin1(attribute)).toDouble(&ok);
    BOOST_CHECK_MESSAGE(ok, "<" << element.tagName().toStdString() << " " << attribute << "=\""
                                << element.attribute(QString::fromLatin1(attribute)).toStdString()
                                << "\"> is not a number");
    BOOST_CHECK_MESSAGE(parsed == expected, "<" << element.tagName().toStdString() << " " << attribute
                                                << "> parses back as " << parsed << ", was set to " << expected);
    BOOST_CHECK_MESSAGE(QString::number(expected, 'f', 2).toDouble() == expected,
                        "the value " << expected << " of " << attribute << " has more than two decimals");
  };

  const QDomNodeList angles = filterElement(doc, "deskew").elementsByTagName("params");
  BOOST_REQUIRE_EQUAL(static_cast<size_t>(angles.size()), numPages);
  for (int i = 0; i < angles.size(); ++i) {
    checkAttribute(angles.at(i).toElement(), "angle", set->getDeskewParams().getDeskewAngleDeg());
  }

  const QDomNodeList hardMargins = filterElement(doc, "page-layout").elementsByTagName("hardMarginsMM");
  BOOST_REQUIRE_EQUAL(static_cast<size_t>(hardMargins.size()), numPages);
  for (int i = 0; i < hardMargins.size(); ++i) {
    const QDomElement element = hardMargins.at(i).toElement();
    checkAttribute(element, "left", margins.left());
    checkAttribute(element, "top", margins.top());
    checkAttribute(element, "right", margins.right());
    checkAttribute(element, "bottom", margins.bottom());
  }

  const output::DepthPerception depth = set->getOutputParams().getDepthPerception();
  const QDomNodeList outputParams = filterElement(doc, "output").elementsByTagName("params");
  BOOST_REQUIRE_EQUAL(static_cast<size_t>(outputParams.size()), numPages);
  for (int i = 0; i < outputParams.size(); ++i) {
    const QDomElement element = outputParams.at(i).toElement();
    checkAttribute(element, "depthPerception", depth.value());
    checkAttribute(element, "despeckleLevel", set->getOutputParams().getDespeckleLevel());
  }

  const QDomNodeList dpis = filterElement(doc, "output").elementsByTagName("dpi");
  BOOST_REQUIRE_EQUAL(static_cast<size_t>(dpis.size()), numPages);
  for (int i = 0; i < dpis.size(); ++i) {
    checkAttribute(dpis.at(i).toElement(), "horizontal", set->getOutputParams().getDpi().horizontal());
    checkAttribute(dpis.at(i).toElement(), "vertical", set->getOutputParams().getDpi().vertical());
  }

  const QDomNodeList rotations = filterElement(doc, "fix-orientation").elementsByTagName("rotation");
  BOOST_REQUIRE_EQUAL(static_cast<size_t>(rotations.size()), numPages);
  for (int i = 0; i < rotations.size(); ++i) {
    checkAttribute(rotations.at(i).toElement(), "degrees",
                   set->getFixOrientationParams().getImageRotation().toDegrees());
  }
}

BOOST_AUTO_TEST_CASE(test_single_page_round_trip) {
  const std::vector<QString> onePage{inputPaths().front()};
  const RoundTripResult result = roundTrip(*this, onePage, true, "single.ScanTailor", "single_2.ScanTailor");
  BOOST_CHECK(!result.firstFile.isEmpty());
  BOOST_CHECK_MESSAGE(result.firstFile == result.secondFile,
                      "one-page project: written, read and written again differ: "
                          << result.firstFile.size() << " vs " << result.secondFile.size() << " bytes");
  BOOST_REQUIRE_EQUAL(result.readPagePaths.size(), 1u);
  BOOST_CHECK(result.readPagePaths.front() == onePage.front());
}

BOOST_AUTO_TEST_CASE(test_control_characters_survive) {
  // XML 1.0 forbids these characters; QDom writes them raw and the file then no longer loads (SAFE-02).
  const QString inDir = QStringLiteral("in") + QChar(0x0001) + QStringLiteral("dir");
  const QString outDir = QStringLiteral("out") + QChar(0x001f) + QStringLiteral("dir");
  const QString nameOne = QStringLiteral("ctrl") + QChar(0x0001) + QStringLiteral("one.png");
  const QString nameUnit = QStringLiteral("ctrl") + QChar(0x001f) + QStringLiteral("unit.png");

  const std::vector<QString> files{writePng(inDir + "/" + nameOne), writePng(inDir + "/" + nameUnit)};
  const QString outputPath = QDir(root()).filePath(outDir);
  BOOST_REQUIRE(QDir().mkpath(outputPath));

  const RoundTripResult result
      = roundTrip(*this, files, true, "control.ScanTailor", "control_2.ScanTailor", outputPath);

  BOOST_REQUIRE_EQUAL(result.readPagePaths.size(), files.size());
  for (size_t i = 0; i < files.size(); ++i) {
    BOOST_CHECK_MESSAGE(result.readPagePaths[i] == files[i], "page " << i << " did not come back unchanged");
  }
  BOOST_CHECK_MESSAGE(result.readOutputDirectory == outputPath, "the output folder did not come back unchanged");
  BOOST_CHECK_MESSAGE(result.firstFile == result.secondFile,
                      "written, read and written again differ: " << result.firstFile.size() << " vs "
                                                                 << result.secondFile.size() << " bytes");
  BOOST_CHECK(result.firstFile.contains("nameEscaped=\"1\""));
  BOOST_CHECK(result.firstFile.contains("pathEscaped=\"1\""));
  BOOST_CHECK(result.firstFile.contains("outputDirectoryEscaped=\"1\""));
}

BOOST_AUTO_TEST_CASE(test_ordinary_projects_carry_no_escape_flag) {
  const RoundTripResult result = roundTrip(*this, inputPaths(), true, "plain.ScanTailor", "plain_2.ScanTailor");
  BOOST_CHECK_MESSAGE(!result.firstFile.contains("Escaped=\""), "an ordinary project carries an escape flag");
}

// Names that are legal on APFS but unusual in XML.  None of them needs the escape of SAFE-02, so each is written
// as QDom writes it (tab, LF and CR as character references) and the file carries no flag attribute.
BOOST_AUTO_TEST_CASE(test_unusual_names_survive) {
  const QString dir = QStringLiteral(" in") + QChar(0x000a) + QStringLiteral("dir");
  const QStringList names{QStringLiteral("tab") + QChar(0x0009) + QStringLiteral("name.png"),
                          QStringLiteral("line") + QChar(0x000a) + QStringLiteral("feed.png"),
                          QStringLiteral("carriage") + QChar(0x000d) + QStringLiteral("return.png"),
                          QStringLiteral("crlf") + QChar(0x000d) + QChar(0x000a) + QStringLiteral("name.png"),
                          QStringLiteral(" leading space.png"),
                          QStringLiteral("trailing space .png"),
                          QStringLiteral("it's.png"),
                          QStringLiteral("a>b.png"),
                          QString::fromUtf8("\xF0\x9F\x93\x84 page.png"),
                          QStringLiteral("del") + QChar(0x007f) + QStringLiteral(".png")};
  std::vector<QString> files;
  for (const QString& name : names) {
    files.push_back(writePng(dir + "/" + name));
  }
  // Tab, CR and CRLF in a folder name (LF is the folder name above), one file in each.
  const QStringList folders{QStringLiteral("tab") + QChar(0x0009) + QStringLiteral("dir"),
                            QStringLiteral("carriage") + QChar(0x000d) + QStringLiteral("dir"),
                            QStringLiteral("crlf") + QChar(0x000d) + QChar(0x000a) + QStringLiteral("dir")};
  for (const QString& folder : folders) {
    files.push_back(writePng(folder + "/page.png"));
  }

  const RoundTripResult result = roundTrip(*this, files, true, "unusual.ScanTailor", "unusual_2.ScanTailor");

  BOOST_REQUIRE_EQUAL(result.readPagePaths.size(), files.size());
  for (size_t i = 0; i < files.size(); ++i) {
    BOOST_CHECK_MESSAGE(result.readPagePaths[i] == files[i], "name " << i << " did not come back unchanged");
  }
  BOOST_CHECK_MESSAGE(result.firstFile == result.secondFile,
                      "written, read and written again differ: " << result.firstFile.size() << " vs "
                                                                 << result.secondFile.size() << " bytes");
  BOOST_CHECK(!result.firstFile.contains("Escaped=\""));
}

BOOST_AUTO_TEST_SUITE_END()

// A backslash is legal in a macOS file name.  Backlog 999.8 concerns RelinkablePath::normalize, which relinking uses;
// the project file round trip does not go through it.
BOOST_FIXTURE_TEST_SUITE(ProjectRoundTripBackslashTestSuite, RoundTripFixture)

BOOST_AUTO_TEST_CASE(test_backslash_in_file_name) {
  const std::vector<QString> files{writePng(inputDirName() + "/" + QStringLiteral("back\\slash.png"))};

  const RoundTripResult result = roundTrip(*this, files, true, "backslash.ScanTailor", "backslash_2.ScanTailor");

  BOOST_REQUIRE_EQUAL(result.readPagePaths.size(), 1u);
  BOOST_CHECK(result.readPagePaths.front() == files.front());
  BOOST_CHECK_MESSAGE(result.firstFile == result.secondFile,
                      "written, read and written again differ: " << result.firstFile.size() << " vs "
                                                                 << result.secondFile.size() << " bytes");
}

BOOST_AUTO_TEST_SUITE_END()
