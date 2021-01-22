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

#ifndef TIMEOUTTRANSITION_H
#define TIMEOUTTRANSITION_H

#include <QtCore/QSignalTransition>
#include <QtQml/QQmlParserStatus>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE
class QTimer;

class TimeoutTransition : public QSignalTransition, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged)
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

public:
    TimeoutTransition(QState *parent = nullptr);
    ~TimeoutTransition();

    int timeout() const;
    void setTimeout(int timeout);

    void classBegin() override {}
    void componentComplete() override;

Q_SIGNALS:
    void timeoutChanged();

private:
    QTimer *m_timer;
};

QT_END_NAMESPACE

#endif
