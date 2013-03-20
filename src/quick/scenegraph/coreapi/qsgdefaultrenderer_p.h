/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGDEFAULTRENDERER_P_H
#define QSGDEFAULTRENDERER_P_H

#include "qsgrenderer_p.h"

#include <QtGui/private/qdatabuffer_p.h>
#include "qsgrendernode_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QSGDefaultRenderer : public QSGRenderer
{
    Q_OBJECT
public:
    QSGDefaultRenderer(QSGContext *context);

    void render();

    void nodeChanged(QSGNode *node, QSGNode::DirtyState state);

    void setSortFrontToBackEnabled(bool sort);
    bool isSortFrontToBackEnabled() const;

private:
    void buildLists(QSGNode *node);
    void renderNodes(QSGNode *const *nodes, int count);

    const QSGClipNode *m_currentClip;
    QSGMaterial *m_currentMaterial;
    QSGMaterialShader *m_currentProgram;
    const QMatrix4x4 *m_currentMatrix;
    QDataBuffer<QSGNode *> m_opaqueNodes;
    QDataBuffer<QSGNode *> m_transparentNodes;
    struct RenderGroup { int opaqueEnd, transparentEnd; };
    QDataBuffer<RenderGroup> m_renderGroups;

    bool m_rebuild_lists;
    bool m_sort_front_to_back;
    bool m_render_node_added;
    int m_currentRenderOrder;

#ifdef QML_RUNTIME_TESTING
    bool m_render_opaque_nodes;
    bool m_render_alpha_nodes;
#endif
};

QT_END_NAMESPACE

#endif // QMLRENDERER_H
