/****************************************************************************
**
** Copyright (C) 2016 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLCHILDRENPRIVATE_H
#define QQMLCHILDRENPRIVATE_H

#include <QAbstractState>
#include <QAbstractTransition>
#include <QStateMachine>
#include <QQmlInfo>
#include <QQmlListProperty>

enum class ChildrenMode {
    None              = 0x0,
    State             = 0x1,
    Transition        = 0x2,
    StateOrTransition = State | Transition
};

template<typename T>
static T *parentObject(QQmlListProperty<QObject> *prop) { return static_cast<T *>(prop->object); }

template<class T, ChildrenMode Mode>
struct ParentHandler
{
    static bool unparentItem(QQmlListProperty<QObject> *prop, QObject *oldItem);
    static bool parentItem(QQmlListProperty<QObject> *prop, QObject *item);
};

template<class T>
struct ParentHandler<T, ChildrenMode::None>
{
    static bool unparentItem(QQmlListProperty<QObject> *, QObject *) { return true; }
    static bool parentItem(QQmlListProperty<QObject> *, QObject *) { return true; }
};

template<class T>
struct ParentHandler<T, ChildrenMode::State>
{
    static bool parentItem(QQmlListProperty<QObject> *prop, QObject *item)
    {
        if (QAbstractState *state = qobject_cast<QAbstractState *>(item)) {
            state->setParent(parentObject<T>(prop));
            return true;
        }
        return false;
    }

    static bool unparentItem(QQmlListProperty<QObject> *, QObject *oldItem)
    {
        if (QAbstractState *state = qobject_cast<QAbstractState *>(oldItem)) {
            state->setParent(nullptr);
            return true;
        }
        return false;
    }
};

template<class T>
struct ParentHandler<T, ChildrenMode::Transition>
{
    static bool parentItem(QQmlListProperty<QObject> *prop, QObject *item)
    {
        if (QAbstractTransition *trans = qobject_cast<QAbstractTransition *>(item)) {
            parentObject<T>(prop)->addTransition(trans);
            return true;
        }
        return false;
    }

    static bool unparentItem(QQmlListProperty<QObject> *prop, QObject *oldItem)
    {
        if (QAbstractTransition *trans = qobject_cast<QAbstractTransition *>(oldItem)) {
            parentObject<T>(prop)->removeTransition(trans);
            return true;
        }
        return false;
    }
};

template<class T>
struct ParentHandler<T, ChildrenMode::StateOrTransition>
{
    static bool parentItem(QQmlListProperty<QObject> *prop, QObject *oldItem)
    {
        return ParentHandler<T, ChildrenMode::State>::parentItem(prop, oldItem)
                || ParentHandler<T, ChildrenMode::Transition>::parentItem(prop, oldItem);
    }

    static bool unparentItem(QQmlListProperty<QObject> *prop, QObject *oldItem)
    {
        return ParentHandler<T, ChildrenMode::State>::unparentItem(prop, oldItem)
                || ParentHandler<T, ChildrenMode::Transition>::unparentItem(prop, oldItem);
    }
};

template <class T, ChildrenMode Mode>
class ChildrenPrivate
{
public:
    static void append(QQmlListProperty<QObject> *prop, QObject *item)
    {
        Handler::parentItem(prop, item);
        static_cast<Self *>(prop->data)->children.append(item);
        emit parentObject<T>(prop)->childrenChanged();
    }

    static int count(QQmlListProperty<QObject> *prop)
    {
        return static_cast<Self *>(prop->data)->children.count();
    }

    static QObject *at(QQmlListProperty<QObject> *prop, int index)
    {
        return static_cast<Self *>(prop->data)->children.at(index);
    }

    static void clear(QQmlListProperty<QObject> *prop)
    {
        auto &children = static_cast<Self *>(prop->data)->children;
        for (QObject *oldItem : qAsConst(children))
            Handler::unparentItem(prop, oldItem);

        children.clear();
        emit parentObject<T>(prop)->childrenChanged();
    }

    static void replace(QQmlListProperty<QObject> *prop, int index, QObject *item)
    {
        auto &children = static_cast<Self *>(prop->data)->children;

        Handler::unparentItem(prop, children.at(index));
        Handler::parentItem(prop, item);

        children.replace(index, item);
        emit parentObject<T>(prop)->childrenChanged();
    }

    static void removeLast(QQmlListProperty<QObject> *prop)
    {
        Handler::unparentItem(prop, static_cast<Self *>(prop->data)->children.takeLast());
        emit parentObject<T>(prop)->childrenChanged();
    }

private:
    using Self = ChildrenPrivate<T, Mode>;
    using Handler = ParentHandler<T, Mode>;

    QList<QObject *> children;
};

#endif
