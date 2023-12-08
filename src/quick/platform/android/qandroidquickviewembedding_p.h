// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDQUICKVIEWEMBEDDING_P_H
#define QANDROIDQUICKVIEWEMBEDDING_P_H

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

#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>
#include <QtQuick/qquickview.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_TYPE(StringArray, "[Ljava/lang/String;")

namespace QtAndroidQuickViewEmbedding
{
    bool registerNatives(QJniEnvironment& env);
    void createQuickView(JNIEnv *env, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference, QtJniTypes::StringArray qmlImportPaths);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(createQuickView)
    void setRootObjectProperty(JNIEnv *env, jobject, jlong parentWindowReference,
                               jstring propertyName, jobject value);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(setRootObjectProperty)
    jobject getRootObjectProperty(JNIEnv *env, jobject, jlong parentWindowReference,
                                  jstring propertyName);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(getRootObjectProperty)
    int addRootObjectSignalListener(JNIEnv *env, jobject, jlong parentWindowReference,
                                   jstring signalName, jclass argType, jobject listener);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(addRootObjectSignalListener)
    bool removeRootObjectSignalListener(JNIEnv *env, jobject, jlong parentWindowReference,
                                       jint signalListenerId);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(removeRootObjectSignalListener)

    class SignalHelper : public QObject
    {
        Q_OBJECT
    public:
        struct ListenerInfo
        {
            ListenerInfo() : propertyIndex(-1) { }
            int id;
            QJniObject listener;
            QByteArray javaArgType;
            QByteArray signalSignature;
            int propertyIndex;
        };

        int connectionHandleCounter;
        explicit SignalHelper(QQuickView *parent) : QObject(parent), connectionHandleCounter(0) { }
        QMultiMap<QByteArray, ListenerInfo> listenersMap;
        QHash<int, QMetaObject::Connection> connections;
        void invokeListener(QObject *sender, int senderSignalIndex, QVariant signalValue);

        template<typename JT, typename T>
        inline QJniObject qVariantToJniObject(const QVariant& v) {
            return QJniObject(QtJniTypes::Traits<JT>::className(), get<T>(std::move(v)));
        };

    public slots:
        void forwardSignal();
        void forwardSignal(int);
        void forwardSignal(double);
        void forwardSignal(float);
        void forwardSignal(bool);
        void forwardSignal(QString);
    };
};

QT_END_NAMESPACE

#endif // QANDROIDQUICKVIEWEMBEDDING_P_H
