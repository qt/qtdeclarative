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

#include "qsgd3d12material_p.h"
#include <private/qsgrenderer_p.h>

QT_BEGIN_NAMESPACE

QSGD3D12Material::RenderState QSGD3D12Material::makeRenderState(QSGRenderer *renderer, RenderState::DirtyStates dirty)
{
    RenderState rs;
    rs.m_dirty = dirty;
    rs.m_data = renderer;
    return rs;
}

float QSGD3D12Material::RenderState::opacity() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentOpacity();
}

float QSGD3D12Material::RenderState::determinant() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->determinant();
}

QMatrix4x4 QSGD3D12Material::RenderState::combinedMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentCombinedMatrix();
}

float QSGD3D12Material::RenderState::devicePixelRatio() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->devicePixelRatio();
}

QMatrix4x4 QSGD3D12Material::RenderState::modelViewMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentModelViewMatrix();
}

QMatrix4x4 QSGD3D12Material::RenderState::projectionMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentProjectionMatrix();
}

QRect QSGD3D12Material::RenderState::viewportRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->viewportRect();
}

QRect QSGD3D12Material::RenderState::deviceRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->deviceRect();
}

QSGMaterialShader *QSGD3D12Material::createShader() const
{
    return nullptr;
}

QT_END_NAMESPACE
