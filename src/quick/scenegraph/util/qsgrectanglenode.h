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

#ifndef QSGRECTANGLENODE_H
#define QSGRECTANGLENODE_H

#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGRectangleNode : public QSGGeometryNode
{
public:
    ~QSGRectangleNode() override = default;

    virtual void setRect(const QRectF &rect) = 0;
    inline void setRect(qreal x, qreal y, qreal w, qreal h) { setRect(QRectF(x, y, w, h)); }
    virtual QRectF rect() const = 0;

    virtual void setColor(const QColor &color) = 0;
    virtual QColor color() const = 0;
};

QT_END_NAMESPACE

#endif // QSGRECTANGLENODE_H
