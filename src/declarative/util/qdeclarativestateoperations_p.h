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

#ifndef QDECLARATIVESTATEOPERATIONS_H
#define QDECLARATIVESTATEOPERATIONS_H

#include "qdeclarativestate_p.h"
#include <qdeclarativescriptstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QDeclarativeStateChangeScriptPrivate;
class Q_AUTOTEST_EXPORT QDeclarativeStateChangeScript : public QDeclarativeStateOperation, public QDeclarativeActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarativeStateChangeScript)

    Q_PROPERTY(QDeclarativeScriptString script READ script WRITE setScript)
    Q_PROPERTY(QString name READ name WRITE setName)

public:
    QDeclarativeStateChangeScript(QObject *parent=0);
    ~QDeclarativeStateChangeScript();

    virtual ActionList actions();

    virtual QString typeName() const;

    QDeclarativeScriptString script() const;
    void setScript(const QDeclarativeScriptString &);
    
    QString name() const;
    void setName(const QString &);

    virtual void execute(Reason reason = ActualChange);
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeStateChangeScript)

QT_END_HEADER

#endif // QDECLARATIVESTATEOPERATIONS_H
