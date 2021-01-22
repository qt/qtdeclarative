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

#ifndef QQUICKTRANSITIONMANAGER_P_H
#define QQUICKTRANSITIONMANAGER_P_H

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

#include "qquickanimation_p.h"
#include <private/qanimationjobutil_p.h>

QT_BEGIN_NAMESPACE

class QQuickState;
class QQuickStateAction;
class QQuickTransitionManagerPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickTransitionManager
{
public:
    QQuickTransitionManager();
    virtual ~QQuickTransitionManager();

    bool isRunning() const;

    void transition(const QList<QQuickStateAction> &, QQuickTransition *transition, QObject *defaultTarget = nullptr);

    void cancel();

    SelfDeletable m_selfDeletable;
protected:
    virtual void finished();

private:
    Q_DISABLE_COPY(QQuickTransitionManager)
    QQuickTransitionManagerPrivate *d;

    void complete();
    void setState(QQuickState *);

    friend class QQuickState;
    friend class ParallelAnimationWrapper;
};

QT_END_NAMESPACE

#endif // QQUICKTRANSITIONMANAGER_P_H
