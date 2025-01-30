// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTACTIVITYCOMMUNICATOR_H
#define TESTACTIVITYCOMMUNICATOR_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlengine.h>
#include <QtCore/qjnitypes.h>

Q_DECLARE_JNI_CLASS(TestActivity,
                    "org/qtproject/qt/android/qtquickview_signallistener/TestActivity")
Q_DECLARE_JNI_CLASS(TestView,
                    "org/qtproject/qt/android/qtquickview_signallistener_qml/"
                    "tst_qtquickview_signallistener_qml/TestViewModule/TestView")
class TestActivityCommunicator : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit TestActivityCommunicator(QObject *parent = nullptr);
    ~TestActivityCommunicator();

    static TestActivityCommunicator *instance();

signals:
    void basicSignal();
    void intSignal(int value);
    void boolSignal(bool value);
    void doubleSignal(double value);
    void stringSignal(QString value);

private:
    QtJniTypes::TestActivity m_activity;
    QtJniTypes::TestView m_view;

    static void onBasicSignal(JNIEnv *, jclass);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(onBasicSignal)
    static void onIntSignal(JNIEnv *, jclass, QtJniTypes::Integer value);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(onIntSignal)
    static void onBoolSignal(JNIEnv *, jclass, QtJniTypes::Boolean value);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(onBoolSignal)
    static void onDoubleSignal(JNIEnv *, jclass, QtJniTypes::Double value);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(onDoubleSignal)
    static void onStringSignal(JNIEnv *, jclass, QtJniTypes::String value);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(onStringSignal)
};

#endif // TESTACTIVITYCOMMUNICATOR_H
