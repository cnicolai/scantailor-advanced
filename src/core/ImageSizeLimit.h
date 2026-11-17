// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGESIZELIMIT_H_
#define SCANTAILOR_CORE_IMAGESIZELIMIT_H_

#include <QCoreApplication>
#include <QSize>
#include <QString>
#include <limits>
#include <vector>

class ImageId;
class ImageMetadata;

/**
 * The one place that knows how large a source image may be and how the refusal is worded.
 *
 * A page must have fewer than 2^29 px in total and at most 65535 px per side. The 1200 dpi A3 reference page
 * (14031 x 19843 px, 278 MP) is inside the limit. Fewer than 2^29, not 2^29: the RGB32 byte count of the
 * largest accepted image, 4 * (2^29 - 1) = 2147483644, fits in an int, while exactly 2^29 px would need 2^31
 * bytes. The reasoning is recorded in .planning/phases/06-hardening-and-cleanup/records/safe-01.md.
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

  /**
   * QImageReader::setAllocationLimit() value in MB, applied once by the image loader. Qt decodes a 16-bit RGB
   * PNG at 8 bytes per pixel, so such a file above about 268 Mpx can be refused by this limit with the general
   * load failure although it is inside kMaxPixels; accepted, not measured.
   */
  static constexpr int kQtAllocationLimitMb = 2048;

  /**
   * The ceiling for files the application wrote itself (output, layers, automask, speckles, thumbnails): fewer
   * than 2^30 px in total. The largest plausible output is a 500 mm side at 1200 dpi, 23622 px per side and about
   * 558 Mpx for a square page; the ceiling is about 1.9 times that and 3.9 times a real A3 page at 1200 dpi.
   * A refused own file is regenerated on every visit, so the ceiling is generous on purpose. It is a constant,
   * the same on every machine, and can be tested with a forged header. 2^30 px as RGB32 is 4 GiB, which no
   * longer fits an int (byte counts go through qsizetype), so no int assertion applies. A forged header just
   * inside the ceiling can therefore still cost an allocation of up to about 4 GiB; that is accepted, because
   * every file the application wrote itself must load.
   */
  static constexpr qint64 kMaxOwnFilePixels = (qint64(1) << 30) - 1;

  /** The largest accepted width or height (inclusive) of an own file. */
  static constexpr int kMaxOwnFileSide = 2 * kMaxSide;

  /** Which ceiling applies: the source limit for the user's scans, the own-file ceiling for the app's files. */
  enum class LimitKind { SOURCE, OWN_FILE };

  /**
   * True if an image whose header claims \p size must not be loaded. Header sizes are unsigned 32-bit values
   * carried in int, so a negative side (a header value of 2^31 or more) counts as too large. A side of 0 does
   * not (the readers refuse it as unreadable instead).
   */
  static bool exceeds(const QSize& size);

  /** Like exceeds(), against the own-file ceiling (kMaxOwnFilePixels, kMaxOwnFileSide). */
  static bool exceedsOwnFile(const QSize& size);

  /** exceeds() or exceedsOwnFile(), depending on \p kind. */
  static bool exceeds(const QSize& size, LimitKind kind);

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

  /**
   * Text for a file whose pages are all known from their headers: empty if no page exceeds the limit,
   * "too large: W × H px" for a one-page file and "page N too large: ..." (N is 1-based, the first
   * oversized page) for a file with several pages. Used where a whole file is accepted or refused.
   */
  static QString oversizedPagesText(const std::vector<ImageMetadata>& pages);
};

static_assert(4 * ImageSizeLimit::kMaxPixels <= std::numeric_limits<int>::max(),
              "the RGB32 byte count of the largest accepted image must fit in an int");
static_assert(ImageSizeLimit::kMaxOwnFilePixels > ImageSizeLimit::kMaxPixels,
              "the own-file pixel ceiling must be above the source limit");
static_assert(ImageSizeLimit::kMaxOwnFileSide > ImageSizeLimit::kMaxSide,
              "the own-file side ceiling must be above the source limit");
static_assert(ImageSizeLimit::kQtAllocationLimitMb * (qint64(1) << 20) >= 4 * ImageSizeLimit::kMaxPixels
                  && (ImageSizeLimit::kQtAllocationLimitMb - 1) * (qint64(1) << 20) < 4 * ImageSizeLimit::kMaxPixels,
              "the Qt allocation limit must be the smallest whole number of MB holding the largest accepted image");

#endif  // ifndef SCANTAILOR_CORE_IMAGESIZELIMIT_H_
