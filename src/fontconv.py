import sys

from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *

def char_range(chFrom, chFromNot, inclusive=False):
    return map(chr, range(chFrom, chFromNot + (1 if inclusive else 0)))

def font_export(of, export_name, family, size, weight=QFont.Weight.Normal, italic=False, charset=None, antialias=False, antialias_bpp=4, extra_chars=""):
    if charset is None:
        charset = char_range(33, 127)
    charset = list(charset) + list(extra_chars)
    font = QFont(family, int(size), weight, italic)
    font.setPixelSize(size)
    if not antialias:
        font.setStyleStrategy(QFont.StyleStrategy.NoAntialias)
    font.setHintingPreference(QFont.HintingPreference.PreferFullHinting)
    font.setFixedPitch(True)
    metrics = QFontMetrics(font)
    charDefs = []
    totalWidth = 0
    maxHeight = 0
    minWidth = 1000
    maxWidth = 0
    firstChar = None
    charCount = 0
    for ch in charset:
        if firstChar is None:
            firstChar = ord(ch)
        charCount += 1
        size = metrics.size(0, ch)
        #print (size, metrics.height(), metrics.ascent())
        if antialias:
            pixmap = QPixmap(size)
        else:
            pixmap = QBitmap(size)
        pixmap.fill(QColor(0, 0, 0))
        painter = QPainter(pixmap)
        painter.setPen(QPen(QColor(255, 255, 255)))
        painter.setFont(font)
        painter.setRenderHint(QPainter.RenderHint.TextAntialiasing, antialias)
        painter.drawText(QRectF(0, 0, 128, 128), Qt.AlignTop | Qt.AlignLeft, ch)
        painter = None
        charDefs.append((ch, size, pixmap))
        totalWidth += size.width()
        minWidth = min(minWidth, size.width())
        maxWidth = max(maxWidth, size.width())
        maxHeight = max(maxHeight, size.height())
    if antialias:
        overallPixmap = QPixmap(totalWidth, maxHeight)
    else:
        overallPixmap = QBitmap(totalWidth, maxHeight)
    overallPixmap.fill(QColor(0, 0, 0))
    painter = QPainter(overallPixmap)
    x = 0
    nbytes = 0
    font_bits = ""
    font_widths = ""
    font_xs = ""
    for ch, size, pixmap in charDefs:
        img = pixmap.toImage()
        # font_bits += f"// Character: '{ch}'\n"
        font_xs += f"{x}, // '{ch}'\n"
        font_widths += f"{size.width()}, // '{ch}'\n"
        painter.drawPixmap(x, 0, pixmap)
        x += size.width()
    font_xs += f"{x}, // end\n"
    painter = None
    bpp = antialias_bpp if antialias else 1
    print (f"{x} x {maxHeight}, {minWidth}..{maxWidth}")
    img = overallPixmap.toImage().convertToFormat(QImage.Format_Grayscale8)
    img.save(f"{export_name}.png")
    size = overallPixmap.size()

    for iy in range(maxHeight):
        if antialias:
            if antialias_bpp == 8:
                for ix in range(0, totalWidth):
                    if ix < size.width() and iy < size.height():
                        byteValue = img.pixelColor(ix, iy).value()
                    else:
                        byteValue = 0
                    font_bits += f"0x{byteValue:02x}, "
                    nbytes += 1
            else:
                for ix in range(0, totalWidth, 2):
                    byteValue = 0
                    for ix2 in range(ix, min(ix + 2, totalWidth)):
                        if ix2 < size.width() and iy < size.height():
                            byteValue |= (img.pixelColor(ix2, iy).value() >> 4) << (4 - 4 * (ix2 - ix))
                    font_bits += f"0x{byteValue:02x}, "
                    nbytes += 1
        else:
            for ix in range(0, totalWidth, 8):
                byteValue = 0
                for ix2 in range(ix, min(ix + 8, maxWidth)):
                    if ix2 < size.width() and iy < size.height() and img.pixelColor(ix2, iy).value() > 127:
                        byteValue |= 128 >> (ix2 - ix)
                font_bits += f"0x{byteValue:02x}, "
                nbytes += 1
        font_bits += "\n"

    bitmap_type = "Gray4Bitmap" if antialias else "MonochromeBitmap"
    of.write(f"""\
const uint16_t {export_name}_xs[] = {"{"}
{font_xs}
{"}"};

const uint8_t {export_name}_bits[] = {"{"}
{font_bits}
{"}"};

{bitmap_type} {export_name}_bitmap({export_name}_bits, {size.width()}U, {size.height()}U);

Font {export_name} = {"{"}
    .charWidth={maxWidth}, .charHeight={maxHeight},
    .firstChar={firstChar}, .charCount={charCount},
    .spaceWidth={metrics.size(0, ' ').width()}, .bitsPerPixel={bpp},
    .xs={export_name}_xs,
    .bitmap=&{export_name}_bitmap, // {nbytes} bytes
{"}"};
""")
    return nbytes
    
nbytes = 0
app = QGuiApplication(sys.argv)
with open("bmpfont.cpp", "w") as of:
    of.write("""\
#include "bmpfont.h"

""")
    symbols = "⌫➕➖❌➗ⁿ√½↲🖩⭡❉⟋"
    nbytes += font_export(of, 'font_lato', 'Lato', 30, antialias=True)
    #nbytes += font_export(of, 'font_serif', 'Nimbus Roman', 25, antialias=True)
    nbytes += font_export(of, 'font_sans', 'Liberation Sans', 25, antialias=True, extra_chars=symbols)
    nbytes += font_export(of, 'font_small', 'Liberation Sans', 22, antialias=True, extra_chars=symbols)
    nbytes += font_export(of, 'font_small_bold', 'Liberation Sans', 22, weight=QFont.Weight.Bold, antialias=True, extra_chars=symbols)

print (f"Total {nbytes} bytes")
