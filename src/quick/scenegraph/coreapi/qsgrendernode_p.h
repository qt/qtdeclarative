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

#ifndef QSGRENDERNODE_P_H
#define QSGRENDERNODE_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgrendernode.h>
#include <functional>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QSGRenderNodePrivate
{
public:
    QSGRenderNodePrivate();

    static QSGRenderNodePrivate *get(QSGRenderNode *node) { return node->d; }

    const QMatrix4x4 *m_matrix;
    const QSGClipNode *m_clip_list;
    qreal m_opacity;

    // ### Qt 6: change this into a value for flags()
    bool m_needsExternalRendering;
    // ### Qt 6: change this into a virtual prepare() function
    std::function<void()> m_prepareCallback;
};

QT_END_NAMESPACE

#endif
