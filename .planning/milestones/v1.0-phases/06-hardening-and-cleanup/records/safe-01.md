# SAFE-01: image size limit, decisions, measurements, int-arithmetic audit and evidence

Plans 06-04, 06-05 and 06-06. Repository path written as `{repo}`, home path as `~`.

## The ceiling

| Constant | Value | Where |
|----------|-------|-------|
| `ImageSizeLimit::kMaxPixels` | 536870911 (fewer than 2^29 px in total, inclusive maximum 2^29 - 1) | `src/core/ImageSizeLimit.h` |
| `ImageSizeLimit::kMaxSide` | 65535 px per side | same |
| `ImageSizeLimit::kQtAllocationLimitMb` | 2048 (set once through `QImageReader::setAllocationLimit` in `ImageLoader::load`, never 0) | same, applied in `src/core/ImageLoader.cpp` |

Two `static_assert`s in `ImageSizeLimit.h` pin the arithmetic: the RGB32 byte count of the largest accepted image (4 x 536870911 = **2147483644**) fits in `int`, and 2048 is the smallest whole number of MB that holds it. The largest accepted rectangle is 55245 x 9718 = 2^29 - 2 px (2147483640 bytes as RGB32). The limit is exclusive on purpose: exactly 2^29 px would take 2^31 = 2147483648 bytes as RGB32, one more than `INT_MAX`.

### Why fewer than 2^29 px and not D-01's 1 gigapixel example

D-01 gave 1 gigapixel as an example of the intended size and left the exact number to research (research open question 3). The candidates:

| Ceiling | px | RGB32 bytes of the largest accepted image | Qt limit | Verdict |
|---------|----|-------------------------------------------|----------|---------|
| D-01 example, 2^30 | 1073741824 (1073741823 inclusive) | up to 2^32 - 4 (4294967292), above `INT_MAX` | 4096 MB | pixel counts stay below 2^31, but an `int` RGB32 byte count would overflow, so it would be safe only if the audit found no `int` byte count at all |
| 2^29 inclusive | 536870912 | 2^31 = 2147483648, one more than `INT_MAX` | 2048 MB | rejected: overflows by one |
| **fewer than 2^29** | **536870911** | **2147483644** | **2048 MB** | accepted: every `int` pixel and RGB32 byte count fits |

536870911 px is still 1.93 times the 1200 dpi A3 reference page (14031 x 19843 = 278417133 px, RGB32 1.11 GB), so every page the user scans loads; a 1200 dpi A2 (about 557 MP) and a 600 dpi A0 (about 558 MP) are refused by design. The ceiling stays one reversible constant (`ImageSizeLimit::kMaxPixels` and the Qt limit that follows from it): if a later audit or a user need allows 2^30, those two numbers change and the two `static_assert`s say what else must hold. The limit bounds the source image as loaded; derived images (deskew rotation, output dpi scaling) are sized by the user's settings and are not covered by it.

## D-03: the Qt reader limit (measured at research time, Qt 6.12.0, Release; files in `build/scratch/phase6/m1`, research session, not committed)

| Test file | Claimed size | `allocationLimit` 256 (default) | 2048 | 0 (off) |
|-----------|--------------|--------------------------------|------|---------|
| RGB32 PNG, 234 KB on disk | 7016 x 9921 (A3 600 dpi, 278422944 B decoded) | fails (`Unable to read image data`) | loads, 145 ms | loads, 124 ms |
| RGB32 JPEG, 4.5 MB | 7016 x 9921 | fails | loads, 96 ms | - |
| 8-bit grey PNG | 7016 x 9921 (69.6 MB decoded) | fails | - | - |
| mono PNG | 14031 x 19843 (34.8 MB) | fails | - | - |
| RGB32 PNG | 2480 x 3508 (A4 300 dpi) | loads | - | - |
| RGB PNG, forged IHDR | 8192 x 8192 / 8192 x 8193 | passes the limit / rejected | - | - |
| RGB PNG, forged IHDR | 100000 x 100000 and 65535 x 65535 | rejected | rejected | with limit 0 `QImageReader::read` returned an image object of the claimed size (40 GB virtual) before failing on missing data |

