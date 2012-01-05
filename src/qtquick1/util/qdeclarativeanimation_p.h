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

#ifndef QDECLARATIVEANIMATION_H
#define QDECLARATIVEANIMATION_H

#include "QtQuick1/private/qdeclarativetransition_p.h"
#include "QtQuick1/private/qdeclarativestate_p.h"
#include <QtGui/qvector3d.h>

#include <QtDeclarative/qdeclarativepropertyvaluesource.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativescriptstring.h>

#include <QtCore/qvariant.h>
#include <QtCore/qeasingcurve.h>
#include <QtCore/QAbstractAnimation>
#include <QtGui/qcolor.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeItem;
class QDeclarative1AbstractAnimationPrivate;
class QDeclarative1AnimationGroup;
class Q_QTQUICK1_EXPORT QDeclarative1AbstractAnimation : public QObject, public QDeclarativePropertyValueSource, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1AbstractAnimation)

    Q_INTERFACES(QDeclarativeParserStatus)
    Q_INTERFACES(QDeclarativePropertyValueSource)
    Q_ENUMS(Loops)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool alwaysRunToEnd READ alwaysRunToEnd WRITE setAlwaysRunToEnd NOTIFY alwaysRunToEndChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopCountChanged)
    Q_CLASSINFO("DefaultMethod", "start()")

public:
    QDeclarative1AbstractAnimation(QObject *parent=0);
    virtual ~QDeclarative1AbstractAnimation();

    enum Loops { Infinite = -2 };

    bool isRunning() const;
    void setRunning(bool);
    bool isPaused() const;
    void setPaused(bool);
    bool alwaysRunToEnd() const;
    void setAlwaysRunToEnd(bool);

    int loops() const;
    void setLoops(int);

    int currentTime();
    void setCurrentTime(int);

    QDeclarative1AnimationGroup *group() const;
    void setGroup(QDeclarative1AnimationGroup *);

    void setDefaultTarget(const QDeclarativeProperty &);
    void setDisableUserControl();

    void classBegin();
    void componentComplete();

Q_SIGNALS:
    void started();
    void completed();
    void runningChanged(bool);
    void pausedChanged(bool);
    void alwaysRunToEndChanged(bool);
    void loopCountChanged(int);

public Q_SLOTS:
    void restart();
    void start();
    void pause();
    void resume();
    void stop();
    void complete();

protected:
    QDeclarative1AbstractAnimation(QDeclarative1AbstractAnimationPrivate &dd, QObject *parent);

public:
    enum TransitionDirection { Forward, Backward };
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation() = 0;

private Q_SLOTS:
    void timelineComplete();
    void componentFinalized();
private:
    virtual void setTarget(const QDeclarativeProperty &);
    void notifyRunningChanged(bool running);
    friend class QDeclarative1Behavior;


};

class QDeclarative1PauseAnimationPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1PauseAnimation : public QDeclarative1AbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1PauseAnimation)

    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)

public:
    QDeclarative1PauseAnimation(QObject *parent=0);
    virtual ~QDeclarative1PauseAnimation();

    int duration() const;
    void setDuration(int);

Q_SIGNALS:
    void durationChanged(int);

protected:
    virtual QAbstractAnimation *qtAnimation();
};

class QDeclarative1ScriptActionPrivate;
class Q_QTQUICK1_EXPORT QDeclarative1ScriptAction : public QDeclarative1AbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1ScriptAction)

    Q_PROPERTY(QDeclarativeScriptString script READ script WRITE setScript)
    Q_PROPERTY(QString scriptName READ stateChangeScriptName WRITE setStateChangeScriptName)

public:
    QDeclarative1ScriptAction(QObject *parent=0);
    virtual ~QDeclarative1ScriptAction();

    QDeclarativeScriptString script() const;
    void setScript(const QDeclarativeScriptString &);

    QString stateChangeScriptName() const;
    void setStateChangeScriptName(const QString &);

protected:
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation();
};

