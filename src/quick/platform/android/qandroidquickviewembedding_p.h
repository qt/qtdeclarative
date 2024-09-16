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

#include <QtQuick/private/qandroidviewsignalmanager_p.h>

#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>
#include <QtQuick/qquickview.h>

QT_BEGIN_NAMESPACE

namespace QtAndroidQuickViewEmbedding
{
    bool registerNatives(QJniEnvironment& env);
    void createQuickView(JNIEnv *env, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference, jlong viewReference,
                         const QJniArray<jstring> &qmlImportPaths);
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

    class QAndroidQuickView : public QQuickView
    {
        Q_OBJECT
        std::unique_ptr<QAndroidViewSignalManager> m_signalManager;

    public:
        explicit QAndroidQuickView(QWindow *parent)
            : QQuickView(parent), m_signalManager(new QAndroidViewSignalManager())
        {
        }
        inline QAndroidViewSignalManager *signalManager() const { return m_signalManager.get(); };
    };
};

QT_END_NAMESPACE

#endif // QANDROIDQUICKVIEWEMBEDDING_P_H
