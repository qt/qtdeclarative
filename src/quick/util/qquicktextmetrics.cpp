// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktextmetrics_p.h"

#include <QFont>
#include <QTextOption>

QT_BEGIN_NAMESPACE

/*!
    \qmltype TextMetrics
    \instantiates QQuickTextMetrics
    \inqmlmodule QtQuick
    \since 5.4
    \ingroup qtquick-text-utility
    \brief Provides metrics for a given font and text.

    TextMetrics calculates various properties of a given string of text for a
    particular font.

    It provides a declarative API for the functions in \l QFontMetricsF which
    take arguments.

    \code
    TextMetrics {
        id: textMetrics
        font.family: "Arial"
        elide: Text.ElideMiddle
        elideWidth: 100
        text: "Hello World"
    }

    MyItem {
        text: textMetrics.elidedText
    }
    \endcode

    \sa QFontMetricsF, FontMetrics
*/
QQuickTextMetrics::QQuickTextMetrics(QObject *parent) :
    QObject(parent),
    m_metrics(m_font),
    m_elide(Qt::ElideNone),
    m_elideWidth(0),
    m_renderType(QQuickText::QtRendering)
{
}

/*!
    \qmlproperty font QtQuick::TextMetrics::font

    This property holds the font used for the metrics calculations.
*/
QFont QQuickTextMetrics::font() const
{
    return m_font;
}

void QQuickTextMetrics::setFont(const QFont &font)
{
    if (m_font != font) {
        m_font = font;
        m_metrics = QFontMetricsF(m_font);
        emit fontChanged();
        emit metricsChanged();
    }
}

/*!
    \qmlproperty string QtQuick::TextMetrics::text

    This property holds the text used for the metrics calculations.
*/
QString QQuickTextMetrics::text() const
{
    return m_text;
}

void QQuickTextMetrics::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged();
        emit metricsChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick::TextMetrics::elide

    This property holds the elide mode of the text. This determines the
    position in which the string is elided. The possible values are:

    \value Qt::ElideNone    No eliding; this is the default value.
    \value Qt::ElideLeft    For example: "...World"
    \value Qt::ElideMiddle  For example: "He...ld"
    \value Qt::ElideRight   For example: "Hello..."

    \sa elideWidth, QFontMetrics::elidedText
*/
Qt::TextElideMode QQuickTextMetrics::elide() const
{
    return m_elide;
}

void QQuickTextMetrics::setElide(Qt::TextElideMode elide)
{
    if (m_elide != elide) {
        m_elide = elide;
        emit elideChanged();
        emit metricsChanged();
    }
}

/*!
    \qmlproperty real QtQuick::TextMetrics::elideWidth

    This property holds the largest width the text can have (in pixels) before
    eliding will occur.

    \sa elide, QFontMetrics::elidedText
*/
qreal QQuickTextMetrics::elideWidth() const
{
    return m_elideWidth;
}

void QQuickTextMetrics::setElideWidth(qreal elideWidth)
{
    if (m_elideWidth != elideWidth) {
        m_elideWidth = elideWidth;
        emit elideWidthChanged();
        emit metricsChanged();
    }
}

/*!
    \qmlproperty real QtQuick::TextMetrics::advanceWidth

    This property holds the advance in pixels of the characters in \l text.
    This is the distance from the position of the string to where the next
    string should be drawn.

    \sa {QFontMetricsF::horizontalAdvance()}
*/
qreal QQuickTextMetrics::advanceWidth() const
{
    QTextOption option;
    option.setUseDesignMetrics(m_renderType == QQuickText::QtRendering);
    return m_metrics.horizontalAdvance(m_text, option);
}

/*!
    \qmlproperty rect QtQuick::TextMetrics::boundingRect

    This property holds the bounding rectangle of the characters in the string
    specified by \l text.

    \sa {QFontMetricsF::boundingRect()}, tightBoundingRect
*/
QRectF QQuickTextMetrics::boundingRect() const
{
    QTextOption option;
    option.setUseDesignMetrics(m_renderType == QQuickText::QtRendering);
    return m_metrics.boundingRect(m_text, option);
}

/*!
    \qmlproperty real QtQuick::TextMetrics::width

    This property holds the width of the bounding rectangle of the characters
    in the string specified by \l text. It is equivalent to:

    \code
    textMetrics.boundingRect.width
    \endcode

    \sa boundingRect
*/
qreal QQuickTextMetrics::width() const
{
    return boundingRect().width();
}

/*!
    \qmlproperty real QtQuick::TextMetrics::height

    This property holds the height of the bounding rectangle of the characters
    in the string specified by \l text. It is equivalent to:

    \code
    textMetrics.boundingRect.height
    \endcode

    \sa boundingRect
*/
qreal QQuickTextMetrics::height() const
{
    return boundingRect().height();
}

/*!
    \qmlproperty rect QtQuick::TextMetrics::tightBoundingRect

    This property holds a tight bounding rectangle around the characters in the
    string specified by \l text.

    \sa {QFontMetricsF::tightBoundingRect()}, boundingRect
*/
QRectF QQuickTextMetrics::tightBoundingRect() const
{
    QTextOption option;
    option.setUseDesignMetrics(m_renderType == QQuickText::QtRendering);
    return m_metrics.tightBoundingRect(m_text, option);
}

/*!
    \qmlproperty string QtQuick::TextMetrics::elidedText

    This property holds an elided version of the string (i.e., a string with
    "..." in it) if the string \l text is wider than \l elideWidth. If the
    text is not wider than \l elideWidth, or \l elide is set to
    \c Qt::ElideNone, this property will be equal to the original string.

    \sa {QFontMetricsF::elidedText()}
*/
QString QQuickTextMetrics::elidedText() const
{
    return m_metrics.elidedText(m_text, m_elide, m_elideWidth);
}

/*!
    \qmlproperty enumeration QtQuick::TextMetrics::renderType

    Override the default rendering type for this component.

    Supported render types are:

    \value TextEdit.QtRendering     Text is rendered using a scalable distance field for each glyph.
    \value TextEdit.NativeRendering Text is rendered using a platform-specific technique.

    This should match the intended \c renderType where you draw the text.

    \since 6.3
    \sa {Text::renderType}{Text.renderType}
*/
QQuickText::RenderType QQuickTextMetrics::renderType() const
{
    return m_renderType;
}

void QQuickTextMetrics::setRenderType(QQuickText::RenderType renderType)
{
    if (m_renderType == renderType)
        return;

    m_renderType = renderType;
    emit renderTypeChanged();
}

QT_END_NAMESPACE

#include "moc_qquicktextmetrics_p.cpp"
