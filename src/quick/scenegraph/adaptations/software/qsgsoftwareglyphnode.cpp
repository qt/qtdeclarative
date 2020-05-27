/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgsoftwareglyphnode_p.h"
#include <QtGui/private/qrawfont_p.h>

QT_BEGIN_NAMESPACE

QSGSoftwareGlyphNode::QSGSoftwareGlyphNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_style(QQuickText::Normal)
{
    setMaterial((QSGMaterial*)1);
    setGeometry(&m_geometry);
}

namespace {
QRectF calculateBoundingRect(const QPointF &position, const QGlyphRun &glyphs)
{
    QFixed minX;
    QFixed minY;
    QFixed maxX;
    QFixed maxY;

    QRawFontPrivate *rawFontD = QRawFontPrivate::get(glyphs.rawFont());
    QFontEngine *fontEngine = rawFontD->fontEngine;

    QFontEngine::GlyphFormat glyphFormat = fontEngine->glyphFormat != QFontEngine::Format_None ? fontEngine->glyphFormat : QFontEngine::Format_A32;

    int margin = fontEngine->glyphMargin(glyphFormat);

    const QVector<uint> glyphIndexes = glyphs.glyphIndexes();
    const QVector<QPointF> glyphPositions = glyphs.positions();
    for (int i = 0, n = qMin(glyphIndexes.size(), glyphPositions.size()); i < n; ++i) {
        glyph_metrics_t gm = fontEngine->alphaMapBoundingBox(glyphIndexes.at(i), QFixed(), QTransform(), glyphFormat);

        gm.x += QFixed::fromReal(glyphPositions.at(i).x()) - margin;
        gm.y += QFixed::fromReal(glyphPositions.at(i).y()) - margin;

        if (i == 0) {
            minX = gm.x;
            minY = gm.y;
            maxX = gm.x + gm.width;
            maxY = gm.y + gm.height;
        } else {
            minX = qMin(gm.x, minX);
            minY = qMin(gm.y, minY);
            maxX = qMax(gm.x + gm.width, maxX);
            maxY = qMax(gm.y + gm.height, maxY);
        }
    }

    QRectF boundingRect(QPointF(minX.toReal(), minY.toReal()), QPointF(maxX.toReal(), maxY.toReal()));
    return boundingRect.translated(position - QPointF(0.0, glyphs.rawFont().ascent()));
}
}

void QSGSoftwareGlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    m_position = position;
    m_glyphRun = glyphs;
    m_bounding_rect = calculateBoundingRect(position, glyphs);
}

void QSGSoftwareGlyphNode::setColor(const QColor &color)
{
    m_color = color;
}

void QSGSoftwareGlyphNode::setStyle(QQuickText::TextStyle style)
{
    m_style = style;
}

void QSGSoftwareGlyphNode::setStyleColor(const QColor &color)
{
    m_styleColor = color;
}

QPointF QSGSoftwareGlyphNode::baseLine() const
{
    return QPointF();
}

void QSGSoftwareGlyphNode::setPreferredAntialiasingMode(QSGGlyphNode::AntialiasingMode)
{
}

void QSGSoftwareGlyphNode::update()
{
}

void QSGSoftwareGlyphNode::paint(QPainter *painter)
{
    painter->setBrush(QBrush());
    QPointF pos = m_position - QPointF(0, m_glyphRun.rawFont().ascent());

    qreal offset = 1.0;
    if (painter->device()->devicePixelRatioF() > 0.0)
        offset = 1.0 / painter->device()->devicePixelRatioF();

    switch (m_style) {
    case QQuickText::Normal: break;
    case QQuickText::Outline:
        painter->setPen(m_styleColor);
        painter->drawGlyphRun(pos + QPointF(0, offset), m_glyphRun);
        painter->drawGlyphRun(pos + QPointF(0, -offset), m_glyphRun);
        painter->drawGlyphRun(pos + QPointF(offset, 0), m_glyphRun);
        painter->drawGlyphRun(pos + QPointF(-offset, 0), m_glyphRun);
        break;
    case QQuickText::Raised:
        painter->setPen(m_styleColor);
        painter->drawGlyphRun(pos + QPointF(0, offset), m_glyphRun);
        break;
    case QQuickText::Sunken:
        painter->setPen(m_styleColor);
        painter->drawGlyphRun(pos + QPointF(0, -offset), m_glyphRun);
        break;
    }

    painter->setPen(m_color);
    painter->drawGlyphRun(pos, m_glyphRun);
}

QT_END_NAMESPACE
