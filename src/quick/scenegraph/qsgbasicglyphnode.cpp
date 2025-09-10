// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgbasicglyphnode_p.h"
#include <qsgmaterial.h> // just so that we can safely do delete m_material in the dtor

QT_BEGIN_NAMESPACE

QSGBasicGlyphNode::QSGBasicGlyphNode()
    : m_style(QQuickText::Normal)
    , m_material(nullptr)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
{
    m_geometry.setDrawingMode(QSGGeometry::DrawTriangles);
    setGeometry(&m_geometry);
}

QSGBasicGlyphNode::~QSGBasicGlyphNode()
{
    delete m_material;
}

void QSGBasicGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    if (m_material != nullptr) {
        setMaterialColor(color);
        markDirty(DirtyMaterial);
    }
}

void QSGBasicGlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    if (m_material != nullptr)
        delete m_material;

    m_position = position;
    m_glyphs = glyphs;

#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("glyphs"));
#endif
}

void QSGBasicGlyphNode::setStyle(QQuickText::TextStyle style)
{
    if (m_style == style)
        return;
    m_style = style;
}

void QSGBasicGlyphNode::setStyleColor(const QColor &color)
{
    if (m_styleColor == color)
        return;
    m_styleColor = color;
}

QT_END_NAMESPACE
