// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGESIZELIMIT_H_
#define SCANTAILOR_CORE_IMAGESIZELIMIT_H_

#include <QCoreApplication>
#include <QSize>
#include <QString>
#include <limits>

class ImageId;

/**
 * The one place that knows how large a source image may be and how the refusal is worded.
 *
 * A page must have fewer than 2^29 px in total and at most 65535 px per side. The 1200 dpi A3 reference page
 * (14031 x 19843 px, 278 MP) is inside the limit. Fewer than 2^29, not 2^29: the RGB32 byte count of the
 * largest accepted image, 4 * (2^29 - 1) = 2147483644, fits in an int, while exactly 2^29 px would need 2^31
 * bytes. The reasoning is recorded in records/safe-01.md (plan 06-06).
 *
 * All functions are stateless, so the thumbnail thread and the worker threads can call them in parallel.
 */
class ImageSizeLimit {
  Q_DECLARE_TR_FUNCTIONS(ImageSizeLimit)
 public:
  /** The largest accepted pixel count (inclusive): fewer than 2^29 px. */
  static constexpr qint64 kMaxPixels = (qint64(1) << 29) - 1;

  /** The largest accepted width or height (inclusive). */
  static constexpr int kMaxSide = 65535;

  /** QImageReader::setAllocationLimit() value in MB, applied once by the image loader (plan 06-05). */
  static constexpr int kQtAllocationLimitMb = 2048;

  /**
   * True if an image whose header claims \p size must not be loaded. Header sizes are unsigned 32-bit values
   * carried in int, so a negative side (a header value of 2^31 or more) counts as too large. A side of 0 does
   * not (the readers refuse it as unreadable instead).
   */
  static bool exceeds(const QSize& size);

  /** "W × H px", sides printed as unsigned numbers without locale grouping. */
  static QString sizeText(const QSize& size);

  /** "65535 px per side and 537 megapixels in total". */
  static QString limitText();

  /** Short form for lists: "too large: W × H px". */
  static QString tooLargeText(const QSize& size);

  /**
   * Reads the header of the page \p imageId refers to and returns true, with the claimed size in
   * \p claimedSize, only if the file could be read and the page exceeds the limit.
   */
  static bool claimedSizeIfTooLarge(const ImageId& imageId, QSize* claimedSize);
};

static_assert(4 * ImageSizeLimit::kMaxPixels <= std::numeric_limits<int>::max(),
              "the RGB32 byte count of the largest accepted image must fit in an int");
static_assert(ImageSizeLimit::kQtAllocationLimitMb * (qint64(1) << 20) >= 4 * ImageSizeLimit::kMaxPixels
                  && (ImageSizeLimit::kQtAllocationLimitMb - 1) * (qint64(1) << 20) < 4 * ImageSizeLimit::kMaxPixels,
              "the Qt allocation limit must be the smallest whole number of MB holding the largest accepted image");

#endif  // ifndef SCANTAILOR_CORE_IMAGESIZELIMIT_H_
