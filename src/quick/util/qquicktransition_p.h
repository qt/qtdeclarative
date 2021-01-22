/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#ifndef QQUICKTRANSITION_H
#define QQUICKTRANSITION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickstate_p.h"
#include <private/qabstractanimationjob_p.h>
#include <private/qqmlguard_p.h>
#include <qqml.h>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickTransitionPrivate;
class QQuickTransitionManager;
class QQuickTransition;

class QQuickTransitionInstance : QAnimationJobChangeListener
{
public:
    QQuickTransitionInstance(QQuickTransition *transition, QAbstractAnimationJob *anim);
    ~QQuickTransitionInstance();

    void start();
    void stop();

    bool isRunning() const;

protected:
    void animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State, QAbstractAnimationJob::State) override;

    void removeStateChangeListener()
    {
        m_anim->removeAnimationChangeListener(this, QAbstractAnimationJob::StateChange);
    }

private:
    QQmlGuard<QQuickTransition> m_transition;
    QAbstractAnimationJob *m_anim;
    friend class QQuickTransition;
};

class Q_QUICK_PRIVATE_EXPORT QQuickTransition : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickTransition)

    Q_PROPERTY(QString from READ fromState WRITE setFromState NOTIFY fromChanged)
    Q_PROPERTY(QString to READ toState WRITE setToState NOTIFY toChanged)
    Q_PROPERTY(bool reversible READ reversible WRITE setReversible NOTIFY reversibleChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QQmlListProperty<QQuickAbstractAnimation> animations READ animations)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_CLASSINFO("DefaultProperty", "animations")
    Q_CLASSINFO("DeferredPropertyNames", "animations")
    QML_NAMED_ELEMENT(Transition)

public:
    QQuickTransition(QObject *parent=nullptr);
    ~QQuickTransition();

    QString fromState() const;
    void setFromState(const QString &);

    QString toState() const;
    void setToState(const QString &);

    bool reversible() const;
    void setReversible(bool);

    bool enabled() const;
    void setEnabled(bool enabled);

    bool running() const;

    QQmlListProperty<QQuickAbstractAnimation> animations();

    QQuickTransitionInstance *prepare(QQuickStateOperation::ActionList &actions,
                 QList<QQmlProperty> &after,
                 QQuickTransitionManager *end,
                 QObject *defaultTarget);

    void setReversed(bool r);

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void reversibleChanged();
    void enabledChanged();
    void runningChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTransition)

#endif // QQUICKTRANSITION_H
