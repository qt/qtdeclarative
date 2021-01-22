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

#ifndef QQMLFINALSTATE_H
#define QQMLFINALSTATE_H

#include "childrenprivate.h"
#include "statemachine.h"

#include <QtCore/QFinalState>
#include <QtQml/QQmlListProperty>
#include <QtQml/qqml.h>


QT_BEGIN_NAMESPACE

class FinalState : public QFinalState
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> children READ children NOTIFY childrenChanged)
    Q_CLASSINFO("DefaultProperty", "children")
    QML_ELEMENT

public:
    explicit FinalState(QState *parent = 0);

    QQmlListProperty<QObject> children();

Q_SIGNALS:
    void childrenChanged();

private:
    ChildrenPrivate<FinalState, ChildrenMode::State> m_children;
};

QT_END_NAMESPACE
#endif
