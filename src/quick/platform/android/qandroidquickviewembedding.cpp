// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qandroidquickviewembedding_p.h>

#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnitypes.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtDelegate, "org/qtproject/qt/android/QtEmbeddedContextDelegate");
Q_DECLARE_JNI_CLASS(QtQuickView, "org/qtproject/qt/android/QtQuickView");
Q_DECLARE_JNI_CLASS(QtWindow, "org/qtproject/qt/android/QtWindow");
Q_DECLARE_JNI_CLASS(View, "android/view/View");

namespace QtAndroidQuickViewEmbedding
{
    void createQuickView(JNIEnv*, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference)
    {
        static_assert (sizeof(jlong) >= sizeof(void*),
                      "Insufficient size of Java type to hold the c++ pointer");
        const QUrl qmlUrl(QJniObject(qmlUri).toString());
        QMetaObject::invokeMethod(qApp, [qtViewObject = QJniObject(nativeWindow),
                                        parentWindowReference,
                                        width,
                                        height,
                                        qmlUrl] {
            QWindow *parentWindow = reinterpret_cast<QWindow *>(parentWindowReference);
            QQuickView* view = new QQuickView(parentWindow);
            view->setSource(qmlUrl);
            view->setColor(QColor(Qt::transparent));
            view->setWidth(width);
            view->setHeight(height);
            const QtJniTypes::QtWindow window = reinterpret_cast<jobject>(view->winId());
            qtViewObject.callMethod<void>("addQtWindow",
                                          window,
                                          reinterpret_cast<jlong>(view),
                                          parentWindowReference);
        });
    }

    bool registerNatives(QJniEnvironment& env) {
        return env.registerNativeMethods(QtJniTypes::Traits<QtJniTypes::QtQuickView>::className(),
                                {Q_JNI_NATIVE_SCOPED_METHOD(createQuickView,
                                                            QtAndroidQuickViewEmbedding)});
    }
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    Q_UNUSED(vm)
    Q_UNUSED(reserved)

    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QJniEnvironment env;
    if (!env.isValid())
        return JNI_ERR;
    if (!QtAndroidQuickViewEmbedding::registerNatives(env))
        return JNI_ERR;
    return JNI_VERSION_1_6;
}

QT_END_NAMESPACE
