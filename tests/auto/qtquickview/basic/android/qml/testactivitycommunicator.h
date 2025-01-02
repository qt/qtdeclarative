// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTACTIVITYCOMMUNICATOR_H
#define TESTACTIVITYCOMMUNICATOR_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlengine.h>
#include <QtCore/qjnitypes.h>

Q_DECLARE_JNI_CLASS(TestActivity, "org/qtproject/qt/android/tst_qtquickview_basic/TestActivity")
Q_DECLARE_JNI_CLASS(TestView,
                    "org/qtproject/qt/android/tst_qtquickview_basic_qml/TestViewModule/TestView")
class TestActivityCommunicator : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit TestActivityCommunicator(QObject *parent = nullptr);

public slots:
    Q_INVOKABLE bool getBoolProperty(const QString &name);
    Q_INVOKABLE int getIntProperty(const QString &name);
    Q_INVOKABLE double getDoubleProperty(const QString &name);
    Q_INVOKABLE QString getStringProperty(const QString &name);

    void setBoolProperty(const QString &name, bool value);
    void setIntProperty(const QString &name, int value);
    void setDoubleProperty(const QString &name, double value);
    void setStringProperty(const QString &name, const QString &value);

private:
    QtJniTypes::TestActivity m_activity;
    QtJniTypes::TestView m_view;

    QByteArray getPropertyGetterName(const QString &name);
    QByteArray getPropertySetterName(const QString &name);
};

#endif // TESTACTIVITYCOMMUNICATOR_H
