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

namespace QtAndroidQuickViewEmbedding
{
    bool registerNatives(QJniEnvironment& env);
    void createQuickView(JNIEnv *env, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(createQuickView)
    void setRootObjectProperty(JNIEnv *env, jobject, jlong parentWindowReference,
                               jstring propertyName, jobject value);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(setRootObjectProperty)
};

QT_END_NAMESPACE

#endif // QANDROIDQUICKVIEWEMBEDDING_P_H
