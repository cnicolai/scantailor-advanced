// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

// Opt-in tool for the OutputGenerator split of Phase 6 (CLEAN-01, D-16). The case never runs under ctest: it is
// disabled by default and only runs when named, see the comment above it.

#include <PageGenerator.h>
#include <filters/output/DewarpingOptions.h>
#include <filters/output/Params.h>
#include <filters/page_split/LayoutType.h>

#include <QByteArray>
#include <QByteArrayView>
#include <QCryptographicHash>
#include <QImage>
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <vector>

#include "HeadlessPipeline.h"

namespace {

struct DigestConfig {
  const char* kindName;
  test_support::PageKind kind;
  int dpi;
  const char* colourName;
  output::ColorMode colour;
  const char* dewarpName;
  output::DewarpingMode dewarp;
};

// SHA-1 of the decoded output pixels, row by row without padding. A bilevel image is expanded to 8 bit first, so
// the unspecified padding bits of its rows never enter the hash.
QByteArray pixelDigest(const QImage& output) {
  const QImage image = output.depth() == 1 ? output.convertToFormat(QImage::Format_Grayscale8) : output;
  QCryptographicHash hash(QCryptographicHash::Sha1);
  for (int y = 0; y < image.height(); ++y) {
    const char* const row = reinterpret_cast<const char*>(image.constScanLine(y));
    hash.addData(QByteArrayView(row, static_cast<qsizetype>((image.width() * image.depth() + 7) / 8)));
  }
  return hash.result().toHex();
}

}  // namespace

BOOST_AUTO_TEST_SUITE(OutputDigestProbeTestSuite)

// A tool for the CLEAN-01 proof, never run by ctest. It runs seven pages through all six stages and prints one
// line per page: DIGEST <kind> <dpi> <colour> <dewarping> - <sha1 of the decoded output pixels>. Run it with
// --run_test=OutputDigestProbeTestSuite --log_level=message before and after a move in OutputGenerator.cpp and
// compare the lines; they must be equal.
BOOST_AUTO_TEST_CASE(test_print_output_digests, *boost::unit_test::disabled()) {
  using test_support::PageKind;
  const std::vector<DigestConfig> configs = {
      {"flat", PageKind::FLAT, 300, "bw", output::BLACK_AND_WHITE, "off", output::OFF},
      {"flat", PageKind::FLAT, 600, "gray", output::COLOR_GRAYSCALE, "off", output::OFF},
      {"mixed", PageKind::MIXED, 300, "mixed", output::MIXED, "off", output::OFF},
      {"mixed", PageKind::MIXED, 600, "mixed", output::MIXED, "off", output::OFF},
      {"curved", PageKind::CURVED, 300, "gray", output::COLOR_GRAYSCALE, "auto", output::AUTO},
      {"curved", PageKind::CURVED, 600, "mixed", output::MIXED, "auto", output::AUTO},
      {"curved", PageKind::CURVED, 600, "bw", output::BLACK_AND_WHITE, "auto", output::AUTO},
  };

  for (const DigestConfig& config : configs) {
    test_support::PageSpec spec;
    spec.kind = config.kind;
    spec.dpi = config.dpi;
    const test_support::GeneratedPage page = test_support::generatePage(spec);
    BOOST_REQUIRE_MESSAGE(!page.image.isNull(), "the generator returned a null page");

    PipelineOptions options;
    options.layout = page_split::SINGLE_PAGE_UNCUT;
    options.colorMode = config.colour;
    options.dewarping = config.dewarp;
    options.outputDpi = config.dpi;
    const PipelineResult result = runPipeline(page.image, options);
    BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
    BOOST_REQUIRE(!result.pages[0].output.isNull());

    std::printf("DIGEST %s %d %s %s - %s\n", config.kindName, config.dpi, config.colourName, config.dewarpName,
                pixelDigest(result.pages[0].output).constData());
    std::fflush(stdout);
  }
}

BOOST_AUTO_TEST_SUITE_END()
