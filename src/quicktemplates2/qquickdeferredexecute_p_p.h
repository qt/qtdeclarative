/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKDEFERREDEXECUTE_P_P_H
#define QQUICKDEFERREDEXECUTE_P_P_H

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

#include <QtCore/qglobal.h>
#include <QtQuickTemplates2/private/qquickdeferredpointer_p_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

#include <QtQml/private/qqmlvme_p.h>

QT_BEGIN_NAMESPACE

class QString;
class QObject;

namespace QtQuickPrivate {
    Q_QUICKTEMPLATES2_PRIVATE_EXPORT void beginDeferred(QObject *object, const QString &property);
    Q_QUICKTEMPLATES2_PRIVATE_EXPORT void cancelDeferred(QObject *object, const QString &property);
    Q_QUICKTEMPLATES2_PRIVATE_EXPORT void completeDeferred(QObject *object, const QString &property);
}

template<typename T>
void quickBeginDeferred(QObject *object, const QString &property, QQuickDeferredPointer<T> &delegate)
{
    if (!QQmlVME::componentCompleteEnabled())
           return;

    delegate.setExecuting(true);
    QtQuickPrivate::beginDeferred(object, property);
    delegate.setExecuting(false);
}

inline void quickCancelDeferred(QObject *object, const QString &property)
{
    QtQuickPrivate::cancelDeferred(object, property);
}

template<typename T>
void quickCompleteDeferred(QObject *object, const QString &property, QQuickDeferredPointer<T> &delegate)
{
    Q_ASSERT(!delegate.wasExecuted());
    QtQuickPrivate::completeDeferred(object, property);
    delegate.setExecuted();
}

QT_END_NAMESPACE

#endif // QQUICKDEFERREDEXECUTE_P_P_H
