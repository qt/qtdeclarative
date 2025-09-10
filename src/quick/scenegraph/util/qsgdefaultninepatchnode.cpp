// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
