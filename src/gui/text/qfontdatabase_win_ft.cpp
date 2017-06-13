/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qt_windows.h"
#include <qmath.h>
#include <private/qapplication_p.h>
#include "qfont_p.h"
#include "qfontengine_p.h"
#include "qpaintdevice.h"
#include <private/qsystemlibrary_p.h>
#include "qabstractfileengine.h"
#include "qendian.h"

#ifndef QT_NO_DEBUG
#  include <QtDebug>
#endif

#ifdef Q_WS_WINCE
#  include "qguifunctions_wince.h"
#  include <wchar.h>
#endif

#include <QDirIterator>

QT_BEGIN_NAMESPACE

#ifdef MAKE_TAG
#undef MAKE_TAG
#endif
// GetFontData expects the tags in little endian ;(
#define MAKE_TAG(ch1, ch2, ch3, ch4) (\
    (((quint32)(ch4)) << 24) | \
    (((quint32)(ch3)) << 16) | \
    (((quint32)(ch2)) << 8) | \
    ((quint32)(ch1)) \
    )

static bool localizedName(const QString &name)
{
    const QChar *c = name.unicode();
    for (int i = 0; i < name.length(); ++i) {
        if (c[i].unicode() >= 0x100)
            return true;
    }
    return false;
}

static inline quint16 getUShort(const unsigned char *p)
{
    quint16 val;
    val = *p++ << 8;
    val |= *p;

    return val;
}

