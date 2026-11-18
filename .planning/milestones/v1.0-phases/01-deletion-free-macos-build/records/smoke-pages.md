# Smoke pages (plan 01-05 Task 1, D-04)

Purpose: three throwaway synthetic pages for the BUILD-03 smoke run (plan 01-06). They are never committed and contain no personal scan; Phase 2 builds the real seeded generator.

Location: `{scratch}/smoke/pages/` (session scratchpad). Empty output directory: `{scratch}/smoke/out/`. Tools: `magick` (ImageMagick 7), no fonts, no random generator. The text is rows of black rectangles, 28 px tall, 14 px gaps, row pitch 70 px, widths 30, 40, ... 140 px repeating.

## Commands as run

The script `{scratch}/smoke/gen-pages.sh` was run as `bash {scratch}/smoke/gen-pages.sh {scratch}/smoke/pages`. Its full content:

```bash
#!/bin/bash
# Throwaway smoke pages (D-04). Deterministic: no random generator, no fonts.
set -eu
OUT="${1:?output directory}"
mkdir -p "$OUT"

# rows X0 X1 Y0 N : N rows of 28 px tall black rectangles, pitch 70 px, 14 px gaps,
# widths 30,40,...,140 repeating (30 + 10*((k+row) mod 12)), clipped at X1.
rows() {
  awk -v x0="$1" -v x1="$2" -v y0="$3" -v n="$4" 'BEGIN {
    for (r = 0; r < n; r++) {
      y = y0 + r * 70; x = x0; k = r
      while (x < x1) {
        w = 30 + 10 * (k % 12); xe = x + w - 1; if (xe > x1) xe = x1
        printf "rectangle %d,%d %d,%d ", x, y, xe, y + 27
        x = xe + 1 + 14; k++
      }
    }
  }'
}
FIN=(-density 300 -units PixelsPerInch -depth 8 -compress LZW)

# 01: flat text, A4 at 300 DPI, deskew target (1.5 degrees)
magick -size 2480x3508 xc:white -fill black -draw "$(rows 250 2229 250 36)" \
  -background white -rotate 1.5 -gravity center -crop 2480x3508+0+0 +repage \
  "${FIN[@]}" "$OUT/01-flat-text.tif"

# 02: book spread, grey gutter band, wave-curved rows
magick -size 4960x3508 xc:white -fill '#808080' -draw "rectangle 2450,0 2509,3507" \
  -fill black -draw "$(rows 250 2249 250 43)" -draw "$(rows 2710 4709 250 43)" \
  -background white -wave 30x9920 -gravity center -extent 4960x3508 +repage \
  "${FIN[@]}" "$OUT/02-book-spread.tif"

# 03: mixed text and colour picture
magick -size 2480x3508 xc:white -fill black \
  -draw "$(rows 250 2229 100 20)" -draw "$(rows 250 2229 2830 10)" \
  \( -size 1400x1000 gradient:'#e02020-#2030e0' \
     \( -size 1400x1000 radial-gradient:'#ffe030-#10a050' \) \
     -compose blend -define compose:args=50 -composite \) \
  -compose over -geometry +540+1760 -composite \
  "${FIN[@]}" "$OUT/03-mixed.tif"
```

Page 03 geometry: the plan's 10 text rows below the picture do not fit under a 1400 by 1000 px block that is centred in the lower half (about 380 px remain below it). The block is therefore placed at y = 1760..2759, just inside the lower half and centred horizontally, and the 10 rows start at y = 2830.

## identify output

```
01-flat-text.tif 2480x3508 300 300 PixelsPerInch Gray depth=8
02-book-spread.tif 4960x3508 300 300 PixelsPerInch Gray depth=8
03-mixed.tif 2480x3508 300 300 PixelsPerInch sRGB depth=8
```

## SHA-256

```
2d47f7364b7c087e202076096848a7f4bf309aae002b8052fd8f46cabfea2c8f  01-flat-text.tif
de3b20d497c2802f716fd1c1cd5ee430ab2855ea7aeee2ed944f170f05f2a416  02-book-spread.tif
d7876136fd3c39e212bd932702765a97f64a649b9bd737013f2bba07dc8c5d7b  03-mixed.tif
```

The script was run twice into separate directories; both runs gave identical checksums, so the pages are reproducible byte for byte with this ImageMagick version.

## Regenerate

If the scratchpad no longer holds the pages, plan 01-06 reruns `bash {scratch}/smoke/gen-pages.sh {scratch}/smoke/pages` (the script content is in this file; recreate it from the block above if the scratchpad is gone) and compares the `shasum -a 256` output with the table above. The density (`-density 300 -units PixelsPerInch`) is in every file so ScanTailor does not ask for a DPI (research A8).
