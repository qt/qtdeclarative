/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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
