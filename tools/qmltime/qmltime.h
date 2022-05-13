// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTIME_H
#define QMLTIME_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>

class Timer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *component READ component WRITE setComponent)
    QML_ELEMENT

public:
    Timer();

    QQmlComponent *component() const;
    void setComponent(QQmlComponent *);

    static Timer *timerInstance();

    void run(uint);

    bool willParent() const;
    void setWillParent(bool p);

private:
    void runTest(QQmlContext *, uint);

    QQmlComponent *m_component;
    static Timer *m_timer;

    bool m_willparent;
    QQuickView m_view;
    QQuickItem *m_item;
};
QML_DECLARE_TYPE(Timer);

#endif // QMLTIME_H