A 278 MB RGB32 PNG or JPEG therefore fails on the unmodified code, and so does every PNG, JPEG, grey or mono page above 64 Mi px (8192 x 8192 passes, 8192 x 8193 fails at the default 256 MB). `QImageReader::size()` returned the true claimed size in all cases without allocating. Limit 0 must never be used; plan 06-05 sets 2048 once and the test `testLargeGreyPngLoads` pins that the loader leaves it at 2048.

## Cost of forged headers on the unmodified code (research session, `build/scratch/phase6/m2` and `m5`, not committed)

| Forged file (size on disk) | Path | Result on the unmodified code |
|----------------------------|------|-------------------------------|
| TIFF 1-bit 30000 x 30000 (3.6 KB) | `TiffReader::readImage` | image returned, 116 MB footprint |
| TIFF RGB 12000 x 12000 (4.8 KB) | `readImage` | image returned, 572 MB |
| TIFF 1-bit 100000 x 100000 (10 KB) | `readImage` | image returned, 1.2 GB, 287 ms |
| TIFF 8-bit grey 60000 x 60000 (9.8 KB) | `readImage` | image returned, 3.5 GB |
| TIFF RGB 40000 x 40000 (13 KB) | `readImage` | image returned, 6.1 GB, 2.5 s |
| TIFF width = height = 2^32 - 1 | `readImage` | libtiff rejected the directory; null image |
| TIFF 2000000000 x 2 mono | `readImage` | accepted, 496 MB |
| JPEG baseline 65500 x 65500 | `JpegMetadataLoader` | 19 MB, 0 ms, size reported |
| JPEG progressive 65500 x 65500 (about 1 KB) | `JpegMetadataLoader` | 2.8 s, 12.3 GB |

All metadata calls reported `LOADED` with the claimed size. Plan 06-04 reproduced the TIFF figure in its mutation run (1250526096 bytes of peak growth with the refusal disabled).

## Where each check sits

| Site | Check now | Plan |
|------|-----------|------|
| `TiffReader::readImage` (`src/core/TiffReader.cpp:276`) | side <= 0 or `ImageSizeLimit::exceeds` after `currentPageMetadata`, before either `QImage` constructor; returns `QImage()`, never `std::bad_alloc` | 06-04 |
| `TiffReader::TiffBuffer` (`TiffReader.cpp:304`) and `readAndUnpackLines` | follow the one check above; the tmp buffer count goes through the 64-bit `tsize_t` | 06-04 |
| `TiffReader::readMetadata` | no allocation; reports every page header, consumers check each page | 06-04 |
| `ImageLoader::load` (`src/core/ImageLoader.cpp:37` and `:61`) | `std::call_once` raises Qt's limit to 2048 MB; `reader.size()` checked with `exceeds` before `QImageReader::read`; an invalid size falls through to Qt | 06-05 |
| `JpegMetadataLoader::loadMetadata` (`src/core/JpegMetadataLoader.cpp:214`) | size checked between `jpeg_read_header` (196) and `jpeg_start_decompress` (221); an oversized file is reported `LOADED` with its claimed size | 06-05 |
| `PngMetadataLoader::loadMetadata` (`src/core/PngMetadataLoader.cpp:86`) | no allocation; libpng's per-side user limit is lifted (`png_set_user_limits`) so every claimed size reaches the one consumer check | 06-05 |
| `LoadFileTask` (`src/core/LoadFileTask.cpp:69`) | a null image triggers `claimedSizeIfTooLarge`; the error widget says "too large to load" with the size and the limit | 06-04 |
| `ThumbnailPixmapCache::loadSaveThumbnail` | loads source images only through `ImageLoader::load` (`git grep -n QImageReader -- src/core/ThumbnailPixmapCache.cpp` prints nothing); a refusal gives a null image, `LOAD_FAILED`, and no thumbnail file | 06-06 (test) |
| `ProjectFilesDialog::FileList::loadNextFile` (`src/app/ProjectFilesDialog.cpp:601`) | `ImageSizeLimit::oversizedPagesText` after a `LOADED` status; `STATUS_TOO_LARGE`, red text with the claimed size, `LOAD_FAILED` result so the existing box appears and the dialog is not accepted | 06-06 |
| `MainWindow::showInsertFileDialog` (`src/app/MainWindow.cpp:1976`) | the same function; the file goes to the existing failed-files dialog with the too-large text appended to its path and is not added | 06-06 |

