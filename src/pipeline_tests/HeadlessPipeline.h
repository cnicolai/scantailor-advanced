// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PIPELINE_TESTS_HEADLESSPIPELINE_H_
#define SCANTAILOR_PIPELINE_TESTS_HEADLESSPIPELINE_H_

#include <ImageSettings.h>
#include <PageId.h>
#include <filters/deskew/Settings.h>
#include <filters/fix_orientation/Settings.h>
#include <filters/output/Settings.h>
#include <filters/page_layout/Settings.h>
#include <filters/page_split/LayoutType.h>
#include <filters/page_split/Settings.h>
#include <filters/select_content/Settings.h>

#include <QImage>
#include <QLineF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <functional>
#include <memory>
#include <vector>

struct StageSettings {
  std::shared_ptr<ImageSettings> imageSettings;
  std::shared_ptr<fix_orientation::Settings> fixOrientation;
  std::shared_ptr<page_split::Settings> pageSplit;
  std::shared_ptr<deskew::Settings> deskew;
  std::shared_ptr<select_content::Settings> selectContent;
  std::shared_ptr<page_layout::Settings> pageLayout;
  std::shared_ptr<output::Settings> output;
};

enum class LastStage { DESKEW, OUTPUT };

struct PipelineOptions {
  page_split::LayoutType layout = page_split::AUTO_LAYOUT_TYPE;  // AUTO keeps the default update action
  output::ColorMode colorMode = output::BLACK_AND_WHITE;
  double despeckleLevel = 1.0;                                    // app default
  output::DewarpingMode dewarping = output::OFF;
  int outputDpi = 600;                                            // app default, set explicitly
  LastStage lastStage = LastStage::OUTPUT;
  std::function<void(StageSettings&, const PageId&)> configure;   // called after defaults, before processing
};

struct SubPageResult {
  PageId pageId;
  double deskewAngleDeg = 0.0;
  QRect contentRect;  // select_content, deskewed frame
  QRectF pageRect;    // select_content
  QSize outputSize;
  int outputDpiX = 0;
  int outputDpiY = 0;
  QImage output;      // decoded output file (ImageLoader), null for LastStage::DESKEW
  // margins of page_layout, recorded in text goldens:
  double hardMarginLeftMM = 0.0;
  double hardMarginTopMM = 0.0;
  double hardMarginRightMM = 0.0;
  double hardMarginBottomMM = 0.0;
};

struct PipelineResult {
  // page_split::PageLayout::Type of the stored layout: SINGLE_PAGE_UNCUT 0, SINGLE_PAGE_CUT 1, TWO_PAGES 2
  int layoutType = 0;
  std::vector<QLineF> cutters;       // page_split cutter lines, input coordinates
  std::vector<SubPageResult> pages;  // one per sub-page (two for TWO_PAGES)
  StageSettings settings;            // for scenario-specific reads
};

// This file is frozen after plan 02-03 of the golden baselines phase.  Wave-3 scenario plans configure
// scenarios through PipelineOptions::configure (called per sub-page after the stage defaults and before
// processing) and must not edit HeadlessPipeline.h or HeadlessPipeline.cpp.  The spread, the AUTO dewarping
// and the MIXED output paths are proven by PipelineSmokeTestSuite.

/**
 * Runs one image through the six stage Tasks the way MainWindow::createCompositeTask chains them, but
 * without a GUI: every call uses fresh Settings objects, a fresh temporary project directory under
 * QDir::tempPath() and the built-in "Default" parameter profile.  Throws std::runtime_error when a stage
 * leaves no parameters.  Needs the core Application object (see main.cpp).
 */
PipelineResult runPipeline(const QImage& input, const PipelineOptions& options);

#endif  // SCANTAILOR_PIPELINE_TESTS_HEADLESSPIPELINE_H_
