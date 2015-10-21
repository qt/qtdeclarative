/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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
    \inqmlmodule Qt.labs.controls
    \ingroup utilities
    \brief An exclusive group of checkable controls.

    ExclusiveGroup is a non-visual, mutually exclusive group of checkable
    controls and objects. It is used with controls such as RadioButton,
    where only one of the options can be selected at a time.

    Any control or object that has a \c checked property, and either a
    \c checkedChanged(), \c toggled(), or \c toggled(bool) signal, can be
    added to an ExclusiveGroup.

    The most straight-forward way to use ExclusiveGroup is to assign
    a list of checkable items. For example, the list of children of a
    \l{Item Positioners}{positioner} or a \l{Qt Quick Layouts}{layout}
    that manages a group of mutually exclusive controls.

    \code
    ExclusiveGroup {
        checkables: column.children
    }

    Column {
        id: column

        RadioButton {
            checked: true
            text: qsTr("DAB")
        }

        RadioButton {
            text: qsTr("FM")
        }

        RadioButton {
            text: qsTr("AM")
        }
    }
    \endcode

    Mutually exclusive controls do not always share the same parent item,
    or the parent layout may sometimes contain items that should not be
    included to the exclusive group. Such cases are best handled using
    the \l group attached property.

    \code
    ExclusiveGroup { id: radioGroup }

    Column {
        Label {
            text: qsTr("Radio:")
        }

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

    More advanced use cases can be handled using the addCheckable() and
    removeCheckable() methods.

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

    static void checkables_append(QQmlListProperty<QObject> *prop, QObject *obj);
    static int checkables_count(QQmlListProperty<QObject> *prop);
    static QObject *checkables_at(QQmlListProperty<QObject> *prop, int index);
    static void checkables_clear(QQmlListProperty<QObject> *prop);

    QObject *current;
    QObjectList checkables;
    QMetaMethod updateCurrentMethod;
};

void QQuickExclusiveGroupPrivate::_q_updateCurrent()
{
    Q_Q(QQuickExclusiveGroup);
    QObject *object = q->sender();
    if (isChecked(object))
        q->setCurrent(object);
}

void QQuickExclusiveGroupPrivate::checkables_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickExclusiveGroup *q = static_cast<QQuickExclusiveGroup *>(prop->object);
    q->addCheckable(obj);
}

int QQuickExclusiveGroupPrivate::checkables_count(QQmlListProperty<QObject> *prop)
{
    QQuickExclusiveGroupPrivate *p = static_cast<QQuickExclusiveGroupPrivate *>(prop->data);
    return p->checkables.count();
}

QObject *QQuickExclusiveGroupPrivate::checkables_at(QQmlListProperty<QObject> *prop, int index)
{
    QQuickExclusiveGroupPrivate *p = static_cast<QQuickExclusiveGroupPrivate *>(prop->data);
    return p->checkables.value(index);
}

void QQuickExclusiveGroupPrivate::checkables_clear(QQmlListProperty<QObject> *prop)
{
    QQuickExclusiveGroupPrivate *p = static_cast<QQuickExclusiveGroupPrivate *>(prop->data);
    if (!p->checkables.isEmpty()) {
        p->checkables.clear();
        QQuickExclusiveGroup *q = static_cast<QQuickExclusiveGroup *>(prop->object);
        q->setCurrent(0);
        emit q->checkablesChanged();
    }
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
    \qmlproperty QtObject Qt.labs.controls::ExclusiveGroup::current

    This property holds the currently selected object or \c null if there is none.

    By default, it is the first checked object added to the exclusive group.
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
    \qmlproperty list<Object> Qt.labs.controls::ExclusiveGroup::checkables
    \default

    This property holds the list of checkables.

    \code
    ExclusiveGroup {
        checkables: column.children
    }

    Column {
        id: column

        RadioButton {
            checked: true
            text: qsTr("Option A")
        }

        RadioButton {
            text: qsTr("Option B")
        }
    }
    \endcode

    \sa group
*/
QQmlListProperty<QObject> QQuickExclusiveGroup::checkables()
{
    Q_D(QQuickExclusiveGroup);
    return QQmlListProperty<QObject>(this, d,
                                     QQuickExclusiveGroupPrivate::checkables_append,
                                     QQuickExclusiveGroupPrivate::checkables_count,
                                     QQuickExclusiveGroupPrivate::checkables_at,
                                     QQuickExclusiveGroupPrivate::checkables_clear);
}

/*!
    \qmlmethod bool Qt.labs.controls::ExclusiveGroup::isCheckable(QtObject object)

    Returns \c true if the \a object is checkable, and \c false otherwise.
*/
bool QQuickExclusiveGroup::isCheckable(QObject *object) const
{
    if (!object)
        return false;

    QVariant checked = object->property(CHECKED_PROPERTY);
    if (!checked.isValid())
        return false;

    QMetaMethod signal = checkableSignal(object);
    return signal.isValid();
}

/*!
    \qmlmethod void Qt.labs.controls::ExclusiveGroup::addCheckable(QtObject object)

    Adds an \a object to the exclusive group.

    \note Manually adding objects to an exclusive group is typically unnecessary.
          The \l checkables property and the \l group attached property provide a
          convenient and declarative syntax.

    \sa checkables, group
*/
void QQuickExclusiveGroup::addCheckable(QObject *object)
{
    Q_D(QQuickExclusiveGroup);
    if (!object || d->checkables.contains(object))
        return;

    QMetaMethod signal = checkableSignal(object);
    if (signal.isValid()) {
        connect(object, signal, this, d->updateCurrentMethod, Qt::UniqueConnection);
        connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(removeCheckable(QObject*)), Qt::UniqueConnection);

        if (isChecked(object))
            setCurrent(object);

        d->checkables.append(object);
        emit checkablesChanged();
    } else {
        qmlInfo(this) << "The object has no checkedChanged() or toggled() signal.";
    }
}

/*!
    \qmlmethod void Qt.labs.controls::ExclusiveGroup::removeCheckable(QtObject object)

    Removes an \a object from the exclusive group.

    \note Manually removing objects from an exclusive group is typically unnecessary.
          The \l checkables property and the \l group attached property provide a
          convenient and declarative syntax.

    \sa checkables, group
*/
void QQuickExclusiveGroup::removeCheckable(QObject *object)
{
    Q_D(QQuickExclusiveGroup);
    if (!object || !d->checkables.contains(object))
        return;

    QMetaMethod signal = checkableSignal(object);
    if (signal.isValid()) {
        if (disconnect(object, signal, this, d->updateCurrentMethod))
            disconnect(object, SIGNAL(destroyed(QObject*)), this, SLOT(removeCheckable(QObject*)));
    }

    if (d->current == object)
        setCurrent(Q_NULLPTR);

    d->checkables.removeOne(object);
    emit checkablesChanged();
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
    \qmlattachedproperty ExclusiveGroup Qt.labs.controls::ExclusiveGroup::group

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

    \sa checkables
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
