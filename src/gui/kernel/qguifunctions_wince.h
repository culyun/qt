/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QGUIFUNCTIONS_WCE_H
#define QGUIFUNCTIONS_WCE_H
#ifdef Q_OS_WINCE
#include <QtCore/qfunctions_wince.h>
#define UNDER_NT
#include <wingdi.h>

#ifdef QT_BUILD_GUI_LIB
QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE
QT_MODULE(Gui)
QT_END_NAMESPACE
QT_END_HEADER
#endif

// application defines
#define SPI_SETNONCLIENTMETRICS 72
#define SPI_SETICONTITLELOGFONT 0x0022
#define WM_ACTIVATEAPP 0x001c
#define SW_PARENTCLOSING    1
#define SW_OTHERMAXIMIZED   2
#define SW_PARENTOPENING    3
#define SW_OTHERRESTORED    4
#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))

// drag n drop
#ifndef CFSTR_PERFORMEDDROPEFFECT
#define CFSTR_PERFORMEDDROPEFFECT TEXT("Performed DropEffect")
#endif
int qt_wince_GetDIBits(HDC, HBITMAP, uint, uint, void*, LPBITMAPINFO, uint);
#define GetDIBits(a,b,c,d,e,f,g) qt_wince_GetDIBits(a,b,c,d,e,f,g)

// QWidget
#define SW_SHOWMINIMIZED SW_MINIMIZE

// QRegion
#define ALTERNATE 0
#define WINDING 1

// QFontEngine
typedef struct _FIXED {
  WORD  fract;
  short value;
} FIXED;

typedef struct tagPOINTFX {
  FIXED x;
  FIXED y;
} POINTFX;

typedef struct _MAT2 {
  FIXED eM11;
  FIXED eM12;
  FIXED eM21;
  FIXED eM22;
} MAT2;

typedef struct _GLYPHMETRICS {
    UINT    gmBlackBoxX;
    UINT    gmBlackBoxY;
    POINT   gmptGlyphOrigin;
    short   gmCellIncX;
    short   gmCellIncY;
} GLYPHMETRICS;

typedef struct tagTTPOLYGONHEADER
{
    DWORD   cb;
    DWORD   dwType;
    POINTFX pfxStart;
} TTPOLYGONHEADER;

typedef struct tagTTPOLYCURVE
{
    WORD    wType;
    WORD    cpfx;
    POINTFX apfx[1];
} TTPOLYCURVE;

#define GGO_NATIVE 2
#define GGO_GLYPH_INDEX 0x0080
#define TT_PRIM_LINE 1
#define TT_PRIM_QSPLINE 2
#define TT_PRIM_CSPLINE 3
#define ANSI_VAR_FONT 12

#ifndef DEVICE_FONTTYPE
#define DEVICE_FONTTYPE 0x0002
#endif

#ifndef RASTER_FONTTYPE
#define RASTER_FONTTYPE 0x0001
#endif

HINSTANCE qt_wince_ShellExecute(HWND hwnd, LPCWSTR operation, LPCWSTR file, LPCWSTR params, LPCWSTR dir, int showCmd);
#define ShellExecute(a,b,c,d,e,f) qt_wince_ShellExecute(a,b,c,d,e,f)


// Clipboard --------------------------------------------------------
#define WM_CHANGECBCHAIN	1
#define WM_DRAWCLIPBOARD	2

BOOL qt_wince_ChangeClipboardChain(
    HWND hWndRemove,  // handle to window to remove
    HWND hWndNewNext  // handle to next window
);
#define ChangeClipboardChain(a,b) qt_wince_ChangeClipboardChain(a,b);

HWND qt_wince_SetClipboardViewer(
    HWND hWndNewViewer   // handle to clipboard viewer window
);
#define SetClipboardViewer(a) qt_wince_SetClipboardViewer(a)

// Graphics ---------------------------------------------------------
COLORREF qt_wince_PALETTEINDEX( WORD wPaletteIndex );
#define PALETTEINDEX(a) qt_wince_PALETTEINDEX(a)

#endif // Q_OS_WINCE
#endif // QGUIFUNCTIONS_WCE_H
