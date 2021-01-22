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

#ifndef QSGABSTRACTRENDERER_P_H
#define QSGABSTRACTRENDERER_P_H

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

#include "qsgabstractrenderer.h"

#include "qsgnode.h"
#include <qcolor.h>

#include <QtCore/private/qobject_p.h>
#include <QtQuick/private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QSGAbstractRendererPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSGAbstractRenderer)
public:
    static const QSGAbstractRendererPrivate *get(const QSGAbstractRenderer *q) { return q->d_func(); }

    QSGAbstractRendererPrivate();
    void updateProjectionMatrix();

    QSGRootNode *m_root_node;
    QColor m_clear_color;
    QSGAbstractRenderer::ClearMode m_clear_mode;

    QRect m_device_rect;
    QRect m_viewport_rect;

    QMatrix4x4 m_projection_matrix;
    QMatrix4x4 m_projection_matrix_native_ndc;
    uint m_mirrored : 1;
};

QT_END_NAMESPACE

#endif
