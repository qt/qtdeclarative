/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QQMLGUARD_P_H
#define QQMLGUARD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmldata_p.h>
#include <private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlGuardImpl
{
public:
    using ObjectDestroyedFn = void(*)(QQmlGuardImpl *);

    inline QQmlGuardImpl();
    inline QQmlGuardImpl(QObject *);
    inline QQmlGuardImpl(const QQmlGuardImpl &);
    inline ~QQmlGuardImpl();

    QObject *o = nullptr;
    QQmlGuardImpl  *next = nullptr;
    QQmlGuardImpl **prev = nullptr;
    ObjectDestroyedFn objectDestroyed = nullptr;

    inline void addGuard();
    inline void remGuard();

    inline void setObject(QObject *g);
    bool isNull() const noexcept { return !o; }
};

class QObject;
template<class T>
class QQmlGuard : protected QQmlGuardImpl
{
    friend class QQmlData;
public:
    inline QQmlGuard();
    inline QQmlGuard(ObjectDestroyedFn objectDestroyed, T *);
    inline QQmlGuard(T *);
    inline QQmlGuard(const QQmlGuard<T> &);

    inline QQmlGuard<T> &operator=(const QQmlGuard<T> &o);
    inline QQmlGuard<T> &operator=(T *);

    T *object() const noexcept { return static_cast<T *>(o); }
    void setObject(T *g) { QQmlGuardImpl::setObject(g); }

    using QQmlGuardImpl::isNull;

    T *operator->() const noexcept { return object(); }
    T &operator*() const { return *object(); }
    operator T *() const noexcept { return object(); }
    T *data() const noexcept { return object(); }
};

template <typename T>
class QQmlStrongJSQObjectReference : protected QQmlGuardImpl
{
public:
    T *object() const noexcept { return static_cast<T *>(o); }

    using QQmlGuardImpl::isNull;

    T *operator->() const noexcept { return object(); }
    T &operator*() const { return *object(); }
    operator T *() const noexcept { return object(); }
    T *data() const noexcept { return object(); }
    void setObject(T *obj, QObject *parent) {
        T *old = object();
        if (obj == old)
            return;

        if (m_jsOwnership && old && old->parent() == parent)
            QQml_setParent_noEvent(old, nullptr);

        QQmlGuardImpl::setObject(obj);

        if (obj && !obj->parent() && !QQmlData::keepAliveDuringGarbageCollection(obj)) {
            m_jsOwnership = true;
            QQml_setParent_noEvent(obj, parent);
        } else {
            m_jsOwnership = false;
        }
    }

private:
    bool m_jsOwnership = false;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlGuard<QObject>)

QT_BEGIN_NAMESPACE

QQmlGuardImpl::QQmlGuardImpl()
{
}

QQmlGuardImpl::QQmlGuardImpl(QObject *g)
: o(g)
{
    if (o) addGuard();
}

/*
    \internal
    Copying a QQmlGuardImpl leaves the old one in the intrinsic linked list of guards.
    The fresh copy does not contain the list pointer of the existing guard; instead
    only the object and objectDestroyed pointers are copied, and if there is an object
    we add the new guard to the object's list of guards.
 */
QQmlGuardImpl::QQmlGuardImpl(const QQmlGuardImpl &g)
: o(g.o), objectDestroyed(g.objectDestroyed)
{
    if (o) addGuard();
}

QQmlGuardImpl::~QQmlGuardImpl()
{
    if (prev) remGuard();
    o = nullptr;
}

void QQmlGuardImpl::addGuard()
{
    Q_ASSERT(!prev);

    if (QObjectPrivate::get(o)->wasDeleted)
        return;

    QQmlData *data = QQmlData::get(o, true);
    next = data->guards;
    if (next) next->prev = &next;
    data->guards = this;
    prev = &data->guards;
}

void QQmlGuardImpl::remGuard()
{
    Q_ASSERT(prev);

    if (next) next->prev = prev;
    *prev = next;
    next = nullptr;
    prev = nullptr;
}

template<class T>
QQmlGuard<T>::QQmlGuard()
{
}

template<class T>
QQmlGuard<T>::QQmlGuard(ObjectDestroyedFn objDestroyed, T *obj)
    : QQmlGuardImpl(obj)
{
    objectDestroyed = objDestroyed;
}

template<class T>
QQmlGuard<T>::QQmlGuard(T *g)
: QQmlGuardImpl(g)
{
}

template<class T>
QQmlGuard<T>::QQmlGuard(const QQmlGuard<T> &g)
: QQmlGuardImpl(g)
{
}

template<class T>
QQmlGuard<T> &QQmlGuard<T>::operator=(const QQmlGuard<T> &g)
{
    objectDestroyed = g.objectDestroyed;
    setObject(g.object());
    return *this;
}

template<class T>
QQmlGuard<T> &QQmlGuard<T>::operator=(T *g)
{
    /* this does not touch objectDestroyed, as operator= is only a convenience
     * for setObject. All logic involving objectDestroyed is (sub-)class specific
     * and remains unaffected.
     */
    setObject(g);
    return *this;
}

void QQmlGuardImpl::setObject(QObject *g)
{
    if (g != o) {
        if (prev) remGuard();
        o = g;
        if (o) addGuard();
    }
}

QT_END_NAMESPACE

#endif // QQMLGUARD_P_H
