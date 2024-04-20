// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qandroidviewsignalmanager_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(Void, "java/lang/Void");
Q_DECLARE_JNI_CLASS(Integer, "java/lang/Integer");
Q_DECLARE_JNI_CLASS(Double, "java/lang/Double");
Q_DECLARE_JNI_CLASS(Float, "java/lang/Float");
Q_DECLARE_JNI_CLASS(Boolean, "java/lang/Boolean");

void QAndroidViewSignalManager::forwardSignal()
{
    invokeListener(sender(), senderSignalIndex(), QVariant());
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

        int valueTypeId = signalValue.typeId();
        QJniObject jValue;

        switch (valueTypeId) {
        case QMetaType::Type::UnknownType:
            break;
        case QMetaType::Type::Int:
            jValue = qVariantToJniObject<Integer, jint>(signalValue);
            break;
        case QMetaType::Type::Double:
            jValue = qVariantToJniObject<Double, jdouble>(signalValue);
            break;
        case QMetaType::Type::Float:
            jValue = qVariantToJniObject<Float, jfloat>(signalValue);
            break;
        case QMetaType::Type::Bool:
            jValue = qVariantToJniObject<Boolean, jboolean>(signalValue);
            break;
        case QMetaType::Type::QString:
            jValue = QJniObject::fromString(get<QString>(std::move(signalValue)));
            break;
        default:
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
