Asp images
==========

The image sources are SVG and can be edited using Inkscape, available at
https://inkscape.org. If modified in Inkscape, images should be saved as Plain
SVG format using the Save As function.

The following sections describe how to reproduce the other image files, some
of which are committed into the repository for convenience.

Creating the icon
-----------------

The source for the icon file is asp.svg. It's natural size is 256x256. To
create an ICO file, the SVG file must first be converted to PNG, and then the
PNG file can be converted to an ICO file.

1. Open asp.svg in Inkscape.

2. Use the export function to export to PNG using the default export settings:
   - Dimensions: 256x256 (same as natural image dimensions).
   - DPI: Use the default.
   - Interlacing: Disabled.
   - Bit depth: RGBA 8 (i.e., with transparency layer).

3. The exported PNG is useful for other purposes, so it is maintained in the
   repository. Commit any changes to asp.png.

4. Use the online service at  https://online-converting.com to convert the PNG
   file to an ICO file with the following options:
   - Sizes enabled: 16x16, 32x32, 48x48, 64x64, 128x128, and 256x256.
   - Depth color: 32 bit for image with transparency.

5. Commit any changes to asp.ico to the repository.

Creating the NSIS installer header image
----------------------------------------

The Nullsoft Scriptable Install System (NSIS) uses both an icon and an
installer header image bitmap when building the package installer. Generating
the icon file was explained above. This section explains how to generate the
header image bitmap.

The header image bitmap is a BMP file. The recommended dimensions of this BMP
file are 150x57. The source SVG file is asp-wide.svg.

1. Open asp-wide.svg in Inkscape. It's natural size is 768x256.

2. Using the document properties, change the image size to 741x282. Do not
   save the changes.

3. Select all objects (ctrl-A) and form them into a single group (ctrl-G).
   Then, using the object align and distribute function, centre the group
   object relative to the page. If desired, this image may be saved to
   asp-nsis.svg, but it is not normally committed to the repository.

4. Use the export function to export to PNG as asp-nsis.png using the
   following settings:
   - Dimensions: 150x57 (same aspect ratio as 741x282).
   - DPI: Use the default.
   - Interlacing: Disabled.
   - Bit depth: RGBA 8 (i.e., with transparency layer).

5. Open the exported PNG file, asp-nsis.png, in Microsoft Paint.

6. Save the image as a BMP file with the name asp-nsis.bmp. You should receive
   a warning that any transparency will be lost. This is expected, as BMP
   files do not support a transparency layer or alpha channel.

7. Commit any changes to asp-nsis.bmp to the repository.