class QDeclarative1PropertyActionPrivate;
class QDeclarative1PropertyAction : public QDeclarative1AbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1PropertyAction)

    Q_PROPERTY(QObject *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(QString properties READ properties WRITE setProperties NOTIFY propertiesChanged)
    Q_PROPERTY(QDeclarativeListProperty<QObject> targets READ targets)
    Q_PROPERTY(QDeclarativeListProperty<QObject> exclude READ exclude)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

public:
    QDeclarative1PropertyAction(QObject *parent=0);
    virtual ~QDeclarative1PropertyAction();

    QObject *target() const;
    void setTarget(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QString properties() const;
    void setProperties(const QString &);

    QDeclarativeListProperty<QObject> targets();
    QDeclarativeListProperty<QObject> exclude();

    QVariant value() const;
    void setValue(const QVariant &);

Q_SIGNALS:
    void valueChanged(const QVariant &);
    void propertiesChanged(const QString &);
    void targetChanged();
    void propertyChanged();

protected:
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation();
};

class QDeclarative1PropertyAnimationPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1PropertyAnimation : public QDeclarative1AbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1PropertyAnimation)

    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QVariant from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(QVariant to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)
    Q_PROPERTY(QObject *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(QString properties READ properties WRITE setProperties NOTIFY propertiesChanged)
    Q_PROPERTY(QDeclarativeListProperty<QObject> targets READ targets)
    Q_PROPERTY(QDeclarativeListProperty<QObject> exclude READ exclude)

public:
    QDeclarative1PropertyAnimation(QObject *parent=0);
    virtual ~QDeclarative1PropertyAnimation();

    virtual int duration() const;
    virtual void setDuration(int);

    QVariant from() const;
    void setFrom(const QVariant &);

    QVariant to() const;
    void setTo(const QVariant &);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

    QObject *target() const;
    void setTarget(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QString properties() const;
    void setProperties(const QString &);

    QDeclarativeListProperty<QObject> targets();
    QDeclarativeListProperty<QObject> exclude();

protected:
    QDeclarative1PropertyAnimation(QDeclarative1PropertyAnimationPrivate &dd, QObject *parent);
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation();

Q_SIGNALS:
    void durationChanged(int);
    void fromChanged(QVariant);
    void toChanged(QVariant);
    void easingChanged(const QEasingCurve &);
    void propertiesChanged(const QString &);
    void targetChanged();
    void propertyChanged();
};

class Q_AUTOTEST_EXPORT QDeclarative1ColorAnimation : public QDeclarative1PropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1PropertyAnimation)
    Q_PROPERTY(QColor from READ from WRITE setFrom)
    Q_PROPERTY(QColor to READ to WRITE setTo)

public:
    QDeclarative1ColorAnimation(QObject *parent=0);
    virtual ~QDeclarative1ColorAnimation();

    QColor from() const;
    void setFrom(const QColor &);

    QColor to() const;
    void setTo(const QColor &);
};

class Q_AUTOTEST_EXPORT QDeclarative1NumberAnimation : public QDeclarative1PropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1PropertyAnimation)

    Q_PROPERTY(qreal from READ from WRITE setFrom)
    Q_PROPERTY(qreal to READ to WRITE setTo)

public:
    QDeclarative1NumberAnimation(QObject *parent=0);
    virtual ~QDeclarative1NumberAnimation();

    qreal from() const;
    void setFrom(qreal);

    qreal to() const;
    void setTo(qreal);

protected:
    QDeclarative1NumberAnimation(QDeclarative1PropertyAnimationPrivate &dd, QObject *parent);

private:
    void init();
};

class Q_AUTOTEST_EXPORT QDeclarative1Vector3dAnimation : public QDeclarative1PropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1PropertyAnimation)

    Q_PROPERTY(QVector3D from READ from WRITE setFrom)
    Q_PROPERTY(QVector3D to READ to WRITE setTo)

public:
    QDeclarative1Vector3dAnimation(QObject *parent=0);
    virtual ~QDeclarative1Vector3dAnimation();

    QVector3D from() const;
    void setFrom(QVector3D);

    QVector3D to() const;
    void setTo(QVector3D);
};

class QDeclarative1RotationAnimationPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1RotationAnimation : public QDeclarative1PropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1RotationAnimation)
    Q_ENUMS(RotationDirection)

    Q_PROPERTY(qreal from READ from WRITE setFrom)
    Q_PROPERTY(qreal to READ to WRITE setTo)
    Q_PROPERTY(RotationDirection direction READ direction WRITE setDirection NOTIFY directionChanged)