static QString getEnglishName(const uchar *table, quint32 bytes)
{
    QString i18n_name;
    enum {
        NameRecordSize = 12,
        FamilyId = 1,
        MS_LangIdEnglish = 0x009
    };

    // get the name table
    quint16 count;
    quint16 string_offset;
    const unsigned char *names;

    int microsoft_id = -1;
    int apple_id = -1;
    int unicode_id = -1;

    if (getUShort(table) != 0)
        goto error;

    count = getUShort(table+2);
    string_offset = getUShort(table+4);
    names = table + 6;

    if (string_offset >= bytes || 6 + count*NameRecordSize > string_offset)
        goto error;

    for (int i = 0; i < count; ++i) {
        // search for the correct name entry

        quint16 platform_id = getUShort(names + i*NameRecordSize);
        quint16 encoding_id = getUShort(names + 2 + i*NameRecordSize);
        quint16 language_id = getUShort(names + 4 + i*NameRecordSize);
        quint16 name_id = getUShort(names + 6 + i*NameRecordSize);

        if (name_id != FamilyId)
            continue;

        enum {
            PlatformId_Unicode = 0,
            PlatformId_Apple = 1,
            PlatformId_Microsoft = 3
        };

        quint16 length = getUShort(names + 8 + i*NameRecordSize);
        quint16 offset = getUShort(names + 10 + i*NameRecordSize);
        if (DWORD(string_offset + offset + length) >= bytes)
            continue;

        if ((platform_id == PlatformId_Microsoft
            && (encoding_id == 0 || encoding_id == 1))
            && (language_id & 0x3ff) == MS_LangIdEnglish
            && microsoft_id == -1)
            microsoft_id = i;
        // not sure if encoding id 4 for Unicode is utf16 or ucs4...
        else if (platform_id == PlatformId_Unicode && encoding_id < 4 && unicode_id == -1)
            unicode_id = i;
        else if (platform_id == PlatformId_Apple && encoding_id == 0 && language_id == 0)
            apple_id = i;
    }
    {
        bool unicode = false;
        int id = -1;
        if (microsoft_id != -1) {
            id = microsoft_id;
            unicode = true;
        } else if (apple_id != -1) {
            id = apple_id;
            unicode = false;
        } else if (unicode_id != -1) {
            id = unicode_id;
            unicode = true;
        }
        if (id != -1) {
            quint16 length = getUShort(names + 8 + id*NameRecordSize);
            quint16 offset = getUShort(names + 10 + id*NameRecordSize);
            if (unicode) {
                // utf16

                length /= 2;
                i18n_name.resize(length);
                QChar *uc = (QChar *) i18n_name.unicode();
                const unsigned char *string = table + string_offset + offset;
                for (int i = 0; i < length; ++i)
                    uc[i] = getUShort(string + 2*i);
            } else {
                // Apple Roman

                i18n_name.resize(length);
                QChar *uc = (QChar *) i18n_name.unicode();
                const unsigned char *string = table + string_offset + offset;
                for (int i = 0; i < length; ++i)
                    uc[i] = QLatin1Char(string[i]);
            }
        }
    }
error:
    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

static QString getEnglishName(const QString &familyName)
{
    QString i18n_name;

    HDC hdc = GetDC( 0 );
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    memcpy(lf.lfFaceName, familyName.utf16(), qMin(LF_FACESIZE, familyName.length()) * sizeof(wchar_t));
    lf.lfCharSet = DEFAULT_CHARSET;
    HFONT hfont = CreateFontIndirect(&lf);

    if (!hfont) {
        ReleaseDC(0, hdc);
        return QString();
    }

    HGDIOBJ oldobj = SelectObject( hdc, hfont );

    const DWORD name_tag = MAKE_TAG( 'n', 'a', 'm', 'e' );

    // get the name table
    unsigned char *table = 0;

    DWORD bytes = GetFontData( hdc, name_tag, 0, 0, 0 );
    if ( bytes == GDI_ERROR ) {
        // ### Unused variable
        // int err = GetLastError();
        goto error;
    }

    table = new unsigned char[bytes];
    GetFontData(hdc, name_tag, 0, table, bytes);
    if ( bytes == GDI_ERROR )
        goto error;

    i18n_name = getEnglishName(table, bytes);
error:
    delete [] table;
    SelectObject( hdc, oldobj );
    DeleteObject( hfont );
    ReleaseDC( 0, hdc );

    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

static inline QFontDatabase::WritingSystem writingSystemFromCharSet(uchar charSet)
{
    switch (charSet) {
    case ANSI_CHARSET:
    case EASTEUROPE_CHARSET:
    case BALTIC_CHARSET:
    case TURKISH_CHARSET:
        return QFontDatabase::Latin;
    case GREEK_CHARSET:
        return QFontDatabase::Greek;
    case RUSSIAN_CHARSET:
        return QFontDatabase::Cyrillic;
    case HEBREW_CHARSET:
        return QFontDatabase::Hebrew;
    case ARABIC_CHARSET:
        return QFontDatabase::Arabic;
    case THAI_CHARSET:
        return QFontDatabase::Thai;
    case GB2312_CHARSET:
        return QFontDatabase::SimplifiedChinese;
    case CHINESEBIG5_CHARSET:
        return QFontDatabase::TraditionalChinese;
    case SHIFTJIS_CHARSET:
        return QFontDatabase::Japanese;
    case HANGUL_CHARSET:
    case JOHAB_CHARSET:
        return QFontDatabase::Korean;
    case VIETNAMESE_CHARSET:
        return QFontDatabase::Vietnamese;
    case SYMBOL_CHARSET:
        return QFontDatabase::Symbol;
    default:
        break;
    }
    return QFontDatabase::Any;
}

typedef struct {
    quint16 majorVersion;
    quint16 minorVersion;
    quint16 numTables;
    quint16 searchRange;
    quint16 entrySelector;
    quint16 rangeShift;
} OFFSET_TABLE;

typedef struct {
    quint32 tag;
    quint32 checkSum;
    quint32 offset;
    quint32 length;
} TABLE_DIRECTORY;

typedef struct {
    quint16 fontSelector;
    quint16 nrCount;
    quint16 storageOffset;
} NAME_TABLE_HEADER;

typedef struct {
    quint16 platformID;
    quint16 encodingID;
    quint16 languageID;
    quint16 nameID;
    quint16 stringLength;
    quint16 stringOffset;
} NAME_RECORD;

static QString fontNameFromTTFile(const QString &filename)
{
    QFile f(filename);
    QString retVal;
    qint64 bytesRead;
    qint64 bytesToRead;

    if (f.open(QIODevice::ReadOnly)) {
        OFFSET_TABLE ttOffsetTable;
        bytesToRead = sizeof(OFFSET_TABLE);
        bytesRead = f.read((char*)&ttOffsetTable, bytesToRead);
        if (bytesToRead != bytesRead)
            return retVal;
        ttOffsetTable.numTables = qFromBigEndian(ttOffsetTable.numTables);
        ttOffsetTable.majorVersion = qFromBigEndian(ttOffsetTable.majorVersion);
        ttOffsetTable.minorVersion = qFromBigEndian(ttOffsetTable.minorVersion);

        if (ttOffsetTable.majorVersion != 1 || ttOffsetTable.minorVersion != 0)
            return retVal;

        TABLE_DIRECTORY tblDir;
        bool found = false;

        for (int i = 0; i < ttOffsetTable.numTables; i++) {
            bytesToRead = sizeof(TABLE_DIRECTORY);
            bytesRead = f.read((char*)&tblDir, bytesToRead);
            if (bytesToRead != bytesRead)
                return retVal;
            if (qFromBigEndian(tblDir.tag) == MAKE_TAG('n', 'a', 'm', 'e')) {
                found = true;
                tblDir.length = qFromBigEndian(tblDir.length);
                tblDir.offset = qFromBigEndian(tblDir.offset);
                break;
            }
        }

        if (found) {
            f.seek(tblDir.offset);
            NAME_TABLE_HEADER ttNTHeader;
            bytesToRead = sizeof(NAME_TABLE_HEADER);
            bytesRead = f.read((char*)&ttNTHeader, bytesToRead);
            if (bytesToRead != bytesRead)
                return retVal;
            ttNTHeader.nrCount = qFromBigEndian(ttNTHeader.nrCount);
            ttNTHeader.storageOffset = qFromBigEndian(ttNTHeader.storageOffset);
            NAME_RECORD ttRecord;
            found = false;

            for (int i = 0; i < ttNTHeader.nrCount; i++) {
                bytesToRead = sizeof(NAME_RECORD);
                bytesRead = f.read((char*)&ttRecord, bytesToRead);
                if (bytesToRead != bytesRead)
                    return retVal;
                ttRecord.nameID = qFromBigEndian(ttRecord.nameID);
                if (ttRecord.nameID == 1) {
                    ttRecord.stringLength = qFromBigEndian(ttRecord.stringLength);
                    ttRecord.stringOffset = qFromBigEndian(ttRecord.stringOffset);
                    int nPos = f.pos();
                    f.seek(tblDir.offset + ttRecord.stringOffset + ttNTHeader.storageOffset);

                    QByteArray nameByteArray = f.read(ttRecord.stringLength);
                    if (!nameByteArray.isEmpty()) {
                        if (ttRecord.encodingID == 256 || ttRecord.encodingID == 768) {
                            //This is UTF-16 in big endian
                            int stringLength = ttRecord.stringLength / 2;
                            retVal.resize(stringLength);
                            QChar *data = retVal.data();
                            const ushort *srcData = (const ushort *)nameByteArray.data();
                            for (int i = 0; i < stringLength; ++i)
                                data[i] = qFromBigEndian(srcData[i]);
                            return retVal;
                        } else if (ttRecord.encodingID == 0) {
                            //This is Latin1
                            retVal = QString::fromLatin1(nameByteArray);
                        } else {
                            qWarning("Could not retrieve Font name from file: %s", qPrintable(QDir::toNativeSeparators(filename)));
                        }
                        break;
                    }
                    f.seek(nPos);
                }
            }
        }
        f.close();
    }
    return retVal;
}

static bool addFontToDatabase(const QString &familyName, uchar charSet,
                              const TEXTMETRIC *textmetric,
                              const FONTSIGNATURE *signature,
                              int type)
{
    typedef QPair<QString, QStringList> FontKey;

    // the "@family" fonts are just the same as "family". Ignore them.
    if (familyName.isEmpty() || familyName.at(0) == QLatin1Char('@') || familyName.startsWith(QLatin1String("WST_")))
        return false;

    const int separatorPos = familyName.indexOf(QLatin1String("::"));
    const QString faceName = separatorPos != -1 ? familyName.left(separatorPos) : familyName;
    const QString fullName = separatorPos != -1 ? familyName.mid(separatorPos + 2) : QString();
    const NEWTEXTMETRIC *tm = (NEWTEXTMETRIC *)textmetric;
    const bool fixed = !(tm->tmPitchAndFamily & TMPF_FIXED_PITCH);
    const bool ttf = (tm->tmPitchAndFamily & TMPF_TRUETYPE);
    const bool scalable = tm->tmPitchAndFamily & (TMPF_VECTOR|TMPF_TRUETYPE);
    const int pixelSize = scalable ? 0 : tm->tmHeight;
    const QFont::Style style = tm->tmItalic ? QFont::StyleItalic : QFont::StyleNormal;
    const bool antialias = true;
    const QFont::Weight weight = weightFromInteger(tm->tmWeight);

#ifndef QT_NO_DEBUG
    qDebug() << __FUNCTION__ << ' ' << familyName << ' ' << charSet << " TTF=" << ttf <<
              QLatin1String((type & DEVICE_FONTTYPE) ? " DEVICE" : "") <<
              QLatin1String((type & RASTER_FONTTYPE) ? " RASTER" : "") <<
              QLatin1String((type & TRUETYPE_FONTTYPE) ? " TRUETYPE" : "") <<
              " scalable=" << scalable << " Size=" << pixelSize << " Style=" << style << " Weight=" << weight;
#endif

    QString englishName;

    if (ttf && localizedName(faceName))
        englishName = getEnglishName(faceName);

    QList<QFontDatabase::WritingSystem> writingSystems;
    if (type & TRUETYPE_FONTTYPE) {
        Q_ASSERT(signature);
        quint32 unicodeRange[4] = {
            signature->fsUsb[0], signature->fsUsb[1],
            signature->fsUsb[2], signature->fsUsb[3]
        };
        quint32 codePageRange[2] = {
            signature->fsCsb[0], signature->fsCsb[1]
        };
        writingSystems = qt_determine_writing_systems_from_truetype_bits(unicodeRange, codePageRange);

        // ### Hack to work around problem with Thai text on Windows 7. Segoe UI contains
        // the symbol for Baht, and Windows thus reports that it supports the Thai script.
        // Since it's the default UI font on this platform, most widgets will be unable to
        // display Thai text by default. As a temporary work around, we special case Segoe UI
        // and remove the Thai script from its list of supported writing systems.
        int thaiSegoeIdx = -1;

        for (int i = 0; i < writingSystems.count(); ++i) {
            if (writingSystems.at(i) != QFontDatabase::Thai) continue;
            if (faceName != QLatin1String("Segoe UI")) continue;
            thaiSegoeIdx = i;
            break;
        }

        if (thaiSegoeIdx != -1) writingSystems.removeAt(thaiSegoeIdx);

    } else {
        const QFontDatabase::WritingSystem ws = writingSystemFromCharSet(charSet);
        if (ws != QFontDatabase::Any)
            writingSystems.append(ws);
    }

#ifndef Q_OS_WINCE
    const QSettings fontRegistry(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"),
                                QSettings::NativeFormat);

    static QVector<FontKey> allFonts;
    if (allFonts.isEmpty()) {
        const QStringList allKeys = fontRegistry.allKeys();
        allFonts.reserve(allKeys.size());
        const QString trueType = QLatin1String("(TrueType)");
        const QRegExp sizeListMatch(QLatin1String("\\s(\\d+,)+\\d+"));
        foreach (const QString &key, allKeys) {
            QString realKey = key;
            realKey.remove(trueType);
            realKey.remove(sizeListMatch);
            QStringList fonts;
            const QStringList fontNames = realKey.trimmed().split(QLatin1Char('&'));
            foreach (const QString &fontName, fontNames)
                fonts.push_back(fontName.trimmed());
            allFonts.push_back(FontKey(key, fonts));
        }
    }

    QString value;
    int index = 0;
    for (int k = 0; k < allFonts.size(); ++k) {
        const FontKey &fontKey = allFonts.at(k);
        for (int i = 0; i < fontKey.second.length(); ++i) {
            const QString &font = fontKey.second.at(i);
            if (font == faceName || fullName == font || englishName == font) {
                value = fontRegistry.value(fontKey.first).toString();
                index = i;
                break;
            }
        }
        if (!value.isEmpty())
            break;
    }
#else
    QString value;
    int index = 0;

    static QHash<QString, QString> fontCache;

    if (fontCache.isEmpty()) {
        QSettings settings(QSettings::SystemScope, QLatin1String("Qt-Project"), QLatin1String("Qtbase"));
        settings.beginGroup(QLatin1String("CEFontCache"));

        foreach (const QString &fontName, settings.allKeys()) {
            const QString fontFileName = settings.value(fontName).toString();
            fontCache.insert(fontName, fontFileName);
        }

        settings.endGroup(); // CEFontCache
    }

    value = fontCache.value(faceName);

    //Fallback if we haven't cached the font yet or the font got removed/renamed iterate again over all fonts
    if (value.isEmpty() || !QFile::exists(value)) {
        QSettings settings(QSettings::SystemScope, QLatin1String("Qt-Project"), QLatin1String("Qtbase"));
        settings.beginGroup(QLatin1String("CEFontCache"));

        //empty the cache first, as it seems that it is dirty
        foreach (const QString &fontName, settings.allKeys())
            settings.remove(fontName);

        QDirIterator it(QLatin1String("/Windows"), QStringList(QLatin1String("*.ttf")), QDir::Files | QDir::Hidden | QDir::System);

        while (it.hasNext()) {
            const QString fontFile = it.next();
            const QString fontName = fontNameFromTTFile(fontFile);
            if (fontName.isEmpty())
                continue;
            fontCache.insert(fontName, fontFile);
            settings.setValue(fontName, fontFile);

            if (localizedName(fontName)) {
                QString englishFontName = getEnglishName(fontName);
                fontCache.insert(englishFontName, fontFile);
                settings.setValue(englishFontName, fontFile);
            }
        }

        value = fontCache.value(faceName);

        settings.endGroup(); // CEFontCache
    }
#endif

    if (value.isEmpty())
        return false;

    if (!QDir::isAbsolutePath(value))
#ifndef Q_OS_WINCE
        value.prepend(QFile::decodeName(qgetenv("windir") + "\\Fonts\\"));
#else
        value.prepend(QFile::decodeName("/Windows/"));
#endif

    QFontDatabasePrivate *db = privateDb();
    const QByteArray fileName = QFile::encodeName(value);

    db->addFont(faceName, "", weight, tm->tmItalic, pixelSize, fileName, index,
        antialias, writingSystems);

    // add fonts windows can generate for us:
    if (weight <= QFont::DemiBold)
        db->addFont(faceName, "", QFont::Bold, tm->tmItalic, pixelSize, fileName, index, antialias, writingSystems);

    if (!tm->tmItalic)
        db->addFont(faceName, "", weight, true, pixelSize, fileName, index, antialias, writingSystems);

    if (weight <= QFont::DemiBold && !tm->tmItalic)
        db->addFont(faceName, "", QFont::Bold, true, pixelSize, fileName, index, antialias, writingSystems);

    // Hack - Add fields not populated by addFont

    QtFontFamily *family = privateDb()->family(faceName, false); // Not super efficient, binary search

    Q_ASSERT(family); // We've just added the family so it should be there

    family->english_name = englishName;
    family->fixedPitch = fixed;

    return true;
}

#ifdef Q_OS_WINCE
static QByteArray getFntTable(HFONT hfont, uint tag)
{
    HDC hdc = GetDC(0);
    HGDIOBJ oldFont = SelectObject(hdc, hfont);
    quint32 t = qFromBigEndian<quint32>(tag);
    QByteArray buffer;

    DWORD length = GetFontData(hdc, t, 0, NULL, 0);
    if (length != GDI_ERROR) {
        buffer.resize(length);
        GetFontData(hdc, t, 0, reinterpret_cast<uchar *>(buffer.data()), length);
    }
    SelectObject(hdc, oldFont);
    return buffer;
}
#endif

static int QT_WIN_CALLBACK storeFont(ENUMLOGFONTEX* f, NEWTEXTMETRICEX *textmetric,
                                     int type, LPARAM unused)
{
    Q_UNUSED(unused);

    typedef QSet<QString> StringSet;
    const QString familyName = QString::fromWCharArray(f->elfLogFont.lfFaceName)
                               + QLatin1String("::")
                               + QString::fromWCharArray(f->elfFullName);
    const uchar charSet = f->elfLogFont.lfCharSet;

#ifndef Q_OS_WINCE
    const FONTSIGNATURE signature = textmetric->ntmFontSig;
#else
    FONTSIGNATURE signature;
    QByteArray table;

    if (type & TRUETYPE_FONTTYPE) {
        HFONT hfont = CreateFontIndirect(&f->elfLogFont);
        table = getFntTable(hfont, MAKE_TAG('O', 'S', '/', '2'));
        DeleteObject((HGDIOBJ)hfont);
    }

    if (table.length() >= 86) {
        // See also qfontdatabase_mac.cpp, offsets taken from OS/2 table in the TrueType spec
        uchar *tableData = reinterpret_cast<uchar *>(table.data());

        signature.fsUsb[0] = qFromBigEndian<quint32>(tableData + 42);
        signature.fsUsb[1] = qFromBigEndian<quint32>(tableData + 46);
        signature.fsUsb[2] = qFromBigEndian<quint32>(tableData + 50);
        signature.fsUsb[3] = qFromBigEndian<quint32>(tableData + 54);

        signature.fsCsb[0] = qFromBigEndian<quint32>(tableData + 78);
        signature.fsCsb[1] = qFromBigEndian<quint32>(tableData + 82);
    } else {
        memset(&signature, 0, sizeof(signature));
    }
#endif

    // NEWTEXTMETRICEX is a NEWTEXTMETRIC, which according to the documentation is
    // identical to a TEXTMETRIC except for the last four members, which we don't use
    // anyway
    bool addedFont = addFontToDatabase(familyName, charSet, (TEXTMETRIC *)textmetric, &signature, type);
#ifndef QT_NO_DEBUG
    qDebug() << QLatin1String(addedFont ? "Added - " : "Failed to add - ") << familyName;
#else
    addedFont;
#endif
    // keep on enumerating
    return 1;
}

/*!
    \brief Populate font database using EnumFontFamiliesEx().

    Normally, leaving the name empty should enumerate
    all fonts, however, system fonts like "MS Shell Dlg 2"
    are only found when specifying the name explicitly.
*/

static void populate(const QString &family)
{
    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    if (family.size() >= LF_FACESIZE) {
        qWarning("%s: Unable to enumerate family '%s'.",
                 __FUNCTION__, qPrintable(family));
        return;
    }
    wmemcpy(lf.lfFaceName, reinterpret_cast<const wchar_t*>(family.utf16()),
            family.size() + 1);
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, (FONTENUMPROC)storeFont, (LPARAM) 0, 0);
    ReleaseDC(0, dummy);
}

static void populateFontDatabase(QString const & sysFontFamily = QString())
{
    populate(QString());
    if (sysFontFamily.isEmpty()) return;

    // Work around EnumFontFamiliesEx() not listing the system font, see populate.
    QFontDatabasePrivate *db = privateDb();

    for (int f = 0; f < db->count; f++) {
        QtFontFamily *family = db->families[f];
        if (family->name == sysFontFamily) return;
    }

    populate(sysFontFamily);
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt)
{
    QFontDatabasePrivate *db = privateDb();
    fnt->families = db->addTTFile(QFile::encodeName(fnt->fileName), fnt->data);
    db->reregisterAppFonts = true;
}

static void initializeDb()
{
    QFontDatabasePrivate *db = privateDb();

    if (!db->count) populateFontDatabase(QApplication::font().family());

    if (db->reregisterAppFonts) {
        for (int i = 0; i < db->applicationFonts.count(); i++) {
            if (!db->applicationFonts.at(i).families.isEmpty())
                registerFont(&db->applicationFonts[i]);
        }
        db->reregisterAppFonts = false;
    }
}

static inline void load(const QString &family = QString(), int = -1)
{
    Q_UNUSED(family);
    initializeDb();
}

static const char *styleHint(const QFontDef &request)
{
    const char *stylehint = 0;
    switch (request.styleHint) {
    case QFont::SansSerif:
        stylehint = "Arial";
        break;
    case QFont::Serif:
        stylehint = "Times New Roman";
        break;
    case QFont::TypeWriter:
        stylehint = "Courier New";
        break;
    default:
        if (request.fixedPitch)
            stylehint = "Courier New";
        break;
    }
    return stylehint;
}

static QFontEngine *loadEngine(const QFontDef &request, const QtFontDesc &desc)
{
    QtFontFoundry *foundry = desc.foundry;
    QtFontStyle *style = desc.style;
    QtFontSize *size = desc.size;

    Q_ASSERT(size);

    if ( foundry->name == QLatin1String("qt") ) return 0; ///#### is this the best way????

    QString file = QFile::decodeName(size->fileName);
    if ((file.isEmpty() || !QFile::exists(file)) && !privateDb()->isApplicationFont(file)) return 0;

    int pixelSize = size->pixelSize;
    if (!pixelSize || (style->smoothScalable && pixelSize == SMOOTH_SCALABLE)) pixelSize = request.pixelSize;
    QFontDef def = request;
    def.pixelSize = pixelSize;

    QFontEngineFT * fte = new QFontEngineFT(def);

    QFontEngine::FaceId faceId;
    faceId.filename = file.toLocal8Bit();
    faceId.index = size->fileIndex;
    bool antialias = style->antialiased && !(request.styleStrategy & QFont::NoAntialias);

    if (!fte->init(faceId, antialias, antialias ? QFontEngineFT::Format_A8 : QFontEngineFT::Format_Mono)) {
        delete fte;
        return 0;
    }

    return fte;
}

void QFontDatabase::load(const QFontPrivate *d, int script)
{
    // sanity checks
    if (!qApp)
        qWarning("QFontDatabase::load: Must construct QApplication first");
    Q_ASSERT(script >= 0 && script < QUnicodeTables::ScriptCount);

    // normalize the request to get better caching
    QFontDef req = d->request;
    if (req.pixelSize <= 0)
        req.pixelSize = floor((100.0 * req.pointSize * d->dpi) / 72. + 0.5) / 100;
    if (req.pixelSize < 1)
        req.pixelSize = 1;
    if (req.weight == 0)
        req.weight = QFont::Normal;
    if (req.stretch == 0)
        req.stretch = 100;

    QFontCache::Key key(req, d->rawMode ? QUnicodeTables::Common : script, d->screen);
    if (!d->engineData)
        getEngineData(d, key);

    // the cached engineData could have already loaded the engine we want
    if (d->engineData->engines[script])
        return;

    QFontEngine *fe = QFontCache::instance()->findEngine(key);
    if (fe) {
      d->engineData->engines[script] = fe;
      fe->ref.ref();
      QFontCache::instance()->insertEngine(key, fe);
      return;
    }

    // set it to the actual pointsize, so QFontInfo will do the right thing
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72./d->dpi;

    ///////////////
    //
    // Initialise Test engine if we're specifically asked to do so.

    if (qt_enable_test_font && req.family == QLatin1String("__Qt__Box__Engine__")) {
        fe = new QTestFontEngine(req.pixelSize);
        fe->fontDef = req;
        d->engineData->engines[script] = fe;
        fe->ref.ref();
        QFontCache::instance()->insertEngine(key, fe);
        return;
    }

    ///////////////
    //
    // Initialise Freetype engine with a suitable initial font

    // Pull together a list of possible font families to try

    QStringList family_list = familyList(req);

    const char *stylehint = styleHint(req);
    if (stylehint) family_list << QLatin1String(stylehint);

    // add the default family
    QString defaultFamily = QApplication::font().family();
    if (!family_list.contains(defaultFamily)) family_list << defaultFamily;

    // add QFont::defaultFamily() to the list, for compatibility with previous versions
    family_list << QApplication::font().defaultFamily();

    // null family means find the first font matching the specified script
    family_list << QString();

    QtFontDesc desc;
    QList<int> blacklistedFamilies;

    {
        QMutexLocker locker(fontDatabaseMutex());

        if (!privateDb()->count) initializeDb();

        QStringList::ConstIterator it = family_list.constBegin();
        QStringList::ConstIterator end = family_list.constEnd();

        for (; !fe && it != end; ++it) {
            req.family = *it;
            QString family_name;
            QString foundry_name;
            parseFontName(req.family, foundry_name, family_name);

            const int force_encoding_id = -1;

            match(script, req, family_name, foundry_name, force_encoding_id, &desc, blacklistedFamilies);
            if (!desc.family || !desc.foundry || !desc.style) continue;

            fe = loadEngine(req, desc);
            if (!fe) blacklistedFamilies.append(desc.familyIndex);
        }
    }

    if (fe) {
        d->engineData->engines[script] = fe;
        fe->ref.ref();
        QFontCache::instance()->insertEngine(key, fe);
        return;
    }

    ///////////////
    //
    // Initialise fallback font engine

    fe = new QFontEngineBox(req.pixelSize);
    fe->fontDef = QFontDef();
    d->engineData->engines[script] = fe;
    fe->ref.ref();
    QFontCache::instance()->insertEngine(key, fe);
}

bool QFontDatabase::removeApplicationFont(int handle)
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (handle < 0 || handle >= db->applicationFonts.count())
        return false;

    db->applicationFonts[handle] = QFontDatabasePrivate::ApplicationFont();

    db->reregisterAppFonts = true;
    db->invalidate();
    return true;
}

bool QFontDatabase::removeAllApplicationFonts()
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (db->applicationFonts.isEmpty())
        return false;

    db->applicationFonts.clear();
    db->invalidate();
    return true;
}

bool QFontDatabase::supportsThreadedFontRendering()
{
    return true;
}

QT_END_NAMESPACE
