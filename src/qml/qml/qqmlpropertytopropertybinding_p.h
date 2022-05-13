// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLPROPERTYTOPROPERTYBINDINDING_P_H
#define QQMLPROPERTYTOPROPERTYBINDINDING_P_H

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

#include <private/qqmlabstractbinding_p.h>
#include <private/qqmlnotifier_p.h>
#include <QtCore/qproperty.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlPropertyToPropertyBinding
    : public QQmlAbstractBinding, public QQmlNotifierEndpoint
{
public:
    QQmlPropertyToPropertyBinding(
            QQmlEngine *engine, QObject *sourceObject, int sourcePropertyIndex,
            QObject *targetObject, int targetPropertyIndex);

    Kind kind() const final;
    void setEnabled(bool e, QQmlPropertyData::WriteFlags flags) final;

    void update(QQmlPropertyData::WriteFlags flags = QQmlPropertyData::DontRemoveBinding);

private:
    static void trigger(QPropertyObserver *, QUntypedPropertyData *);

    void captureProperty(
            const QMetaObject *sourceMetaObject, int notifyIndex,
            bool isSourceBindable, bool isTargetBindable);

    struct Observer : QPropertyObserver {
        static void trigger(QPropertyObserver *observer, QUntypedPropertyData *);
        Observer(QQmlPropertyToPropertyBinding *binding)
            : QPropertyObserver(trigger)
            , binding(binding)
        {
        }
        QQmlPropertyToPropertyBinding *binding = nullptr;
    };

    std::unique_ptr<Observer> observer;
    QQmlEngine *m_engine = nullptr;
    QObject *m_sourceObject = nullptr;
    int m_sourcePropertyIndex = -1;
};

void QQmlPropertyGuard_callback(QQmlNotifierEndpoint *e, void **);

QT_END_NAMESPACE

#endif