Besides `ImageSizeLimit::claimedSizeIfTooLarge`, only the two sites in the last two rows call `ImageMetadataLoader::load` (research open question 4).

### Multi-page files at creation (D-04 interpretation)

A file with at least one oversized page is refused as a whole in the project file dialog and in the insert flow, and the text names the first such page (`page 2 too large: 100000 × 100000 px`). The dialog adds whole files and the project model has no way to leave out one page of a file. Inside an existing project (an older project, or a file replaced on disk) the readers refuse only the oversized page and the other pages load (`test_multi_page_tiff_refuses_only_the_oversized_page`). Both places use the same `ImageSizeLimit::exceeds`, so creation and loading agree on the limit.

## Audit of the `int` arithmetic (closes RESEARCH assumption A2)

Question: can an `int` pixel, element or byte count computed from image dimensions or strides overflow for a source image the limit accepts (fewer than 2^29 px, sides at most 65535, at most 4 bytes per pixel, so at most 2147483644 bytes)?

### Scan rule

`build/scratch/int-audit/scan.py` (Python 3, run as `python3 -I`, standard library only) lists, over `git ls-files src/core src/imageproc src/dewarping src/foundation src/math` (`.cpp` and `.h`, paths containing `/tests/` left out), every line, comments stripped, with a product of two image-dimension or stride terms that mentions no 64-bit or floating type. A term is an identifier chain whose last name contains `width`, `height`, `stride`, `bpl`, `wpl` or `bytesPerLine` in any case (member and accessor forms included), or is the bare name `w` or `h`; the excluded types are `qint64`, `quint64`, `int64_t`, `uint64_t`, `size_t`, `qsizetype`, `double` and `float`. Positive control: the list contains `src/imageproc/Posterizer.cpp:221` and `src/imageproc/PolynomialSurface.cpp:34` (RESEARCH Pitfall 4). The scan printed **47** candidate lines (the planning-time scan with a looser reading of "term" printed 60). A wider scan (a dimension or stride term times anything, 208 lines, `supplement-all.txt`) was read as a cross-check; its extra 161 lines are row offsets (`y * stride`, `rect.top() * wpl`, below the byte count of the image they index, class c), small constant factors (`width * 3`, `wpl * 2`, `* 0.5`), floating-point products, and the derived-image sizes named below.

