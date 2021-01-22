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

#ifndef DESIGNERSUPPORTSTATES_H
#define DESIGNERSUPPORTSTATES_H

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

#include "qquickdesignersupport_p.h"

#include <QVariant>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickDesignerSupportStates
{
public:
    static bool isStateActive(QObject *object, QQmlContext *context);
    static void activateState(QObject *object, QQmlContext *context);
    static void deactivateState(QObject *object);
    static bool changeValueInRevertList(QObject *state,
                                        QObject *target,
                                        const QQuickDesignerSupport::PropertyName &propertyName,
                                        const QVariant &value);

    static bool updateStateBinding(QObject *state, QObject *target,
                                   const QQuickDesignerSupport::PropertyName &propertyName,
                                   const QString &expression);

    static bool resetStateProperty(QObject *state, QObject *target,
                                   const QQuickDesignerSupport::PropertyName &propertyName,
                                   const QVariant &);
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORTSTATES_H
