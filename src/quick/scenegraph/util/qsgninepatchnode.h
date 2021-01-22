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

#ifndef QSGNINEPATCHNODE_H
#define QSGNINEPATCHNODE_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexture.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGNinePatchNode : public QSGGeometryNode
{
public:
    ~QSGNinePatchNode() override = default;

    virtual void setTexture(QSGTexture *texture) = 0;
    virtual void setBounds(const QRectF &bounds) = 0;
    virtual void setDevicePixelRatio(qreal ratio) = 0;
    virtual void setPadding(qreal left, qreal top, qreal right, qreal bottom) = 0;
    virtual void update() = 0;

    static void rebuildGeometry(QSGTexture *texture, QSGGeometry *geometry,
                                const QVector4D &padding,
                                const QRectF &bounds, qreal dpr);
};

QT_END_NAMESPACE

#endif // QSGNINEPATCHNODE_H
