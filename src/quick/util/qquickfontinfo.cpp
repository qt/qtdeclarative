// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfontinfo_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype FontInfo
    \instantiates QQuickFontInfo
    \inqmlmodule QtQuick
    \since 6.9
    \ingroup qtquick-text-utility
    \brief Provides info about how a given font query is resolved.

    FontInfo provides information about the actual font which is matched for a font query by Qt's
    font selection system.

    It corresponds to the class QFontInfo in C++.

    \code
    FontInfo {
        id: fontInfo
        font.family: "Arial"
    }

    Text {
        text: fontInfo.family === "Arial"
            ? "System has 'Arial' font"
            : "System does not have 'Arial' font"
    }
    \endcode

    \sa FontMetrics, TextMetrics
*/
QQuickFontInfo::QQuickFontInfo(QObject *parent)
    : QObject(parent)
    , m_info(m_font)
{
}

/*!
    \qmlproperty font QtQuick::FontInfo::font

    This property holds the font which will be resolved by the FontInfo.
*/
QFont QQuickFontInfo::font() const
{
    return m_font;
}

void QQuickFontInfo::setFont(QFont font)
{
    if (m_font != font) {
        m_font = font;
        m_info = QFontInfo(m_font);
        emit fontChanged();
    }
}

/*!
    \qmlproperty string QtQuick::FontInfo::family

    This property holds the family of the matched font.

    \sa {QFontInfo::family()}
*/
QString QQuickFontInfo::family() const
{
    return m_info.family();
}

/*!
    \qmlproperty real QtQuick::FontInfo::styleName

    This property holds the style name (or "sub-family") of the mathed font.

    \sa {QFontInfo::styleName()}
*/
QString QQuickFontInfo::styleName() const
{
    return m_info.styleName();
}

/*!
    \qmlproperty int QtQuick::FontInfo::pixelSize

    This property holds the pixel size of the matched font.

    \sa {QFontInfo::pixelSize()}
*/
int QQuickFontInfo::pixelSize() const
{
    return m_info.pixelSize();
}

/*!
    \qmlproperty real QtQuick::FontInfo::pointSize

    This property holds the point size of the matched font.

    \sa {QFontInfo::pointSizeF()}
*/
qreal QQuickFontInfo::pointSize() const
{
    return m_info.pointSizeF();
}

/*!
    \qmlproperty bool QtQuick::FontInfo::italic

    This property holds the italic value of the matched font.

    \sa {QFontInfo::italic()}
*/
bool QQuickFontInfo::italic() const
{
    return m_info.italic();
}

/*!
    \qmlproperty int QtQuick::FontInfo::weight

    This property holds the weight of the matched font.

    \sa {QFontInfo::weight()}
*/
int QQuickFontInfo::weight() const
{
    return m_info.weight();
}

/*!
    \qmlproperty bool QtQuick::FontInfo::bold

    This property is true if weight() would return a value greater than Font.Normal; otherwise
    returns false.

    \sa weight(), {QFontInfo::bold()}
*/
bool QQuickFontInfo::bold() const
{
    return m_info.bold();
}

/*!
    \qmlproperty bool QtQuick::FontInfo::fixedPitch

    This property holds the fixed pitch value of the matched font.

    \sa {QFontInfo::fixedPitch()}
*/
bool QQuickFontInfo::fixedPitch() const
{
    return m_info.fixedPitch();
}

/*!
    \qmlproperty enum QtQuick::FontInfo::style

    This property holds the style of the matched font.

    \value Font.StyleNormal     Contains normal glyphs without italic slant.
    \value Font.StyleItalic     Contains glyphs designed to be used for representing italicized
                                text.
    \value Font.StyleOblique    Contains glyphs with an italic appearance, typically not
                                specially designed, but rather produced by applying a slant on the
                                font family's normal glyphs.

    \sa {QFontInfo::style()}
*/
QQuickFontEnums::Style QQuickFontInfo::style() const
{
    return QQuickFontEnums::Style(m_info.style());
}

/*!
    \qmlproperty list QtQuick::FontInfo::variableAxes

    This property holds the variable axes supported by the matched font. The list consists of
    QFontVariableAxis objects, which have the properties \c{tag}, \c{name}, \c{minimumValue},
    \c{maximumValue}, and \c{defaultValue}.

    \sa {QFontInfo::variableAxes()}, QFontVariableAxis
*/
QList<QFontVariableAxis> QQuickFontInfo::variableAxes() const
{
    return m_info.variableAxes();
}

QT_END_NAMESPACE
