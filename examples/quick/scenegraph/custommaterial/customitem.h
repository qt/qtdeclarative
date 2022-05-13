// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CUSTOMITEM_H
#define CUSTOMITEM_H

//! [1]
#include <QQuickItem>

class CustomItem : public QQuickItem
{
    Q_OBJECT
//! [2]
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(int iterationLimit READ iterationLimit WRITE setIterationLimit NOTIFY iterationLimitChanged)
    Q_PROPERTY(QPointF center READ center WRITE setCenter NOTIFY centerChanged)
//! [2]
    QML_ELEMENT

public:
    explicit CustomItem(QQuickItem *parent = nullptr);

    qreal zoom() const
    {
        return m_zoom;
    }

    int iterationLimit() const
    {
        return m_limit;
    }

    QPointF center() const
    {
        return m_center;
    }

public slots:
    void setZoom(qreal zoom);

    void setIterationLimit(int iterationLimit);

    void setCenter(QPointF center);

signals:
    void zoomChanged(qreal zoom);

    void iterationLimitChanged(int iterationLimit);

    void centerChanged(QPointF center);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    bool m_geometryChanged = true;
    qreal m_zoom;
    bool m_zoomChanged = true;
    int m_limit;
    bool m_limitChanged = true;
    QPointF m_center;
    bool m_centerChanged = true;
};
//! [1]
#endif // CUSTOMITEM_H
