/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <private/qqmldata_p.h>

QT_BEGIN_NAMESPACE

class QQmlGuardImpl
{
public:
    inline QQmlGuardImpl();
    inline QQmlGuardImpl(QObject *);
    inline QQmlGuardImpl(const QQmlGuardImpl &);
    inline ~QQmlGuardImpl();

    QObject *o = nullptr;
    QQmlGuardImpl  *next = nullptr;
    QQmlGuardImpl **prev = nullptr;

    inline void addGuard();
    inline void remGuard();
};

class QObject;
template<class T>
class QQmlGuard : private QQmlGuardImpl
{
    friend class QQmlData;
public:
    inline QQmlGuard();
    inline QQmlGuard(T *);
    inline QQmlGuard(const QQmlGuard<T> &);
    inline virtual ~QQmlGuard();

    inline QQmlGuard<T> &operator=(const QQmlGuard<T> &o);
    inline QQmlGuard<T> &operator=(T *);

    inline T *object() const;
    inline void setObject(T *g);

    inline bool isNull() const
        { return !o; }

    inline T* operator->() const
        { return static_cast<T*>(const_cast<QObject*>(o)); }
    inline T& operator*() const
        { return *static_cast<T*>(const_cast<QObject*>(o)); }
    inline operator T*() const
        { return static_cast<T*>(const_cast<QObject*>(o)); }
    inline T* data() const
        { return static_cast<T*>(const_cast<QObject*>(o)); }

protected:
    virtual void objectDestroyed(T *) {}
};

template <typename T>
class QQmlStrongJSQObjectReference : public QQmlGuard<T>
{
public:
    void setObject(T *o, QObject *parent) {
        T *old = this->object();
        if (o == old)
            return;

        if (m_jsOwnership && old && old->parent() == parent)
            QQml_setParent_noEvent(old, nullptr);

        this->QQmlGuard<T>::operator=(o);

        if (o && !o->parent() && !QQmlData::keepAliveDuringGarbageCollection(o)) {
            m_jsOwnership = true;
            QQml_setParent_noEvent(o, parent);
        } else {
            m_jsOwnership = false;
        }
    }

private:
    using QQmlGuard<T>::setObject;
    using QQmlGuard<T>::operator=;
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

QQmlGuardImpl::QQmlGuardImpl(const QQmlGuardImpl &g)
: o(g.o)
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
QQmlGuard<T>::~QQmlGuard()
{
}

template<class T>
QQmlGuard<T> &QQmlGuard<T>::operator=(const QQmlGuard<T> &g)
{
    setObject(g.object());
    return *this;
}

template<class T>
QQmlGuard<T> &QQmlGuard<T>::operator=(T *g)
{
    setObject(g);
    return *this;
}

template<class T>
T *QQmlGuard<T>::object() const
{
    return static_cast<T *>(o);
}

template<class T>
void QQmlGuard<T>::setObject(T *g)
{
    if (g != o) {
        if (prev) remGuard();
        o = g;
        if (o) addGuard();
    }
}

QT_END_NAMESPACE

#endif // QQMLGUARD_P_H
