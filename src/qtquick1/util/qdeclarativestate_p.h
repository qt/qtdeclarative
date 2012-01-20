/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef QDECLARATIVESTATE_H
#define QDECLARATIVESTATE_H

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeproperty.h>
#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeAbstractBinding;
class QDeclarativeBinding;
class QDeclarativeExpression;


class QDeclarative1ActionEvent;
class Q_QTQUICK1_EXPORT QDeclarative1Action
{
public:
    QDeclarative1Action();
    QDeclarative1Action(QObject *, const QString &, const QVariant &);
    QDeclarative1Action(QObject *, const QString &,
                       QDeclarativeContext *, const QVariant &);

    bool restore:1;
    bool actionDone:1;
    bool reverseEvent:1;
    bool deletableToBinding:1;

    QDeclarativeProperty property;
    QVariant fromValue;
    QVariant toValue;

    QDeclarativeAbstractBinding *fromBinding;
    QWeakPointer<QDeclarativeAbstractBinding> toBinding;
    QDeclarative1ActionEvent *event;

    //strictly for matching
    QObject *specifiedObject;
    QString specifiedProperty;

    void deleteFromBinding();
};

class Q_AUTOTEST_EXPORT QDeclarative1ActionEvent
{
public:
    virtual ~QDeclarative1ActionEvent();
    virtual QString typeName() const;

    enum Reason { ActualChange, FastForward };

    virtual void execute(Reason reason = ActualChange);
    virtual bool isReversable();
    virtual void reverse(Reason reason = ActualChange);
    virtual void saveOriginals() {}
    virtual bool needsCopy() { return false; }
    virtual void copyOriginals(QDeclarative1ActionEvent *) {}

    virtual bool isRewindable() { return isReversable(); }
    virtual void rewind() {}
    virtual void saveCurrentValues() {}
    virtual void saveTargetValues() {}

    virtual bool changesBindings();
    virtual void clearBindings();
    virtual bool override(QDeclarative1ActionEvent*other);
};

//### rename to QDeclarative1StateChange?
class QDeclarative1StateGroup;
class QDeclarative1State;
class QDeclarative1StateOperationPrivate;
class Q_QTQUICK1_EXPORT QDeclarative1StateOperation : public QObject
{
    Q_OBJECT
public:
    QDeclarative1StateOperation(QObject *parent = 0)
        : QObject(parent) {}
    typedef QList<QDeclarative1Action> ActionList;

    virtual ActionList actions();

    QDeclarative1State *state() const;
    void setState(QDeclarative1State *state);

protected:
    QDeclarative1StateOperation(QObjectPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(QDeclarative1StateOperation)
    Q_DISABLE_COPY(QDeclarative1StateOperation)
};

typedef QDeclarative1StateOperation::ActionList QDeclarative1StateActions;

class QDeclarative1Transition;
class QDeclarative1StatePrivate;
class Q_QTQUICK1_EXPORT QDeclarative1State : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QDeclarativeBinding *when READ when WRITE setWhen)
    Q_PROPERTY(QString extend READ extends WRITE setExtends)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarative1StateOperation> changes READ changes)
    Q_CLASSINFO("DefaultProperty", "changes")
    Q_CLASSINFO("DeferredPropertyNames", "changes")

public:
    QDeclarative1State(QObject *parent=0);
    virtual ~QDeclarative1State();

    QString name() const;
    void setName(const QString &);
    bool isNamed() const;

    /*'when' is a QDeclarativeBinding to limit state changes oscillation
     due to the unpredictable order of evaluation of bound expressions*/
    bool isWhenKnown() const;
    QDeclarativeBinding *when() const;
    void setWhen(QDeclarativeBinding *);

    QString extends() const;
    void setExtends(const QString &);

    QDeclarativeListProperty<QDeclarative1StateOperation> changes();
    int operationCount() const;
    QDeclarative1StateOperation *operationAt(int) const;

    QDeclarative1State &operator<<(QDeclarative1StateOperation *);

    void apply(QDeclarative1StateGroup *, QDeclarative1Transition *, QDeclarative1State *revert);
    void cancel();

    QDeclarative1StateGroup *stateGroup() const;
    void setStateGroup(QDeclarative1StateGroup *);

    bool containsPropertyInRevertList(QObject *target, const QString &name) const;
    bool changeValueInRevertList(QObject *target, const QString &name, const QVariant &revertValue);
    bool changeBindingInRevertList(QObject *target, const QString &name, QDeclarativeAbstractBinding *binding);
    bool removeEntryFromRevertList(QObject *target, const QString &name);
    void addEntryToRevertList(const QDeclarative1Action &action);
    void removeAllEntriesFromRevertList(QObject *target);
    void addEntriesToRevertList(const QList<QDeclarative1Action> &actions);
    QVariant valueInRevertList(QObject *target, const QString &name) const;
    QDeclarativeAbstractBinding *bindingInRevertList(QObject *target, const QString &name) const;

    bool isStateActive() const;

Q_SIGNALS:
    void completed();

private:
    Q_DECLARE_PRIVATE(QDeclarative1State)
    Q_DISABLE_COPY(QDeclarative1State)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1StateOperation)
QML_DECLARE_TYPE(QDeclarative1State)

QT_END_HEADER

#endif // QDECLARATIVESTATE_H
