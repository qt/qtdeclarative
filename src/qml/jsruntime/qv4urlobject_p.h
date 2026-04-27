// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
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
#define UrlObjectMembers(class, Member)
DECLARE_HEAP_OBJECT(UrlObject, Object)
{
    DECLARE_MARKOBJECTS(UrlObject)
    void init()
    {
        new (&m_url) QUrl;
        Object::init();
    }
    void init(const QUrl &url)
    {
        new (&m_url) QUrl(url);
        Object::init();
    }

    void destroy()
    {
        std::destroy_at<QUrl>(reinterpret_cast<QUrl *>(&m_url));
    }

    QUrl url() const { return *reinterpret_cast<const QUrl *>(&m_url); }
    void setUrl(QUrl &&url) { *reinterpret_cast<QUrl *>(&m_url) = std::move(url); }

    static constexpr auto alignment = alignof(QUrl);
    alignas(alignment) std::byte m_url[sizeof(QUrl)];
};

struct UrlCtor : FunctionObject
{
    void init(ExecutionEngine *engine);
};

// clang-format off
#define UrlSearchParamsObjectMembers(class, Member)                                                \
    Member(class, Pointer, ArrayObject *, params) \
    Member(class, Pointer, ArrayObject *, keys) \
    Member(class, Pointer, ArrayObject *, values) \
    Member(class, Pointer, UrlObject *, url)
// clang-format on

DECLARE_HEAP_OBJECT(UrlSearchParamsObject, Object)
{
    DECLARE_MARKOBJECTS(UrlSearchParamsObject)
    void init() { Object::init(); }
};

struct UrlSearchParamsCtor : FunctionObject
{
    void init(ExecutionEngine *engine);
};
}

struct UrlObject : Object
{
    V4_OBJECT2(UrlObject, Object)
    V4_NEEDS_DESTROY
    Q_MANAGED_TYPE(UrlObject)
    V4_PROTOTYPE(urlPrototype)

    QString hash() const
    {
        const QUrl url = constD()->url();
        return QLatin1Char('#') + url.fragment();
    }
    bool setHash(const QString &hash);

    QString host() const
    {
        const QUrl url = constD()->url();
        const int port = url.port();
        return port == -1 ? url.host() : (url.host() + QLatin1Char(':') + QString::number(port));
    }
    bool setHost(const QString &host);

    QString hostname() const
    {
        const QUrl url = constD()->url();
        return url.isValid() ? url.host() : QString();
    }
    bool setHostname(const QString &hostname);

    QString href() const
    {
        return constD()->url().toString(QUrl::ComponentFormattingOption::FullyEncoded);
    }
    bool setHref(const QString &href);

    QString origin() const
    {
        const auto resolve = [](const QUrl &url) {
            const QString proto = url.scheme();
            if (proto != QLatin1String("http") && proto != QLatin1String("https")
                    && proto != QLatin1String("ftp")) {
                return QString();
            }

            if (const int port = url.port(); port != -1)
                return QLatin1String("%1://%2:%3").arg(proto, url.host(), QString::number(port));

            return QLatin1String("%1://%2").arg(proto, url.host());
        };

        const QUrl url = constD()->url();

        // A blob's origin is the origin of the URL that it points to
        return url.scheme() == QLatin1String("blob") ? resolve(QUrl(url.path())) : resolve(url);
    }

    QString password() const { return constD()->url().password(); }
    bool setPassword(const QString &password);

    QString pathname() const { return constD()->url().path(); }
    bool setPathname(const QString &pathname);

    QString port() const
    {
        const int port = constD()->url().port();
        return port == -1 ? QString() : QString::number(port);
    }
    bool setPort(const QString &port);

    QString protocol() const { return constD()->url().scheme() + QLatin1Char(':'); }
    bool setProtocol(const QString &protocol);

    Q_QML_AUTOTEST_EXPORT QString search() const;
    bool setSearch(const QString &search);

    QString username() const { return constD()->url().userName(); }
    bool setUsername(const QString &username);

    QUrl toQUrl() const
    {
        return d()->url();
    }
    void setUrl(const QUrl &url);

private:
    const Heap::UrlObject *constD() const { return d(); }

    template<typename F>
    bool updateUrl(F &&f) {
        QUrl url = d()->url();
        f(&url);
        if (!url.isValid())
            return false;

        d()->setUrl(std::move(url));
        return true;
    }
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
    void setParams(const QList<QStringList> &params);
    Heap::UrlObject *urlObject() const;
    void setUrlObject(const UrlObject *url);

    QString searchString() const;

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
