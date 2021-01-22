/****************************************************************************
**
** Copyright (C) 2016 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "childrenprivate.h"

#include <QtCore/QStateMachine>
#include <QtQml/QQmlParserStatus>
#include <QtQml/QQmlListProperty>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class StateMachine : public QStateMachine, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> children READ children NOTIFY childrenChanged)

    // Override to delay execution after componentComplete()
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY qmlRunningChanged)

    Q_CLASSINFO("DefaultProperty", "children")
    QML_ELEMENT

public:
    explicit StateMachine(QObject *parent = 0);

    void classBegin() override {}
    void componentComplete() override;
    QQmlListProperty<QObject> children();

    bool isRunning() const;
    void setRunning(bool running);

private Q_SLOTS:
    void checkChildMode();

Q_SIGNALS:
    void childrenChanged();
    /*!
     * \internal
     */
    void qmlRunningChanged();

private:
    ChildrenPrivate<StateMachine, ChildrenMode::StateOrTransition> m_children;
    bool m_completed;
    bool m_running;
};

QT_END_NAMESPACE

#endif
