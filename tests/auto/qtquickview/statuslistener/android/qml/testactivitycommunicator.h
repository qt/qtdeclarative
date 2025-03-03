// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTACTIVITYCOMMUNICATOR_H
#define TESTACTIVITYCOMMUNICATOR_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlengine.h>
#include <QtCore/qjnitypes.h>

Q_DECLARE_JNI_CLASS(TestActivity,
                    "org/qtproject/qt/android/qtquickview_statuslistener/TestActivity")

class TestActivityCommunicator : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit TestActivityCommunicator(QObject *parent = nullptr);
    ~TestActivityCommunicator();

    static TestActivityCommunicator *instance();

public slots:
    void loadQtQuickView(const QString &qmlUri);

signals:
    void qtQuickViewStatusChanged(int status);

private:
    QtJniTypes::TestActivity m_activity;

    static void jni_onQtQuickViewStatusChanged(JNIEnv *, jclass, jint status);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_onQtQuickViewStatusChanged)
};

#endif // TESTACTIVITYCOMMUNICATOR_H
