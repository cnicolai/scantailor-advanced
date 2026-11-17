// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGELOADER_H_
#define SCANTAILOR_CORE_IMAGELOADER_H_

class ImageId;
class QImage;
class QString;
class QIODevice;

class ImageLoader {
 public:
  /**
   * Loads an image the application wrote itself: its cached output, the foreground and background layers, the
   * automask and speckles files, the thumbnail files. These can legitimately exceed ImageSizeLimit (an output at
   * a high dpi is larger than its source), so the limit is not applied; a refusal here would make the page
   * regenerate on every visit. Qt's own allocation limit still applies to formats other than TIFF.
   */
  static QImage load(const QString& filePath, int pageNum = 0);

  static QImage load(const ImageId& imageId);

  static QImage load(QIODevice& ioDev, int pageNum);

  /**
   * Loads an image from the user's source scans, which are untrusted: a header that claims more than
   * ImageSizeLimit allows is refused with a null image before any pixel buffer exists.
   */
  static QImage loadSource(const ImageId& imageId);

  static QImage loadSource(QIODevice& ioDev, int pageNum);

 private:
  static QImage loadImpl(QIODevice& ioDev, int pageNum, bool enforceSizeLimit);
};


#endif
