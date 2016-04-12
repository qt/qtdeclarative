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

#include <private/qquickgenericshadereffect_p.h>

QT_BEGIN_NAMESPACE

// The generic shader effect is used when the scenegraph backend indicates
// SupportsShaderEffectV2. This, unlike the monolithic and interconnected (e.g.
// with particles) OpenGL variant, passes most of the work to a scenegraph node
// created via the adaptation layer, thus allowing different implementation in
// the backends.

QQuickGenericShaderEffect::QQuickGenericShaderEffect(QQuickShaderEffect *item, QObject *parent)
    : QObject(parent)
    , m_item(item)
    , m_meshResolution(1, 1)
    , m_mesh(0)
    , m_cullMode(QQuickShaderEffect::NoCulling)
    , m_status(QQuickShaderEffect::Uncompiled)
    , m_blending(true)
    , m_supportsAtlasTextures(false)
{
}

QQuickGenericShaderEffect::~QQuickGenericShaderEffect()
{
}

void QQuickGenericShaderEffect::setFragmentShader(const QByteArray &code)
{
    Q_UNUSED(code);
}

void QQuickGenericShaderEffect::setVertexShader(const QByteArray &code)
{
    Q_UNUSED(code);
}

void QQuickGenericShaderEffect::setBlending(bool enable)
{
    Q_UNUSED(enable);
}

QVariant QQuickGenericShaderEffect::mesh() const
{
    return m_mesh ? qVariantFromValue(static_cast<QObject *>(m_mesh))
                  : qVariantFromValue(m_meshResolution);
}

void QQuickGenericShaderEffect::setMesh(const QVariant &mesh)
{
    Q_UNUSED(mesh);
}

void QQuickGenericShaderEffect::setCullMode(QQuickShaderEffect::CullMode face)
{
    Q_UNUSED(face);
}

void QQuickGenericShaderEffect::setSupportsAtlasTextures(bool supports)
{
    Q_UNUSED(supports);
}

void QQuickGenericShaderEffect::handleEvent(QEvent *event)
{
    Q_UNUSED(event);
}

void QQuickGenericShaderEffect::handleGeometryChanged(const QRectF &, const QRectF &)
{
}

QSGNode *QQuickGenericShaderEffect::handleUpdatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    Q_UNUSED(oldNode);
    return nullptr;
}

void QQuickGenericShaderEffect::handleComponentComplete()
{
}

void QQuickGenericShaderEffect::handleItemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_UNUSED(change);
    Q_UNUSED(value);
}

QT_END_NAMESPACE
