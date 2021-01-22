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

#ifndef QQUICKPOINTERSINGLEHANDLER_P_H
#define QQUICKPOINTERSINGLEHANDLER_P_H

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

#include "qquickhandlerpoint_p.h"
#include "qquickpointerdevicehandler_p_p.h"
#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickSinglePointHandlerPrivate : public QQuickPointerDeviceHandlerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSinglePointHandler)

public:
    static QQuickSinglePointHandlerPrivate* get(QQuickSinglePointHandler *q) { return q->d_func(); }
    static const QQuickSinglePointHandlerPrivate* get(const QQuickSinglePointHandler *q) { return q->d_func(); }

    QQuickSinglePointHandlerPrivate();

    void reset();

    QQuickHandlerPoint pointInfo;
    bool ignoreAdditionalPoints = false;
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERSINGLEHANDLER_P_H

