/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#ifndef QV4URLOBJECT_P_H
#define QV4URLOBJECT_P_H

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

#include "qv4object_p.h"
#include "qv4functionobject_p.h"

#include <QtCore/QString>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Heap {
// clang-format off
#define UrlObjectMembers(class, Member) \
    Member(class, Pointer, String *, hash) \
    Member(class, Pointer, String *, host) \
    Member(class, Pointer, String *, hostname) \
    Member(class, Pointer, String *, href) \
    Member(class, Pointer, String *, origin) \
    Member(class, Pointer, String *, password) \
    Member(class, Pointer, String *, pathname) \
    Member(class, Pointer, String *, port) \
    Member(class, Pointer, String *, protocol) \
    Member(class, Pointer, String *, search) \
    Member(class, Pointer, String *, username)
// clang-format on

DECLARE_HEAP_OBJECT(UrlObject, Object)
{
    DECLARE_MARKOBJECTS(UrlObject);
    void init() { Object::init(); }
};

struct UrlCtor : FunctionObject
{
    void init(QV4::ExecutionContext *scope);
};

// clang-format on
#define UrlSearchParamsObjectMembers(class, Member)                                                \
    Member(class, Pointer, ArrayObject *, params) \
    Member(class, Pointer, ArrayObject *, keys) \
    Member(class, Pointer, ArrayObject *, values)
// clang-format off

DECLARE_HEAP_OBJECT(UrlSearchParamsObject, Object)
{
    DECLARE_MARKOBJECTS(UrlSearchParamsObject);
    void init() { Object::init(); }
};

struct UrlSearchParamsCtor : FunctionObject
{
    void init(QV4::ExecutionContext *scope);
};
}

struct UrlObject : Object
{
    V4_OBJECT2(UrlObject, Object)
    Q_MANAGED_TYPE(UrlObject)
    V4_PROTOTYPE(urlPrototype)

    QString hash() const { return QLatin1String("#") + d()->hash->toQString(); }
    bool setHash(QString hash);

    QString host() const { return d()->host->toQString(); }
    bool setHost(QString host);

    QString hostname() const { return d()->hostname->toQString(); }
    bool setHostname(QString hostname);

    QString href() const { return d()->href->toQString(); }
    bool setHref(QString href);

    QString origin() const { return d()->origin->toQString(); }

    QString password() const { return d()->password->toQString(); }
    bool setPassword(QString password);

    QString pathname() const { return d()->pathname->toQString(); }
    bool setPathname(QString pathname);

    QString port() const { return d()->port->toQString(); }
    bool setPort(QString port);

    QString protocol() const { return d()->protocol->toQString(); }
    bool setProtocol(QString protocol);

    QString search() const { return QLatin1String("?") + d()->search->toQString(); }
    bool setSearch(QString search);

    QString username() const { return d()->username->toQString(); }
    bool setUsername(QString username);

    QUrl toQUrl() const;

private:
    void updateOrigin();
    void updateHost();
};

template<>
inline const UrlObject *Value::as() const
{
    return isManaged() && m()->internalClass->vtable->type == Managed::Type_UrlObject
            ? static_cast<const UrlObject *>(this)
            : nullptr;
}

struct UrlCtor : FunctionObject
{
    V4_OBJECT2(UrlCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv,
                                                  int argc, const Value *);
};

struct UrlPrototype : Object
{
    V4_PROTOTYPE(objectPrototype)

    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_getHash(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);
    static ReturnedValue method_setHash(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);

    static ReturnedValue method_getHost(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);
    static ReturnedValue method_setHost(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);

    static ReturnedValue method_getHostname(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);
    static ReturnedValue method_setHostname(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);

    static ReturnedValue method_getHref(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);
    static ReturnedValue method_setHref(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);

    static ReturnedValue method_getOrigin(const FunctionObject *, const Value *thisObject,
                                          const Value *argv, int argc);

    static ReturnedValue method_getPassword(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);
    static ReturnedValue method_setPassword(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);

    static ReturnedValue method_getPathname(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);
    static ReturnedValue method_setPathname(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);

    static ReturnedValue method_getPort(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);
    static ReturnedValue method_setPort(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);

    static ReturnedValue method_getProtocol(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);
    static ReturnedValue method_setProtocol(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);

    static ReturnedValue method_getSearch(const FunctionObject *, const Value *thisObject,
                                          const Value *argv, int argc);
    static ReturnedValue method_setSearch(const FunctionObject *, const Value *thisObject,
                                          const Value *argv, int argc);

    static ReturnedValue method_getUsername(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);
    static ReturnedValue method_setUsername(const FunctionObject *, const Value *thisObject,
                                            const Value *argv, int argc);

    static ReturnedValue method_getSearchParams(const FunctionObject *, const Value *thisObject,
                                                const Value *argv, int argc);
};

struct UrlSearchParamsObject : Object
{
    V4_OBJECT2(UrlSearchParamsObject, Object)
    Q_MANAGED_TYPE(UrlSearchParamsObject)
    V4_PROTOTYPE(urlSearchParamsPrototype)

    void initializeParams();
    void initializeParams(QString params);
    void initializeParams(ScopedArrayObject& params);
    void initializeParams(ScopedObject& params);

    QList<QStringList> params() const;
    void setParams(QList<QStringList> params);

    QString nameAt(int index) const;
    Heap::String * nameAtRaw(int index) const;
    QString valueAt(int index) const;
    Heap::String * valueAtRaw(int index) const;

    void append(Heap::String *name, Heap::String *value);

    int indexOf(QString name, int last = -1) const;
    int length() const;

    using Object::getOwnProperty;
protected:
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
private:
    QString stringAt(int index, int pairIndex) const;
    Heap::String * stringAtRaw(int index, int pairIndex) const;
};

template<>
inline const UrlSearchParamsObject *Value::as() const
{
    return isManaged() && m()->internalClass->vtable->type == Managed::Type_UrlSearchParamsObject
            ? static_cast<const UrlSearchParamsObject *>(this)
            : nullptr;
}

struct UrlSearchParamsCtor : FunctionObject
{
    V4_OBJECT2(UrlSearchParamsCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv,
                                                  int argc, const Value *);
};

struct UrlSearchParamsPrototype : Object
{
    V4_PROTOTYPE(objectPrototype)

    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject,
                                         const Value *argv, int argc);
    static ReturnedValue method_sort(const FunctionObject *, const Value *thisObject,
                                     const Value *argv, int argc);
    static ReturnedValue method_append(const FunctionObject *, const Value *thisObject,
                                       const Value *argv, int argc);
    static ReturnedValue method_delete(const FunctionObject *, const Value *thisObject,
                                       const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject,
                                    const Value *argv, int argc);
    static ReturnedValue method_set(const FunctionObject *, const Value *thisObject,
                                    const Value *argv, int argc);
    static ReturnedValue method_get(const FunctionObject *, const Value *thisObject,
                                    const Value *argv, int argc);
    static ReturnedValue method_getAll(const FunctionObject *, const Value *thisObject,
                                       const Value *argv, int argc);
    static ReturnedValue method_forEach(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);
    static ReturnedValue method_entries(const FunctionObject *, const Value *thisObject,
                                        const Value *argv, int argc);
    static ReturnedValue method_keys(const FunctionObject *, const Value *thisObject,
                                     const Value *argv, int argc);
    static ReturnedValue method_values(const FunctionObject *, const Value *thisObject,
                                       const Value *argv, int argc);

};

}

QT_END_NAMESPACE

#endif // QV4URLOBJECT_P_H