Classes: (a) pixel or element count of an image or of a buffer of image size; (b) byte count; (c) row offset (`stride * y`, below the image's byte count); (d) not a size count (squared lengths, w x w x h comparisons, histogram or kernel sizes): listed, not analysed further, because they do not measure memory and their range does not follow from this limit alone; (e) not reachable from a loaded source image.

Reference values for an accepted source image, computed by `build/scratch/int-audit/classify.py` over all integer sides 1..65535 with the pixel limit:

| Quantity | Maximum | Fits in 2147483647 |
|----------|---------|--------------------|
| pixels, w x h | 536870911 | yes |
| RGB32 bytes, 4 x w x h | 2147483644 | yes |
| padded grid, (w + 2) x (h + 2) (at w = 8193, h = 65528) | 537018350 | yes |
| integral image, (w + 1) x (h + 1) | 536944626 | yes |
| 32-bit words of a bilevel image, ceil(w / 32) x h | 16840696 (67362784 bytes) | yes |
| 8-bit grey rows, stride (width rounded up to 4) x h | 537067488 | yes |

### Classified table

| Location | Code | Class | Maximum for an accepted source image | Fits in 2147483647 | What it counts |
|----------|------|-------|--------------------------------------|--------------------|----------------|
| `src/core/Despeckle.cpp:333` | `dist.resize(width * height, Distance::zero());` | a | 537,018,350 | yes | (w+2) x (h+2) cells of the Voronoi distance grid |
| `src/core/Despeckle.cpp:707` | `components[unifiedBigComponent].numPixels = width * height;` | a | 536,870,911 | yes | pixel count of the despeckled image |
| `src/core/EstimateBackground.cpp:128` | `if (sum < 0.01 * (diff.width() * diff.height())) {` | a | 536,870,911 | yes | pixel count of the difference image, compared with a double |
| `src/core/ProjectPages.cpp:208` | `if (size.width() * dpi.height() > size.height() * dpi.width()) {` | d | - | - | page width times dpi, a ratio comparison; the dpi comes from the file header and is not bounded by the size limit |
| `src/core/TiffReader.cpp:304` | `TiffBuffer<uint32_t>(info.width * info.height).swap(tmpBuffer);` | a | 536,870,911 | yes | uint32 element count; bytes go through the 64-bit tsize_t (compile-checked) and the branch is dead for RGB32 |
| `src/core/filters/output/ColorPickupInteraction.cpp:122` | `bitmixedColors.reserve(width * height);` | e | - | - | screen grab around the pointer, not a loaded source image |
| `src/core/filters/output/OutputGenerator.cpp:1010` | `std::vector<char> patternData(static_cast<unsigned long long int>(patternWidth * patternHeight), ' ');` | e | - | - | hit-and-miss pattern of a few pixels |
| `src/core/filters/select_content/ContentBoxFinder.cpp:60` | `return lhs.width() * lhs.width() * lhs.height() < rhs.width() * rhs.width() * rhs.height();` | d | - | - | w x w x h comparison of two rectangles |
| `src/core/filters/select_content/ContentBoxFinder.cpp:66` | `return lhs.width() * lhs.height() * lhs.height() < rhs.width() * rhs.height() * rhs.height();` | d | - | - | w x h x h comparison of two rectangles |
| `src/core/filters/select_content/ContentBoxFinder.cpp:192` | `if (ws.width() * ws.height() < areaThreshold) {` | a | 536,870,911 | yes | area of a rectangle inside the page |
| `src/core/filters/select_content/ContentBoxFinder.cpp:211` | `if (ws.width() * ws.height() < areaThreshold) {` | a | 536,870,911 | yes | area of a rectangle inside the page |
| `src/core/filters/select_content/ContentBoxFinder.cpp:243` | `if (ws.width() * ws.height() < areaThreshold) {` | a | 536,870,911 | yes | area of a rectangle inside the page |
| `src/core/filters/select_content/ContentBoxFinder.cpp:1080` | `while (removedArea.width() * removedArea.height() > 0.3 * (newArea.width() * newArea.height())) {` | a | 536,870,911 | yes | areas of two rectangles inside the page, compared with a double |
| `src/core/zones/ZoneContextMenuInteraction.cpp:22` | `const qreal lhsArea = lhsBbox.width() * lhsBbox.height();` | e | - | - | qreal (double) product |
| `src/core/zones/ZoneContextMenuInteraction.cpp:23` | `const qreal rhsArea = rhsBbox.width() * rhsBbox.height();` | e | - | - | qreal (double) product |
| `src/dewarping/DetectVertContentBounds.cpp:138` | `m_maxSegmentSqLen = (w * w + h * h) / 3;` | d | - | - | squared lengths w x w + h x h |
| `src/imageproc/Binarize.cpp:396` | `const int areaFull = w * h;` | a | 536,870,911 | yes | pixel count of the grey image |
| `src/imageproc/BinaryImage.cpp:70` | `m_data = SharedData::create(m_height * m_wpl);` | a | 16,840,696 | yes | 32-bit words of a bilevel image: height x ceil(width / 32); the allocation takes size_t |
| `src/imageproc/BinaryImage.cpp:79` | `m_data = SharedData::create(m_height * m_wpl);` | a | 16,840,696 | yes | same, QSize constructor |
| `src/imageproc/BinaryImage.cpp:88` | `m_data = SharedData::create(m_height * m_wpl);` | a | 16,840,696 | yes | same, with fill colour |
| `src/imageproc/BinaryImage.cpp:98` | `m_data = SharedData::create(m_height * m_wpl);` | a | 16,840,696 | yes | same, QSize with fill colour |
| `src/imageproc/BinaryImage.cpp:249` | `memset(data(), pattern, m_height * m_wpl * 4);` | b | 67,362,784 | yes | bytes of a whole bilevel image (memset) |
| `src/imageproc/BinaryImage.cpp:396` | `return r.width() * r.height() - countBlackPixels(r);` | a | 536,870,911 | yes | pixel count of a rectangle |
| `src/imageproc/BinaryImage.cpp:415` | `const uint32_t* line = data + h * wpl;` | c | 16,840,696 | yes | row offset in words |
| `src/imageproc/BinaryImage.cpp:570` | `memset(data + rect.y() * m_wpl, pattern, rect.height() * m_wpl * 4);` | b | 67,362,784 | yes | bytes of the filled rows (memset) |
| `src/imageproc/BinaryThreshold.cpp:142` | `srcLine += maxEdgeWidth * srcStride;` | c | 537,067,488 | yes | edge rows times the grey stride, at most the grey image bytes |
| `src/imageproc/BinaryThreshold.cpp:143` | `dilatedLine += maxEdgeWidth * dilatedStride;` | c | 537,067,488 | yes | edge rows times the dilated stride |
| `src/imageproc/ConnectivityMap.cpp:298` | `for (int todo = width * height; todo > 0; --todo) {` | a | 537,018,350 | yes | (w+2) x (h+2) labels of the padded map |
| `src/imageproc/FindPeaksGeneric.h:49` | `std::vector<T> mask(toBeRaised, toBeRaised + toBeRaisedStride * size.height());` | a | 537,067,488 | yes | stride x height elements of the copied mask (8-bit stride at most width + 3) |
| `src/imageproc/FindPeaksGeneric.h:111` | `std::vector<T> raised(size.width() * size.height());` | a | 536,870,911 | yes | pixel count of a work buffer |
| `src/imageproc/FindPeaksGeneric.h:155` | `std::vector<T> raised(size.width() * size.height());` | a | 536,870,911 | yes | pixel count of a work buffer |
| `src/imageproc/Grayscale.cpp:166` | `const int numPixels = width * height;` | a | 536,870,911 | yes | pixel count of the grey image |
| `src/imageproc/Grayscale.cpp:340` | `const int numBits0 = w * h - numBits1;` | a | 536,870,911 | yes | pixel count minus the black bits |
| `src/imageproc/HoughLineDetector.cpp:65` | `m_histogram.resize(m_histWidth * m_histHeight, 0);` | d | - | - | histogram bins times angles |
| `src/imageproc/InfluenceMap.cpp:68` | `m_data.resize(width * height);` | a | 537,018,350 | yes | (w+2) x (h+2) cells of the padded map |
| `src/imageproc/InfluenceMap.cpp:76` | `for (int i = width * height; i > 0; --i) {` | a | 537,018,350 | yes | same, loop counter |
| `src/imageproc/IntegralImage.h:97` | `m_data = new T[width * height];` | a | 536,944,626 | yes | (w+1) x (h+1) elements; the array allocation takes size_t (uint64 elements need 8 bytes each, no int byte count) |
| `src/imageproc/LocalMinMaxGeneric.h:260` | `std::vector<T> temp(inputSize.width() * inputSize.height());` | a | 536,870,911 | yes | pixel count of a work buffer |
| `src/imageproc/MaxWhitespaceFinder.cpp:16` | `const int lhsArea = lhs.width() * lhs.height();` | a | 536,870,911 | yes | area of a rectangle inside the image |
| `src/imageproc/MaxWhitespaceFinder.cpp:17` | `const int rhsArea = rhs.width() * rhs.height();` | a | 536,870,911 | yes | area of a rectangle inside the image |
| `src/imageproc/MaxWhitespaceFinder.cpp:271` | `if (m_integralImg.sum(outerRect) == unsigned(outerRect.width() * outerRect.height())) {` | a | 536,870,911 | yes | area of a rectangle inside the image |
| `src/imageproc/MaxWhitespaceFinder.cpp:302` | `const unsigned area = middleRect.width() * middleRect.height();` | a | 536,870,911 | yes | area of a rectangle inside the image |
| `src/imageproc/Morphology.cpp:990` | `const int patternLen = patternWidth * patternHeight;` | e | - | - | hit-and-miss pattern of a few pixels |
| `src/imageproc/PolynomialSurface.cpp:34` | `const int numDataPoints = src.width() * src.height();` | a | 536,870,911 | yes | pixel count of the sampled image (Pitfall 4) |
| `src/imageproc/Posterizer.cpp:221` | `const int pixelCount = image.width() * image.height();` | a | 536,870,911 | yes | pixel count of the image (Pitfall 4) |
| `src/imageproc/SavGolFilter.cpp:260` | `if (calcNumTerms(horDegree, vertDegree) > windowSize.width() * windowSize.height()) {` | d | - | - | kernel window size |
| `src/imageproc/SavGolKernel.cpp:28` | `m_numDataPoints(size.width() * size.height()) {` | d | - | - | kernel window size |

Counts: a = 30 b = 2 c = 3 d = 7 e = 5 total = 47 (a: pixel or element counts, b: byte counts, c: row offsets, d: not a size count, e: not reachable from a loaded source image). All 35 lines of the classes a, b and c have a maximum of at most 2147483647; the 7 (d) lines are listed without analysis.

### Two format facts

- The TIFF reader hands the pipeline at most 4 bytes per pixel: `Format_Mono`, `Format_Indexed8` (grey and palette), `Format_RGB32` and `Format_ARGB32` (`TiffReader.cpp:287`, `:384-388`).
- Qt may decode a 16-bit PNG into an 8-byte format (`Format_RGBA64`). Qt's own limit of 2048 MB admits such an image only up to 2^28 px, and `git grep -n -e RGBA64 -e RGBX64 -- src` prints nothing, so no code under `src` handles such a format itself; the audited products see it only through Qt's own `QImage` members, which return `qsizetype`.

### Verdict (A2)

Assumption A2 is **closed**: no (a), (b) or (c) line exceeds 2147483647 for an accepted source image; the largest is the 8-bit grey row offset at 537067488 and the largest RGB32 byte count is 2147483644. Allocations that take these counts pass them through `size_t` (`SharedData::create(size_t)`, `new T[n]`, `tsize_t` is 64-bit as the compiler confirmed with a `static_assert` on `sizeof(tsize_t) == 8`), so the `uint64_t` squared-value integral image of the windowed binarizers (`Binarize.cpp:48`, `:141`, `:375`) needs 8 bytes per element (4.3 GB for a 2^29 px page) without any `int` byte count; that is a memory question, not an overflow. The ceiling stays 536870911.

Observations for the (d) lines, not changed here (D-00a: no change on an output path, and none of them measures memory):

- `DetectVertContentBounds.cpp:138` computes `w * w + h * h` in `int`; it overflows for a side above 46340 px. Its caller `TextLineTracer` first scales the page to 200 dpi (`TextLineTracer::downscale`), so a side that large is reached only by a page that is that long at 200 dpi, or by a low-dpi source that is scaled up.
- `ContentBoxFinder.cpp:60` and `:66` compare `w * w * h` and `w * h * h` of rectangles in `int`; the finder works at 150 dpi (`xform150dpi`), so the same reasoning applies.
- `ProjectPages.cpp:208` multiplies a page width by a dpi value that comes from the file header; the dpi is not bounded by this limit. A forged header with an absurd dpi can overflow that comparison; the result only chooses between one and two logical pages.

None of these is a count of pixels or bytes; they are listed so a later hardening step can bound them on their own terms.

## Tests that pin SAFE-01

| Test | What it proves | Evidence |
|------|----------------|----------|
| `CoreImageSizeLimitTestSuite` (core_tests, 14 cases) | `exceeds` table, texts, TIFF side and total boundaries, forged mono, RGB, wide and 2^29 px RGB TIFFs refused without allocating (peak growth below 64 MiB, under 1000 ms), multi-page rule, largest accepted size fits `int` bytes, three JPEG metadata cases, `test_oversized_pages_text` | `No errors detected`, 14 "Leaving test case" lines (`build/scratch/core-s06.txt`) |
| `TestCoreQt::testForgedPngIsRefused`, `testLargeGreyPngLoads`, `testForgedJpegIsRefusedAndLargeJpegLoads`, `testForgedPngMetadataReportsClaimedSize` | PNG and JPEG follow the limit; a 72 MP grey PNG and JPEG load; the loader leaves Qt's limit at 2048; real 65535 x 1 loads while 65536 x 1 and 1 x 65536 are refused | PASS in `build/scratch/qt-s07.txt` |
| `TestUxQt::testOversizedImageShowsTooLargeInPageView` (rows tiff, png, png-wide, jpeg, garbage) | the page view says "too large to load" with the claimed size and the limit; an unreadable file keeps the old text | PASS |
| `TestUxQt::testProjectFilesDialogMarksOversizedFile` | the real dialog lists `forged.tif (too large: 100000 × 100000 px)` and `two-pages.tif (page 2 too large: 100000 × 100000 px)` in red, `normal.png` in green, shows the existing box with the limit line, does not accept; a control folder with one normal PNG accepts without a box | PASS |
| `TestUxQt::testOversizedThumbnailFails` | a thumbnail request for a forged 100000 x 100000 TIFF ends `LOAD_FAILED`, writes no file, grows the peak footprint by less than 64 MiB; a normal PNG gives `LOADED` and one file | PASS |

Mutation runs (never committed, files restored from byte copies):

- Plan 06-04: refusal in `TiffReader::readImage` disabled: `test_forged_mono_tiff_is_refused_without_allocating` fails on `image.isNull()` and on the footprint (1250526096 >= 67108864).
- Plan 06-05: `ImageLoader` check disabled: only the real 65536 x 1 grey PNG case discriminates (Qt's raised limit already refuses a forged 100000 x 100000 PNG); Qt limit not raised: the 72 MP cases fail; JPEG check disabled: footprint 432 MB with a reduced forgery; PNG limit not lifted: `forgedPng(2000000, 1)` gave `GENERIC_ERROR`.
- Plan 06-06: the `STATUS_TOO_LARGE` branch in `loadNextFile` disabled (`if (false && ...)`): `testProjectFilesDialogMarksOversizedFile` fails at `QTRY_VERIFY(!closer.texts().isEmpty())` (the forged files are accepted, no box). The refusal in `TiffReader::readImage` disabled: `testOversizedThumbnailFails` fails (`status` 0 = `LOADED`, expected `LOAD_FAILED`); the page-view test crashes in that run before the thumbnail test is reached, which is why the thumbnail slot was run alone for the mutation.

The insert flow of `MainWindow` cannot be driven offscreen (the native file panel), so it is covered by the shared function's case `test_oversized_pages_text`, the dialog test (same call shape) and the source assertion `grep -cF oversizedPagesText src/app/MainWindow.cpp` printing 1.

## Sanitizer run (D-05)

Commands (run at HEAD `05b21cf1`, whose `src/` equals the tested commit `aeeb0419`): `cmake --preset macos-asan`, `cmake --build --preset macos-asan --target core_tests qt_tests`, `ctest --preset macos-asan -R '^(core_tests|qt_tests)$' --output-on-failure` (`100% tests passed out of 2`, hides reports of passing tests), then `ctest --preset macos-asan -R '^(core_tests|qt_tests)$' -V` to read the reports (`build/scratch/asan-s07-ctest-v.txt`).

| Report | Count | Where | Classification |
|--------|-------|-------|----------------|
| `ERROR: AddressSanitizer` | 0 | - | - |
| out of memory | 0 | - | - |
| `runtime error:` | 2 (one in `core_tests`, one in `qt_tests`) | `src/imageproc/BinaryImage.cpp:1044:37`, member access within null pointer of type `SharedData` | older code: the known Phase 1 location listed in `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`; recorded, not fixed |

Under the sanitizers `CoreImageSizeLimitTestSuite` (14 cases) reports `No errors detected` and the SAFE-01 Qt tests pass: `testForgedPngIsRefused`, `testForgedJpegIsRefusedAndLargeJpegLoads`, `testLargeGreyPngLoads`, `testForgedPngMetadataReportsClaimedSize`, the five rows of `testOversizedImageShowsTooLargeInPageView`, `testProjectFilesDialogMarksOversizedFile` and `testOversizedThumbnailFails`. The forged headers never allocate their claimed size, so no out-of-memory report appears; the footprint tests are the proof, the sanitizers the second check (RESEARCH D-05).

## Gate rows

| Label | Commit | Subject | Result |
|-------|--------|---------|--------|
| s03-tiff-limit | dc8e8091 | fix(06-04): refuse TIFF pages whose header claims an oversized image | Release and Debug 100% tests passed out of 7, goldens unchanged |
| s04-tiff-limit-tests | b4cca806 | test(06-04): pin the image size limit for TIFF boundaries and multi-page files | same |
| s05-qt-formats | 1fb688fc | fix(06-05): apply the image size limit to PNG and JPEG and raise the Qt reader limit | same |
| s06-project-files | 752a869b | fix(06-06): mark oversized files in the project file list | same, command diff 0 / 0 |
| s07-insert-thumbs | aeeb0419 | fix(06-06): list oversized files as failed when inserting pages | same, command diff 0 / 0 |

The full rows with the command-diff notes are in `records/gate-log.md`.
