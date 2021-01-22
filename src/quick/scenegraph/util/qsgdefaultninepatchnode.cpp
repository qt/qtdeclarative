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

#include "qsgdefaultninepatchnode_p.h"

QT_BEGIN_NAMESPACE

QSGDefaultNinePatchNode::QSGDefaultNinePatchNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    m_geometry.setDrawingMode(QSGGeometry::DrawTriangleStrip);
    setGeometry(&m_geometry);
    setMaterial(&m_material);
}

QSGDefaultNinePatchNode::~QSGDefaultNinePatchNode()
{
    delete m_material.texture();
}

void QSGDefaultNinePatchNode::setTexture(QSGTexture *texture)
{
    delete m_material.texture();
    m_material.setTexture(texture);
}

void QSGDefaultNinePatchNode::setBounds(const QRectF &bounds)
{
    m_bounds = bounds;
}

void QSGDefaultNinePatchNode::setDevicePixelRatio(qreal ratio)
{
    m_devicePixelRatio = ratio;
}

void QSGDefaultNinePatchNode::setPadding(qreal left, qreal top, qreal right, qreal bottom)
{
    m_padding = QVector4D(left, top, right, bottom);
}

void QSGDefaultNinePatchNode::update()
{
    rebuildGeometry(m_material.texture(), &m_geometry, m_padding, m_bounds, m_devicePixelRatio);
    markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
}

QT_END_NAMESPACE
