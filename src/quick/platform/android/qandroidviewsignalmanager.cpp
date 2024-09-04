// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>

#include <QtCore/private/qandroidtypeconverter_p.h>
#include <QtQuick/private/qandroidviewsignalmanager_p.h>

QT_BEGIN_NAMESPACE

void QAndroidViewSignalManager::forwardSignal()
{
    // We use VoidStar because creating QVariant from QtMetaType::Void is not possible
    invokeListener(sender(), senderSignalIndex(), QVariant::fromValue<void *>(nullptr));
}

void QAndroidViewSignalManager::forwardSignal(int signalValue)
{
    invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
}

void QAndroidViewSignalManager::forwardSignal(bool signalValue)
{
    invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
}

void QAndroidViewSignalManager::forwardSignal(double signalValue)
{
    invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
}

void QAndroidViewSignalManager::forwardSignal(float signalValue)
{
    invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
}

void QAndroidViewSignalManager::forwardSignal(QString signalValue)
{
    invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
}

void QAndroidViewSignalManager::invokeListener(QObject *sender, int senderSignalIndex,
                                               QVariant signalValue)
{
    using namespace QtJniTypes;

    const QMetaObject *metaObject = sender->metaObject();
    const QMetaMethod signalMethod = metaObject->method(senderSignalIndex);

    for (auto connectionInfoIter = connectionInfoMap.constFind(signalMethod.methodSignature());
         connectionInfoIter != connectionInfoMap.constEnd()
         && connectionInfoIter.key() == signalMethod.methodSignature();
         ++connectionInfoIter) {
        const ConnectionInfo connectionInfo = *connectionInfoIter;
        const QByteArray javaArgType = connectionInfo.javaArgType;
        QJniObject jSignalMethodName =
                QJniObject::fromString(QLatin1StringView(signalMethod.name()));

        if (connectionInfo.propertyIndex != -1 && javaArgType != Traits<Void>::className())
            signalValue = metaObject->property(connectionInfo.propertyIndex).read(sender);

        QJniObject jValue(
                QAndroidTypeConverter::toJavaObject(signalValue, QJniEnvironment::getJniEnv()));

        if (!jValue.isValid()) {
            qWarning("Mismatching argument types between QML signal (%s) and the Java function "
                     "(%s). Sending null as argument.",
                     signalMethod.methodSignature().constData(), javaArgType.constData());
        }

        QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                [connectionInfo, jSignalMethodName, jValue]() {
                    connectionInfo.listener.callMethod<void, jstring, jobject>(
                            "onSignalEmitted", jSignalMethodName.object<jstring>(),
                            jValue.object());
                });
    }
}

QT_END_NAMESPACE
