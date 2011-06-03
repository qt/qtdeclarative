/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVESTATECHANGE_P_H
#define QDECLARATIVESTATECHANGE_P_H

#include <QObject>
#include <QtScript/qscriptvalue.h>
#include <qdeclarativescriptstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeState;
class QDeclarativeExpression;
class QDeclarativeStateChange : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString toState READ toState WRITE setToState NOTIFY toStateChanged)
    Q_PROPERTY(QDeclarativeScriptString when READ when WRITE setWhen)
    Q_PROPERTY(QDeclarativeScriptString trigger READ trigger WRITE setTrigger)
public:
    explicit QDeclarativeStateChange(QObject *parent = 0);
    ~QDeclarativeStateChange();

    QString toState() const;
    void setToState(QString arg);


    QDeclarativeScriptString when() const;
    void setWhen(QDeclarativeScriptString arg);

    QDeclarativeScriptString trigger() const;
    void setTrigger(QDeclarativeScriptString arg);

    void setState(QDeclarativeState *);
    void setActive(bool active);

Q_SIGNALS:
    void toStateChanged(QString arg);

private Q_SLOTS:
    void updateState();
    void activate();

private:
    void createTrigger();
    QString m_toState;
    QDeclarativeState *m_state;
    QDeclarativeScriptString m_when;
    QDeclarativeExpression *m_expression;
    QDeclarativeScriptString m_trigger;
    QObject *m_triggerObject;
    int m_triggerIdx;
    bool m_triggerDirty;
    static int m_activateIdx;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVESTATECHANGE_P_H
