/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgrendernode_p.h"

QT_BEGIN_NAMESPACE

QSGRenderNode::QSGRenderNode()
    : QSGNode(RenderNodeType)
    , m_matrix(0)
    , m_clip_list(0)
    , m_opacity(1)
{
}

void QSGRenderNode::setInheritedOpacity(qreal opacity)
{
    Q_ASSERT(opacity >= 0 && opacity <= 1);
    m_opacity = opacity;
}

/*!
    \fn QSGRenderNode::StateFlags QSGRenderNode::changedStates()

    This function should return a mask where each bit represents OpenGL states changed by
    the \l render() function:
    \list
    \o DepthState - depth write mask, depth test enabled, depth comparison function
    \o StencilState - stencil write masks, stencil test enabled, stencil operations,
                      stencil comparison functions
    \o ScissorState - scissor enabled, scissor test enabled
    \o ColorState - clear color, color write mask
    \o BlendState - blend enabled, blend function
    \o CullState - front face, cull face enabled
    \o ViewportState - viewport
    \endlist

    The function is called by the renderer so it can reset the OpenGL states after rendering this
    node.

    \internal
  */

/*!
    \fn void QSGRenderNode::render(const RenderState &state)

    This function is called by the renderer and should paint this node with OpenGL commands.

    The states necessary for clipping has already been set before the function is called.
    The clip is a combination of a stencil clip and scissor clip. Information about the clip is
    found in \a state.

    The effective opacity can be retrieved with \l inheritedOpacity().

    The projection matrix is available through \a state, while the model-view matrix can be
    fetched with \l matrix(). The combined matrix is then the projection matrix times the
    model-view matrix.

    The following states are set before this function is called:
    \list
    \o glDepthMask(false)
    \o glDisable(GL_DEPTH_TEST)
    \o glStencilMask(0)
    \o glEnable(GL_STENCIL_TEST)/glDisable(GL_STENCIL_TEST) depending on clip
    \o glStencilFunc(GL_EQUAL, state.stencilValue, 0xff) depending on clip
    \o glEnable(GL_SCISSOR_TEST)/glDisable(GL_SCISSOR_TEST) depending on clip
    \o glScissor(state.scissorRect.x(), state.scissorRect.y(),
                 state.scissorRect.width(), state.scissorRect.height()) depending on clip
    \o glEnable(GL_BLEND)
    \o glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
    \o glDisable(GL_CULL_FACE)
    \endlist

    States that are not listed above, but are included in \l StateFlags, can have arbitrary
    values.

    \l changedStates() should return which states this function has changed. If a state is not
    covered by \l StateFlags, the state should be set to the default value according to the
    OpenGL specification.

    \internal
  */

QT_END_NAMESPACE
