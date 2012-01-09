/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVETRANSITION_H
#define QDECLARATIVETRANSITION_H

#include "QtQuick1/private/qdeclarativestate_p.h"

#include <QtDeclarative/qdeclarative.h>

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDeclarative1AbstractAnimation;
class QDeclarative1TransitionPrivate;
class QDeclarative1TransitionManager;
class Q_QTQUICK1_EXPORT QDeclarative1Transition : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1Transition)

    Q_PROPERTY(QString from READ fromState WRITE setFromState NOTIFY fromChanged)
    Q_PROPERTY(QString to READ toState WRITE setToState NOTIFY toChanged)
    Q_PROPERTY(bool reversible READ reversible WRITE setReversible NOTIFY reversibleChanged)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarative1AbstractAnimation> animations READ animations)
    Q_CLASSINFO("DefaultProperty", "animations")
    Q_CLASSINFO("DeferredPropertyNames", "animations")

public:
    QDeclarative1Transition(QObject *parent=0);
    ~QDeclarative1Transition();

    QString fromState() const;
    void setFromState(const QString &);

    QString toState() const;
    void setToState(const QString &);

    bool reversible() const;
    void setReversible(bool);

    QDeclarativeListProperty<QDeclarative1AbstractAnimation> animations();

    void prepare(QDeclarative1StateOperation::ActionList &actions,
                 QList<QDeclarativeProperty> &after,
                 QDeclarative1TransitionManager *end);

    void setReversed(bool r);
    void stop();

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void reversibleChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1Transition)

QT_END_HEADER

#endif // QDECLARATIVETRANSITION_H
