/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
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

#ifndef QSGOPENGLVISUALIZER_P_H
#define QSGOPENGLVISUALIZER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsgbatchrenderer_p.h"

QT_BEGIN_NAMESPACE

namespace QSGBatchRenderer
{

// ### Qt 6: remove
class OpenGLVisualizer : public Visualizer
{
public:
    OpenGLVisualizer(Renderer *renderer);
    ~OpenGLVisualizer();

    void prepareVisualize() override;
    void visualize() override;

    void releaseResources() override;

private:
    void visualizeBatch(Batch *b);
    void visualizeClipping(QSGNode *node);
    void visualizeChanges(Node *n);
    void visualizeOverdraw();
    void visualizeOverdraw_helper(Node *node);
    void visualizeDrawGeometry(const QSGGeometry *g);

    QOpenGLFunctions *m_funcs;
    QOpenGLShaderProgram *m_visualizeProgram;
};

} // namespace QSGBatchRenderer

QT_END_NAMESPACE

#endif // QSGOPENGLVISUALIZER_P_H