public:
    QDeclarative1RotationAnimation(QObject *parent=0);
    virtual ~QDeclarative1RotationAnimation();

    qreal from() const;
    void setFrom(qreal);

    qreal to() const;
    void setTo(qreal);

    enum RotationDirection { Numerical, Shortest, Clockwise, Counterclockwise };
    RotationDirection direction() const;
    void setDirection(RotationDirection direction);

Q_SIGNALS:
    void directionChanged();
};

class QDeclarative1AnimationGroupPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1AnimationGroup : public QDeclarative1AbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1AnimationGroup)

    Q_CLASSINFO("DefaultProperty", "animations")
    Q_PROPERTY(QDeclarativeListProperty<QDeclarative1AbstractAnimation> animations READ animations)

public:
    QDeclarative1AnimationGroup(QObject *parent);
    virtual ~QDeclarative1AnimationGroup();

    QDeclarativeListProperty<QDeclarative1AbstractAnimation> animations();
    friend class QDeclarative1AbstractAnimation;

protected:
    QDeclarative1AnimationGroup(QDeclarative1AnimationGroupPrivate &dd, QObject *parent);
};

class QDeclarative1SequentialAnimation : public QDeclarative1AnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1AnimationGroup)

public:
    QDeclarative1SequentialAnimation(QObject *parent=0);
    virtual ~QDeclarative1SequentialAnimation();

protected:
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation();
};

class QDeclarative1ParallelAnimation : public QDeclarative1AnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1AnimationGroup)

public:
    QDeclarative1ParallelAnimation(QObject *parent=0);
    virtual ~QDeclarative1ParallelAnimation();

protected:
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation();
};

class QDeclarative1ParentAnimationPrivate;
class QDeclarative1ParentAnimation : public QDeclarative1AnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1ParentAnimation)

    Q_PROPERTY(QDeclarativeItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QDeclarativeItem *newParent READ newParent WRITE setNewParent NOTIFY newParentChanged)
    Q_PROPERTY(QDeclarativeItem *via READ via WRITE setVia NOTIFY viaChanged)

public:
    QDeclarative1ParentAnimation(QObject *parent=0);
    virtual ~QDeclarative1ParentAnimation();

    QDeclarativeItem *target() const;
    void setTarget(QDeclarativeItem *);

    QDeclarativeItem *newParent() const;
    void setNewParent(QDeclarativeItem *);

    QDeclarativeItem *via() const;
    void setVia(QDeclarativeItem *);

Q_SIGNALS:
    void targetChanged();
    void newParentChanged();
    void viaChanged();

protected:
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation();
};

class QDeclarative1AnchorAnimationPrivate;
class QDeclarative1AnchorAnimation : public QDeclarative1AbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1AnchorAnimation)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarativeItem> targets READ targets)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)

public:
    QDeclarative1AnchorAnimation(QObject *parent=0);
    virtual ~QDeclarative1AnchorAnimation();

    QDeclarativeListProperty<QDeclarativeItem> targets();

    int duration() const;
    void setDuration(int);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

Q_SIGNALS:
    void durationChanged(int);
    void easingChanged(const QEasingCurve&);

protected:
    virtual void transition(QDeclarative1StateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
    virtual QAbstractAnimation *qtAnimation();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1AbstractAnimation)
QML_DECLARE_TYPE(QDeclarative1PauseAnimation)
QML_DECLARE_TYPE(QDeclarative1ScriptAction)
QML_DECLARE_TYPE(QDeclarative1PropertyAction)
QML_DECLARE_TYPE(QDeclarative1PropertyAnimation)
QML_DECLARE_TYPE(QDeclarative1ColorAnimation)
QML_DECLARE_TYPE(QDeclarative1NumberAnimation)
QML_DECLARE_TYPE(QDeclarative1SequentialAnimation)
QML_DECLARE_TYPE(QDeclarative1ParallelAnimation)
QML_DECLARE_TYPE(QDeclarative1Vector3dAnimation)
QML_DECLARE_TYPE(QDeclarative1RotationAnimation)
QML_DECLARE_TYPE(QDeclarative1ParentAnimation)
QML_DECLARE_TYPE(QDeclarative1AnchorAnimation)

QT_END_HEADER

#endif // QDECLARATIVEANIMATION_H
