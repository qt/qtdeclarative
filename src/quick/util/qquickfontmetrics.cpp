// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfontmetrics_p.h"

#include <QFont>

QT_BEGIN_NAMESPACE

/*!
    \qmltype FontMetrics
    \instantiates QQuickFontMetrics
    \inqmlmodule QtQuick
    \since 5.4
    \ingroup qtquick-text-utility
    \brief Provides metrics for a given font.

    FontMetrics calculates the size of characters and strings for a given font.

    It provides a subset of the C++ \l QFontMetricsF API, with the added
    ability to change the font that is used for calculations via the \l font
    property.

    \code
    FontMetrics {
        id: fontMetrics
        font.family: "Arial"
    }

    Rectangle {
        width: fontMetrics.height * 4
        height: fontMetrics.height * 2
    }
    \endcode

    \sa QFontMetricsF, TextMetrics
*/
QQuickFontMetrics::QQuickFontMetrics(QObject *parent) :
    QObject(parent),
    m_metrics(m_font)
{
}

/*!
    \qmlproperty font QtQuick::FontMetrics::font

    This property holds the font used for the metrics calculations.
*/
QFont QQuickFontMetrics::font() const
{
    return m_font;
}

void QQuickFontMetrics::setFont(const QFont &font)
{
    if (m_font != font) {
        m_font = font;
        m_metrics = QFontMetricsF(m_font);
        emit fontChanged(m_font);
    }
}

/*!
    \qmlproperty real QtQuick::FontMetrics::ascent

    This property holds the ascent of the font.

    \sa {QFontMetricsF::ascent()}, descent, height
*/
qreal QQuickFontMetrics::ascent() const
{
    return m_metrics.ascent();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::descent

    This property holds the descent of the font.

    \sa {QFontMetricsF::descent()}, ascent, height
*/
qreal QQuickFontMetrics::descent() const
{
    return m_metrics.descent();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::height

    This property holds the height of the font.

    \sa {QFontMetricsF::height()}
*/
qreal QQuickFontMetrics::height() const
{
    return m_metrics.height();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::leading

    This property holds the leading of the font.

    \sa {QFontMetricsF::leading()}
*/
qreal QQuickFontMetrics::leading() const
{
    return m_metrics.leading();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::lineSpacing

    This property holds the distance from one base line to the next.

    \sa {QFontMetricsF::lineSpacing()}
*/
qreal QQuickFontMetrics::lineSpacing() const
{
    return m_metrics.lineSpacing();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::minimumLeftBearing

    This property holds the minimum left bearing of the font.

    \sa {QFontMetricsF::minLeftBearing()}
*/
qreal QQuickFontMetrics::minimumLeftBearing() const
{
    return m_metrics.minLeftBearing();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::minimumRightBearing

    This property holds the minimum right bearing of the font.

    \sa {QFontMetricsF::minRightBearing()}
*/
qreal QQuickFontMetrics::minimumRightBearing() const
{
    return m_metrics.minRightBearing();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::maximumCharacterWidth

    This property holds the width of the widest character in the font.

    \sa {QFontMetricsF::maxWidth()}
*/
qreal QQuickFontMetrics::maximumCharacterWidth() const
{
    return m_metrics.maxWidth();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::xHeight

    This property holds the 'x' height of the font.

    \sa {QFontMetricsF::xHeight()}
*/
qreal QQuickFontMetrics::xHeight() const
{
    return m_metrics.xHeight();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::averageCharacterWidth

    This property holds the average width of glyphs in the font.

    \sa {QFontMetricsF::averageCharWidth()}
*/
qreal QQuickFontMetrics::averageCharacterWidth() const
{
    return m_metrics.averageCharWidth();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::underlinePosition

    This property holds the distance from the base line to where an underscore
    should be drawn.

    \sa {QFontMetricsF::underlinePos()}, overlinePosition, strikeOutPosition
*/
qreal QQuickFontMetrics::underlinePosition() const
{
    return m_metrics.underlinePos();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::overlinePosition

    This property holds the distance from the base line to where an overline
    should be drawn.

    \sa {QFontMetricsF::overlinePos()}, underlinePosition, strikeOutPosition
*/
qreal QQuickFontMetrics::overlinePosition() const
{
    return m_metrics.overlinePos();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::strikeOutPosition

    This property holds the distance from the base line to where the strikeout
    line should be drawn.

    \sa {QFontMetricsF::strikeOutPos()}, overlinePosition, underlinePosition
*/
qreal QQuickFontMetrics::strikeOutPosition() const
{
    return m_metrics.strikeOutPos();
}

/*!
    \qmlproperty real QtQuick::FontMetrics::lineWidth

    This property holds the width of the underline and strikeout lines,
    adjusted for the point size of the font.

    \sa {QFontMetricsF::lineWidth()}
*/
qreal QQuickFontMetrics::lineWidth() const
{
    return m_metrics.lineWidth();
}

/*!
    \qmlmethod qreal QtQuick::FontMetrics::advanceWidth(string text)

    This method returns the advance in pixels of the characters in \a text.
    This is the distance from the position of the string to where the next
    string should be drawn.

    This method is offered as an imperative alternative to the
    \l {QtQuick::TextMetrics::advanceWidth}{advanceWidth} property of
    TextMetrics.

    \sa {QFontMetricsF::horizontalAdvance()}, {QFontMetricsF::height()}
*/
qreal QQuickFontMetrics::advanceWidth(const QString &text) const
{
    return m_metrics.horizontalAdvance(text);
}

/*!
    \qmlmethod rect QtQuick::FontMetrics::boundingRect(string text)

    This method returns the bounding rectangle of the characters in the string
    specified by \a text.

    This method is offered as an imperative alternative to the
    \l {QtQuick::TextMetrics::boundingRect}{boundingRect} property of
    TextMetrics.

    \sa {QFontMetricsF::boundingRect()}, tightBoundingRect()
*/
QRectF QQuickFontMetrics::boundingRect(const QString &text) const
{
    return m_metrics.boundingRect(text);
}

/*!
    \qmlmethod rect QtQuick::FontMetrics::tightBoundingRect(string text)

    This method returns a tight bounding rectangle around the characters in the
    string specified by \a text.

    This method is offered as an imperative alternative to the
    \l {QtQuick::TextMetrics::tightBoundingRect}{tightBoundingRect} property of
    TextMetrics.

    \sa {QFontMetricsF::tightBoundingRect()}, boundingRect()
*/
QRectF QQuickFontMetrics::tightBoundingRect(const QString &text) const
{
    return m_metrics.tightBoundingRect(text);
}

/*!
    \qmlmethod string QtQuick::FontMetrics::elidedText(string text, enumeration mode, real width, int flags)

    This method returns an elided version of the string (i.e., a
    string with "..." in it) if the string \a text is wider than \a width.
    Otherwise, returns the original string.

    The \a mode argument specifies the text elide mode; that is, where
    the ellipsis should appear when displaying text that doesn't fit.

    The \a flags argument is optional and currently only supports
    \l {Qt::TextShowMnemonic}.

    This method is offered as an imperative alternative to the
    \l {QtQuick::TextMetrics::elidedText}{elidedText} property of
    TextMetrics.

    \sa Qt::TextElideMode, QFontMetricsF::elidedText()
*/
QString QQuickFontMetrics::elidedText(const QString &text, Qt::TextElideMode mode, qreal width, int flags) const
{
    return m_metrics.elidedText(text, mode, width, flags);
}

QT_END_NAMESPACE

#include "moc_qquickfontmetrics_p.cpp"
