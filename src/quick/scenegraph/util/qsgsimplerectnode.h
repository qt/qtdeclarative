/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSGSIMPLERECTNODE_H
#define QSGSIMPLERECTNODE_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgflatcolormaterial.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGSimpleRectNode : public QSGGeometryNode
{
public:
    QSGSimpleRectNode(const QRectF &rect, const QColor &color);
    QSGSimpleRectNode();

    void setRect(const QRectF &rect);
    inline void setRect(qreal x, qreal y, qreal w, qreal h) { setRect(QRectF(x, y, w, h)); }
    QRectF rect() const;

    void setColor(const QColor &color);
    QColor color() const;

private:
    QSGFlatColorMaterial m_material;
    QSGGeometry m_geometry;
    void *reserved;
};

QT_END_NAMESPACE

#endif // SOLIDRECTNODE_H
