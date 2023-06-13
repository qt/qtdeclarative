// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DEBUGPAINTITEM_H
#define DEBUGPAINTITEM_H

#include <QQuickPaintedItem>
#include <QtQuick/private/qquickpath_p.h>

class DebugPaintItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickPath *shape READ shape WRITE setShape NOTIFY shapeChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal pathScale READ pathScale WRITE setPathScale NOTIFY pathScaleChanged)
    Q_PROPERTY(Qt::FillRule fillRule READ fillRule WRITE setFillRule NOTIFY fillRuleChanged)

    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeChanged)
    Q_PROPERTY(Qt::PenStyle strokeStyle READ strokeStyle WRITE setStrokeStyle NOTIFY strokeChanged)
    Q_PROPERTY(Qt::PenJoinStyle joinStyle READ joinStyle WRITE setJoinStyle NOTIFY strokeChanged)
    Q_PROPERTY(Qt::PenCapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY strokeChanged)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeChanged)
public:
    DebugPaintItem(QQuickItem *item = nullptr);

    void setShape(QQuickPath *path);
    QQuickPath *shape() const;

    void setColor(const QColor &color);
    QColor color() const;

    void setPathScale(qreal pathScale);
    qreal pathScale() const;

    void setFillRule(Qt::FillRule filleRule);
    Qt::FillRule fillRule() const;

    QColor strokeColor() const;
    void setStrokeColor(const QColor &strokeColor);

    Qt::PenStyle strokeStyle() const;
    void setStrokeStyle(Qt::PenStyle penStyle);

    Qt::PenJoinStyle joinStyle() const;
    void setJoinStyle(Qt::PenJoinStyle style);

    Qt::PenCapStyle capStyle() const;
    void setCapStyle(Qt::PenCapStyle style);

    qreal strokeWidth() const;
    void setStrokeWidth(qreal w);

signals:
    void shapeChanged();
    void colorChanged();
    void opacityChanged();
    void pathScaleChanged();
    void fillRuleChanged();
    void strokeChanged();

public slots:
    void handlePathChanged();

protected:
    void paint(QPainter *p) override;

private:
    QQuickPath *m_path = nullptr;
    QColor m_color = Qt::red;
    qreal m_pathScale = 1.0;
    Qt::FillRule m_fillRule = Qt::WindingFill;
    Qt::PenStyle m_strokeStyle = Qt::NoPen;
    QColor m_strokeColor = Qt::transparent;
    qreal m_strokeWidth = 1.0;
    Qt::PenCapStyle m_capStyle = Qt::FlatCap;
    Qt::PenJoinStyle m_joinStyle = Qt::MiterJoin;
};

#endif // DEBUGPAINTITEM_H
