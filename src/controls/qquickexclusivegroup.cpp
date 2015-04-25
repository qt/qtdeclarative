/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickexclusivegroup_p.h"
#include <QtCore/private/qobject_p.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ExclusiveGroup
    \inherits QtObject
    \instantiates QQuickExclusiveGroup
    \inqmlmodule QtQuick.Controls
    \ingroup utilities
    \brief A non-visual mutually exclusive group of checkable controls.

    TODO
*/

class QQuickExclusiveGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickExclusiveGroup)

public:
    QQuickExclusiveGroupPrivate() : current(Q_NULLPTR) { }

    void _q_updateCurrent();

    QObject *current;
};

void QQuickExclusiveGroupPrivate::_q_updateCurrent()
{
    Q_Q(QQuickExclusiveGroup);
    QObject *object = q->sender();
    if (object->property("checked").toBool())
        q->setCurrent(object);
}

QQuickExclusiveGroup::QQuickExclusiveGroup(QObject *parent)
    : QObject(*(new QQuickExclusiveGroupPrivate), parent)
{
}

/*!
    \qmlproperty QtObject QtQuickControls2::ExclusiveGroup::current

    TODO
*/
QObject *QQuickExclusiveGroup::current() const
{
    Q_D(const QQuickExclusiveGroup);
    return d->current;
}

void QQuickExclusiveGroup::setCurrent(QObject *current)
{
    Q_D(QQuickExclusiveGroup);
    if (d->current != current) {
        if (d->current)
            d->current->setProperty("checked", false);
        d->current = current;
        if (current)
            current->setProperty("checked", true);
        emit currentChanged();
    }
}

/*!
    \qmlmethod void QtQuickControls2::ExclusiveGroup::addCheckable(QtObject object)

    TODO
*/
void QQuickExclusiveGroup::addCheckable(QObject *object)
{
    if (!object)
        return;

    connect(object, SIGNAL(checkedChanged()), this, SLOT(_q_updateCurrent()), Qt::UniqueConnection);
    connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(removeCheckable(QObject*)), Qt::UniqueConnection);

    if (object->property("checked").toBool())
        setCurrent(object);
}

/*!
    \qmlmethod void QtQuickControls2::ExclusiveGroup::removeCheckable(QtObject object)

    TODO
*/
void QQuickExclusiveGroup::removeCheckable(QObject *object)
{
    Q_D(QQuickExclusiveGroup);
    if (!object)
        return;

    if (object->metaObject()->indexOfProperty("checked") != -1)
        disconnect(object, SIGNAL(checkedChanged()), this, SLOT(_q_updateCurrent()));
    disconnect(object, SIGNAL(destroyed(QObject*)), this, SLOT(removeCheckable(QObject*)));

    if (d->current == object)
        setCurrent(Q_NULLPTR);
}

/*!
    \qmltype Exclusive
    \inherits QtObject
    \instantiates QQuickExclusive
    \inqmlmodule QtQuick.Controls
    \ingroup utilities
    \brief TODO

    TODO
*/

class QQuickExclusiveAttachedPrivate : public QObjectPrivate
{
public:
    QQuickExclusiveAttachedPrivate() : group(Q_NULLPTR) { }

    QQuickExclusiveGroup *group;
};

QQuickExclusiveAttached::QQuickExclusiveAttached(QObject *parent) :
    QObject(*(new QQuickExclusiveAttachedPrivate), parent)
{
}

QQuickExclusiveAttached *QQuickExclusiveAttached::qmlAttachedProperties(QObject *object)
{
    return new QQuickExclusiveAttached(object);
}

/*!
    \qmlattachedproperty ExclusiveGroup QtQuickControls2::Exclusive::group

    TODO
*/
QQuickExclusiveGroup *QQuickExclusiveAttached::group() const
{
    Q_D(const QQuickExclusiveAttached);
    return d->group;
}

void QQuickExclusiveAttached::setGroup(QQuickExclusiveGroup *group)
{
    Q_D(QQuickExclusiveAttached);
    if (d->group != group) {
        if (d->group)
            d->group->removeCheckable(parent());
        d->group = group;
        if (group)
            group->addCheckable(parent());
        emit groupChanged();
    }
}

QT_END_NAMESPACE

#include "moc_qquickexclusivegroup_p.cpp"
