// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "debugpaintitem.h"

#include <QPainter>
#include <QPen>

DebugPaintItem::DebugPaintItem(QQuickItem *item)
    : QQuickPaintedItem(item)
{
    connect(this, &DebugPaintItem::shapeChanged, this, &QQuickItem::update);
    connect(this, &DebugPaintItem::colorChanged, this, &QQuickItem::update);
    connect(this, &DebugPaintItem::pathScaleChanged, this, &QQuickItem::update);
    connect(this, &DebugPaintItem::fillRuleChanged, this, &QQuickItem::update);
    connect(this, &DebugPaintItem::strokeChanged, this, &QQuickItem::update);
}

void DebugPaintItem::setShape(QQuickPath *p)
{
    if (p == m_path)
        return;

    m_path = p;
    emit shapeChanged();
}

QQuickPath *DebugPaintItem::shape() const
{
    return m_path;
}

void DebugPaintItem::handlePathChanged()
{
    update();
}

void DebugPaintItem::paint(QPainter *p)
{
    if (!isVisible())
        return;

    if (m_path != nullptr) {
        QPainterPath painterPath = m_path->path();
        painterPath.setFillRule(m_fillRule);

        p->scale(m_pathScale, m_pathScale);
        p->setRenderHint(QPainter::Antialiasing);
        if (m_strokeColor.alpha() > 0) {
            QPen pen(m_strokeStyle);
            pen.setWidthF(m_strokeWidth);
            pen.setCapStyle(m_capStyle);
            pen.setJoinStyle(m_joinStyle);
            pen.setColor(m_strokeColor);
            p->setPen(pen);
        } else {
            p->setPen(Qt::NoPen);
        }
        p->setBrush(m_color);
        p->drawPath(painterPath);
    }
}

QColor DebugPaintItem::color() const
{
    return m_color;
}

void DebugPaintItem::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;
    emit colorChanged();
}

void DebugPaintItem::setPathScale(qreal pathScale)
{
    if (qFuzzyCompare(m_pathScale, pathScale))
        return;

    m_pathScale = pathScale;
    emit pathScaleChanged();
}

qreal DebugPaintItem::pathScale() const
{
    return m_pathScale;
}

void DebugPaintItem::setFillRule(Qt::FillRule fillRule)
{
    if (m_fillRule == fillRule)
        return;

    m_fillRule = fillRule;
    emit fillRuleChanged();
}

Qt::FillRule DebugPaintItem::fillRule() const
{
    return m_fillRule;
}

void DebugPaintItem::setStrokeColor(const QColor &strokeColor)
{
    if (m_strokeColor == strokeColor)
        return;
    m_strokeColor = strokeColor;
    emit strokeChanged();
}

QColor DebugPaintItem::strokeColor() const
{
    return m_strokeColor;
}

void DebugPaintItem::setStrokeStyle(Qt::PenStyle strokeStyle)
{
    if (m_strokeStyle == strokeStyle)
        return;

    m_strokeStyle = strokeStyle;
    emit strokeChanged();
}

Qt::PenStyle DebugPaintItem::strokeStyle() const
{
    return m_strokeStyle;
}

void DebugPaintItem::setJoinStyle(Qt::PenJoinStyle style)
{
    if (m_joinStyle == style)
        return;
    m_joinStyle = style;
    emit strokeChanged();
}

Qt::PenJoinStyle DebugPaintItem::joinStyle() const
{
    return m_joinStyle;
}

void DebugPaintItem::setCapStyle(Qt::PenCapStyle style)
{
    if (m_capStyle == style)
        return;
    m_capStyle = style;
    emit strokeChanged();
}

Qt::PenCapStyle DebugPaintItem::capStyle() const
{
    return m_capStyle;
}

void DebugPaintItem::setStrokeWidth(qreal w)
{
    if (qFuzzyCompare(m_strokeWidth, w))
        return;
    m_strokeWidth = w;
    emit strokeChanged();
}

qreal DebugPaintItem::strokeWidth() const
{
    return m_strokeWidth;
}
