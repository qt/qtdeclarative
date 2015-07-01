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
#include <QtCore/qmetaobject.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ExclusiveGroup
    \inherits QtObject
    \instantiates QQuickExclusiveGroup
    \inqmlmodule QtQuick.Controls
    \ingroup utilities
    \brief An exclusive group of checkable controls.

    ExclusiveGroup is a non-visual, mutually exclusive group of checkable
    controls and objects. It is used with controls like RadioButton, where
    only one of options can be selected at a time.

    Any control or object that has a \c checked property, and either a
    \c checkedChanged(), \c toggled(), or \c toggled(bool) signal, can be
    attached to an ExclusiveGroup.

    \code
    Column {
        ExclusiveGroup { id: radioGroup }

        RadioButton {
            checked: true
            text: qsTr("DAB")
            ExclusiveGroup.group: radioGroup
        }

        RadioButton {
            text: qsTr("FM")
            ExclusiveGroup.group: radioGroup
        }

        RadioButton {
            text: qsTr("AM")
            ExclusiveGroup.group: radioGroup
        }
    }
    \endcode

    \sa RadioButton
*/

#define CHECKED_PROPERTY "checked"

static const char *checkableSignals[] = {
    CHECKED_PROPERTY"Changed()",
    "toggled(bool)",
    "toggled()",
    0
};

static QMetaMethod checkableSignal(QObject *object)
{
    const QMetaObject *mo = object->metaObject();
    for (const char **signal = checkableSignals; *signal; ++signal) {
        int index = mo->indexOfSignal(*signal);
        if (index != -1)
            return mo->method(index);
    }
    return QMetaMethod();
}

static bool isChecked(const QObject *object)
{
    if (!object)
        return false;
    QVariant checked = object->property(CHECKED_PROPERTY);
    return checked.isValid() && checked.toBool();
}

class QQuickExclusiveGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickExclusiveGroup)

public:
    QQuickExclusiveGroupPrivate() : current(Q_NULLPTR) { }

    void _q_updateCurrent();

    QObject *current;
    QMetaMethod updateCurrentMethod;
};

void QQuickExclusiveGroupPrivate::_q_updateCurrent()
{
    Q_Q(QQuickExclusiveGroup);
    QObject *object = q->sender();
    if (isChecked(object))
        q->setCurrent(object);
}

QQuickExclusiveGroup::QQuickExclusiveGroup(QObject *parent)
    : QObject(*(new QQuickExclusiveGroupPrivate), parent)
{
    Q_D(QQuickExclusiveGroup);
    int index = metaObject()->indexOfMethod("_q_updateCurrent()");
    d->updateCurrentMethod = metaObject()->method(index);
}

QQuickExclusiveGroupAttached *QQuickExclusiveGroup::qmlAttachedProperties(QObject *object)
{
    return new QQuickExclusiveGroupAttached(object);
}

/*!
    \qmlproperty QtObject QtQuickControls2::ExclusiveGroup::current

    This property holds the currently selected object or \c null if there is none.

    By default, it is the first checked object attached to the exclusive group.
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
            d->current->setProperty(CHECKED_PROPERTY, false);
        d->current = current;
        if (current)
            current->setProperty(CHECKED_PROPERTY, true);
        emit currentChanged();
    }
}

/*!
    \qmlmethod void QtQuickControls2::ExclusiveGroup::addCheckable(QtObject object)

    Adds an \a object to the exclusive group.

    \note Manually adding objects to an exclusive group is typically unnecessary.
          The \l group attached property provides a convenient and declarative syntax.

    \sa group
*/
void QQuickExclusiveGroup::addCheckable(QObject *object)
{
    Q_D(QQuickExclusiveGroup);
    if (!object)
        return;

    QMetaMethod signal = checkableSignal(object);
    if (signal.isValid()) {
        connect(object, signal, this, d->updateCurrentMethod, Qt::UniqueConnection);
        connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(removeCheckable(QObject*)), Qt::UniqueConnection);

        if (isChecked(object))
            setCurrent(object);
    } else {
        qmlInfo(this) << "The object has no checkedChanged() or toggled() signal.";
    }
}

/*!
    \qmlmethod void QtQuickControls2::ExclusiveGroup::removeCheckable(QtObject object)

    Removes an \a object from the exclusive group.

    \note Manually removing objects from an exclusive group is typically unnecessary.
          The \l group attached property provides a convenient and declarative syntax.

    \sa group
*/
void QQuickExclusiveGroup::removeCheckable(QObject *object)
{
    Q_D(QQuickExclusiveGroup);
    if (!object)
        return;

    QMetaMethod signal = checkableSignal(object);
    if (signal.isValid()) {
        if (disconnect(object, signal, this, d->updateCurrentMethod))
            disconnect(object, SIGNAL(destroyed(QObject*)), this, SLOT(removeCheckable(QObject*)));
    }

    if (d->current == object)
        setCurrent(Q_NULLPTR);
}

class QQuickExclusiveGroupAttachedPrivate : public QObjectPrivate
{
public:
    QQuickExclusiveGroupAttachedPrivate() : group(Q_NULLPTR) { }

    QQuickExclusiveGroup *group;
};

QQuickExclusiveGroupAttached::QQuickExclusiveGroupAttached(QObject *parent) :
    QObject(*(new QQuickExclusiveGroupAttachedPrivate), parent)
{
}

/*!
    \qmlattachedproperty ExclusiveGroup QtQuickControls2::ExclusiveGroup::group

    This property attaches a checkable control or object to an exclusive group.

    \code
    ExclusiveGroup { id: group }

    RadioButton {
        checked: true
        text: qsTr("Option A")
        ExclusiveGroup.group: group
    }

    RadioButton {
        text: qsTr("Option B")
        ExclusiveGroup.group: group
    }
    \endcode
*/
QQuickExclusiveGroup *QQuickExclusiveGroupAttached::group() const
{
    Q_D(const QQuickExclusiveGroupAttached);
    return d->group;
}

void QQuickExclusiveGroupAttached::setGroup(QQuickExclusiveGroup *group)
{
    Q_D(QQuickExclusiveGroupAttached);
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
