// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4arrayiterator_p.h"
#include "qv4urlobject_p.h"

#include <QtCore/QUrl>

#include <qv4jscall_p.h>
#include <qv4objectiterator_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(UrlObject);
DEFINE_OBJECT_VTABLE(UrlCtor);

DEFINE_OBJECT_VTABLE(UrlSearchParamsObject);
DEFINE_OBJECT_VTABLE(UrlSearchParamsCtor);


void Heap::UrlCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QLatin1String("URL"));
}

void UrlPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Q_UNUSED(ctor);

    Scope scope(engine);
    ScopedObject o(scope);

    defineDefaultProperty(QLatin1String("toString"), method_getHref);
    defineDefaultProperty(QLatin1String("toJSON"), method_getHref);

    defineAccessorProperty(QLatin1String("hash"), method_getHash, method_setHash);
    defineAccessorProperty(QLatin1String("host"), method_getHost, method_setHost);
    defineAccessorProperty(QLatin1String("hostname"), method_getHostname, method_setHostname);
    defineAccessorProperty(QLatin1String("href"), method_getHref, method_setHref);
    defineAccessorProperty(QLatin1String("origin"), method_getOrigin, nullptr);
    defineAccessorProperty(QLatin1String("password"), method_getPassword, method_setPassword);
    defineAccessorProperty(QLatin1String("pathname"), method_getPathname, method_setPathname);
    defineAccessorProperty(QLatin1String("port"), method_getPort, method_setPort);
    defineAccessorProperty(QLatin1String("protocol"), method_getProtocol, method_setProtocol);
    defineAccessorProperty(QLatin1String("search"), method_getSearch, method_setSearch);
    defineAccessorProperty(QLatin1String("searchParams"), method_getSearchParams, nullptr);
    defineAccessorProperty(QLatin1String("username"), method_getUsername, method_setUsername);
}

bool UrlObject::setHash(QString hash)
{
    if (hash.startsWith(QLatin1Char('#')))
        hash = hash.mid(1);

    QUrl url = toQUrl();
    url.setFragment(hash);

    if (!url.isValid())
        return false;

    d()->hash.set(engine(), engine()->newString(url.fragment()));
    d()->href.set(engine(), engine()->newString(url.toString()));

    return true;
}

bool UrlObject::setHostname(QString host)
{
    QUrl url = toQUrl();
    url.setHost(host);

    if (!url.isValid())
        return false;

    d()->hostname.set(engine(), engine()->newString(url.host()));
    d()->href.set(engine(), engine()->newString(url.toString()));

    updateOrigin();
    updateHost();

    return true;
}

bool UrlObject::setHost(QString hostname)
{
    int port = -1;

    if (hostname.contains(QLatin1Char(':'))) {
        const QStringList list = hostname.split(QLatin1Char(':'));
        hostname = list[0];
        port = list[1].toInt();
    }

    QUrl url = toQUrl();
    url.setHost(hostname);
    url.setPort(port);

    if (!url.isValid())
        return false;

    if (url.port() != -1)
        d()->port.set(engine(), engine()->newString(QString::number(url.port())));

    d()->hostname.set(engine(), engine()->newString(url.host()));
    d()->href.set(engine(), engine()->newString(url.toString()));

    updateOrigin();
    updateHost();

    return true;
}

bool UrlObject::setHref(QString href)
{
    const QUrl url(href);
    if (!url.isValid() || url.isRelative())
        return false;

    setUrl(url);
    return true;
}

void UrlObject::setUrl(const QUrl &url)
{
    d()->hash.set(engine(), engine()->newString(url.fragment()));
    d()->hostname.set(engine(), engine()->newString(url.host()));
    d()->href.set(engine(), engine()->newString(url.toString(QUrl::ComponentFormattingOptions(QUrl::ComponentFormattingOption::FullyEncoded))));
    d()->password.set(engine(), engine()->newString(url.password()));
    d()->pathname.set(engine(), engine()->newString(url.path()));
    d()->port.set(engine(),
                  engine()->newString(url.port() == -1 ? QLatin1String("")
                                                       : QString::number(url.port())));
    d()->protocol.set(engine(), engine()->newString(url.scheme() + QLatin1Char(':')));
    d()->search.set(engine(), engine()->newString(url.query(QUrl::ComponentFormattingOptions(QUrl::ComponentFormattingOption::FullyEncoded))));
    d()->username.set(engine(), engine()->newString(url.userName()));

    updateOrigin();
    updateHost();
}

bool UrlObject::setPassword(QString password)
{
    QUrl url = toQUrl();
    url.setPassword(password);

    if (!url.isValid())
        return false;

    d()->password.set(engine(), engine()->newString(url.password()));
    d()->href.set(engine(), engine()->newString(url.toString()));

    return true;
}

bool UrlObject::setPathname(QString pathname)
{
    QUrl url = toQUrl();
    url.setPath(pathname);

    if (!url.isValid())
        return false;

    d()->pathname.set(engine(), engine()->newString(url.path()));
    d()->href.set(engine(), engine()->newString(url.toString()));

    return true;
}

bool UrlObject::setPort(QString port)
{
    QUrl url = toQUrl();
    url.setPort(port.isEmpty() ? -1 : port.toInt());

    if (!url.isValid())
        return false;

    d()->port.set(engine(),
                  engine()->newString(url.port() == -1 ? QLatin1String("")
                                                       : QString::number(url.port())));
    d()->href.set(engine(), engine()->newString(url.toString()));

    updateOrigin();
    updateHost();

    return true;
}

bool UrlObject::setProtocol(QString protocolOrScheme)
{
    QUrl url = toQUrl();
    // If there is one or several ':' in the protocolOrScheme,
    // everything from the first colon is removed.

    qsizetype firstColonPos = protocolOrScheme.indexOf(QLatin1Char(':'));

    if (firstColonPos != -1)
        protocolOrScheme.truncate(firstColonPos);

    url.setScheme(protocolOrScheme);

    if (!url.isValid())
        return false;

    d()->protocol.set(engine(), engine()->newString(url.scheme() + QLatin1Char(':')));
    d()->href.set(engine(), engine()->newString(url.toString()));

    updateOrigin();
    updateHost();

    return true;
}

bool UrlObject::setSearch(QString search)
{
    QUrl url = toQUrl();

    if (search.startsWith(QLatin1Char('?')))
        search = search.mid(1);

    url.setQuery(search);

    if (!url.isValid())
        return false;

    d()->search.set(engine(), engine()->newString(url.query()));
    d()->href.set(engine(), engine()->newString(url.toString()));

    return true;
}

bool UrlObject::setUsername(QString username)
{
    QUrl url = toQUrl();
    url.setUserName(username);

    if (!url.isValid())
        return false;

    d()->username.set(engine(), engine()->newString(url.userName()));
    d()->href.set(engine(), engine()->newString(url.toString()));

    return true;
}

QString UrlObject::search() const
{
    auto url = QUrl(href());
    if (auto url = QUrl(href()); !url.hasQuery() || url.query().isEmpty())
        return QLatin1String("");

    constexpr auto options = QUrl::ComponentFormattingOption::EncodeSpaces
            | QUrl::ComponentFormattingOption::EncodeUnicode
            | QUrl::ComponentFormattingOption::EncodeReserved;
    return u'?' + url.query(options);
}

QUrl UrlObject::toQUrl() const
{
    return QUrl(href());
}

void UrlObject::updateOrigin()
{
    QUrl url = toQUrl();

    QString proto = url.scheme();

    // A blob's origin is the origin of the URL that it points to
    if (proto == QLatin1String("blob")) {
        url = QUrl(url.path());
        proto = url.scheme();
    }

    QString origin;
    if (proto == QLatin1String("http") || proto == QLatin1String("https")
        || proto == QLatin1String("ftp")) {
        origin = QLatin1String("%1://%2").arg(url.scheme(), url.host());

        if (url.port() != -1)
            origin.append(QLatin1String(":") + QString::number(url.port()));
    }

    d()->origin.set(engine(), engine()->newString(origin));
}

void UrlObject::updateHost()
{
    QUrl url = toQUrl();

    QString host = url.host();

    if (url.port() != -1)
        host.append(QLatin1String(":") + QString::number(url.port()));

    d()->host.set(engine(), engine()->newString(host));
}

static bool checkUrlObjectType(ExecutionEngine *v4, const Scoped<UrlObject> &r)
{
    if (r)
        return true;

    v4->throwTypeError(QStringLiteral("Value of \"this\" must be of type URL"));
    return false;
}

ReturnedValue UrlPrototype::method_getHash(const FunctionObject *b, const Value *thisObject,
                                           const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->hash()));
}

ReturnedValue UrlPrototype::method_setHash(const FunctionObject *b, const Value *thisObject,
                                           const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    r->setHash(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getHost(const FunctionObject *b, const Value *thisObject,
                                           const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->host()));
}

ReturnedValue UrlPrototype::method_setHost(const FunctionObject *b, const Value *thisObject,
                                           const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    QString host = stringValue->toQString();
    if (!r->setHost(host))
        return v4->throwTypeError(QLatin1String("Invalid host: %1").arg(host));

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getHostname(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->hostname()));
}

ReturnedValue UrlPrototype::method_setHostname(const FunctionObject *b, const Value *thisObject,
                                               const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    QString hostname = stringValue->toQString();
    if (!r->setHostname(hostname))
        return v4->throwTypeError(QLatin1String("Invalid hostname: %1").arg(hostname));

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getHref(const FunctionObject *b, const Value *thisObject,
                                           const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->href()));
}

ReturnedValue UrlPrototype::method_setHref(const FunctionObject *b, const Value *thisObject,
                                           const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    QString href = stringValue->toQString();
    if (!r->setHref(href))
        return v4->throwTypeError(QLatin1String("Invalid URL: %1").arg(href));

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getOrigin(const FunctionObject *b, const Value *thisObject,
                                             const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->origin()));
}

ReturnedValue UrlPrototype::method_getPassword(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->password()));
}

ReturnedValue UrlPrototype::method_setPassword(const FunctionObject *b, const Value *thisObject,
                                               const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    r->setPassword(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getPathname(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->pathname()));
}

ReturnedValue UrlPrototype::method_setPathname(const FunctionObject *b, const Value *thisObject,
                                               const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    r->setPathname(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getPort(const FunctionObject *b, const Value *thisObject,
                                           const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->port()));
}

ReturnedValue UrlPrototype::method_setPort(const FunctionObject *b, const Value *thisObject,
                                           const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    QString port;

    if (stringValue != nullptr)
        port = stringValue->toQString();
    else if (arg->isInt32())
        port = QString::number(arg->toInt32());
    else
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    if (!r->setPort(port))
        return v4->throwTypeError(QLatin1String("Invalid port: %1").arg(port));

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getProtocol(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->protocol()));
}

ReturnedValue UrlPrototype::method_setProtocol(const FunctionObject *b, const Value *thisObject,
                                               const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    r->setProtocol(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getSearch(const FunctionObject *b, const Value *thisObject,
                                             const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->search()));
}

ReturnedValue UrlPrototype::method_setSearch(const FunctionObject *b, const Value *thisObject,
                                             const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    r->setSearch(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getUsername(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    return Encode(v4->newString(r->username()));
}

ReturnedValue UrlPrototype::method_setUsername(const FunctionObject *b, const Value *thisObject,
                                               const Value *argv, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    String *stringValue = arg->stringValue();

    if (stringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    r->setUsername(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getSearchParams(const FunctionObject *b, const Value *thisObject,
                                                   const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject);
    if (!checkUrlObjectType(v4, r))
        return Encode::undefined();

    Scoped<UrlSearchParamsObject> usp(scope, v4->newUrlSearchParamsObject());

    usp->setUrlObject(thisObject->as<UrlObject>());
    usp->initializeParams(r->search());

    return usp->asReturnedValue();
}

ReturnedValue UrlCtor::virtualCallAsConstructor(const FunctionObject *that, const Value *argv,
                                                int argc, const Value *newTarget)
{
    ExecutionEngine *v4 = that->engine();

    if (argc < 1 || argc > 2)
        return v4->throwError(QLatin1String("Invalid amount of arguments"));

    Scope scope(v4);

    ScopedValue arg1(scope, argv[0]);

    QString arg1String = arg1->toQString();
    QString urlString;

    if (argc == 2) {
        ScopedValue arg2(scope, argv[1]);
        String *arg2StringValue = arg2->stringValue();

        if (arg2StringValue == nullptr)
            return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

        QUrl url = QUrl(arg2StringValue->toQString());
        QUrl relativeUrl = QUrl(arg1String);

        QString baseUrlPath = url.path();
        QString relativePath = relativeUrl.path();

        // If the base URL contains a path the last section of it is discarded
        int lastSlash = baseUrlPath.lastIndexOf(QLatin1Char('/'));
        if (lastSlash != -1)
            baseUrlPath.truncate(lastSlash);

        if (!relativePath.startsWith(QLatin1Char('/')))
            relativePath = relativePath.prepend(QLatin1Char('/'));

        url.setPath(baseUrlPath + relativePath);
        url.setFragment(relativeUrl.fragment());
        url.setQuery(relativeUrl.query());

        urlString = url.toString();
    } else {
        urlString = arg1String;
    }

    ReturnedValue o = Encode(v4->newUrlObject());

    if (!newTarget)
        return o;

    ScopedObject obj(scope, o);
    obj->setProtoFromNewTarget(newTarget);

    UrlObject *urlObject = obj->as<UrlObject>();

    if (!urlObject->setHref(urlString))
        return v4->throwTypeError(QLatin1String("Invalid URL: %1").arg(urlString));

    return obj->asReturnedValue();
}


void Heap::UrlSearchParamsCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QLatin1String("URLSearchParams"));
}

void UrlSearchParamsPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Q_UNUSED(ctor);

    Scope scope(engine);
    ScopedObject o(scope);

    defineDefaultProperty(QLatin1String("toString"), method_toString);
    defineDefaultProperty(QLatin1String("sort"), method_sort);
    defineDefaultProperty(QLatin1String("append"), method_append);
    defineDefaultProperty(QLatin1String("delete"), method_delete);
    defineDefaultProperty(QLatin1String("has"), method_has);
    defineDefaultProperty(QLatin1String("set"), method_set);
    defineDefaultProperty(QLatin1String("get"), method_get);
    defineDefaultProperty(QLatin1String("getAll"), method_getAll);
    defineDefaultProperty(QLatin1String("forEach"), method_forEach);
    defineDefaultProperty(QLatin1String("entries"), method_entries);
    defineDefaultProperty(QLatin1String("keys"), method_keys);
    defineDefaultProperty(QLatin1String("values"), method_values);
}

ReturnedValue UrlSearchParamsCtor::virtualCallAsConstructor(const FunctionObject *that, const Value *argv,
                                                int argc, const Value *newTarget)
{
    ExecutionEngine *v4 = that->engine();

    if (argc > 1)
        return v4->throwError(QLatin1String("Invalid amount of arguments"));

    Scope scope(v4);

    ScopedValue arg(scope, argv[0]);
    ArrayObject *argArrayObject = arg->as<ArrayObject>();
    Object *argObject = arg->as<Object>();

    ReturnedValue o = Encode(v4->newUrlSearchParamsObject());

    if (!newTarget)
        return o;

    ScopedObject obj(scope, o);
    obj->setProtoFromNewTarget(newTarget);

    UrlSearchParamsObject *urlSearchParamsObject = obj->as<UrlSearchParamsObject>();

    if (argArrayObject != nullptr) {
        ScopedArrayObject argArray(scope, argArrayObject);

        uint len = argArray->getLength();

        for (uint i = 0; i < len; i++) {
            QV4::Value pair = argArray->get(i);
            auto *pairArrayObject = pair.as<ArrayObject>();

            if (pairArrayObject == nullptr) {
                return v4->throwTypeError(
                    QLatin1String("element %1 is not a pair").arg(QString::number(i)));
            }


            ScopedArrayObject pairArray(scope, pairArrayObject);


            uint pairLen = pairArray->getLength();


            if (pairLen != 2) {
                return v4->throwTypeError(QLatin1String("pair %1 has %2 elements instead of 2")
                                          .arg(QString::number(i))
                                          .arg(QString::number(pairLen)));
            }
        }

        urlSearchParamsObject->initializeParams(argArray);
    } else if (argObject != nullptr) {
        ScopedObject scopedObject(scope, argObject);
        urlSearchParamsObject->initializeParams(scopedObject);
    } else {
        QString value = argc > 0 ? arg->toQString() : QLatin1String("");
        urlSearchParamsObject->initializeParams(value);
    }

    return obj->asReturnedValue();
}

void UrlSearchParamsObject::initializeParams()
{
    auto *arrayObject = engine()->newArrayObject(0);
    auto *keys = engine()->newArrayObject(0);
    auto *values = engine()->newArrayObject(0);

    d()->params.set(engine(), arrayObject);
    d()->keys.set(engine(), keys);
    d()->values.set(engine(), values);
}

void UrlSearchParamsObject::initializeParams(QString value)
{
    Q_ASSERT(d()->params == nullptr);

    initializeParams();

    if (value.startsWith(QLatin1Char('?')))
        value = value.mid(1);

    const QStringList params = value.split(QLatin1Char('&'));

    for (const QString& param : params) {
        if (param.isEmpty())
            continue;

        QString key, value;

        int equalsIndex = param.indexOf(QLatin1Char('='));
        if (equalsIndex != -1) {
            key = param.left(equalsIndex);
            value = param.mid(equalsIndex+1);
        } else {
            key = param;
        }

        append(engine()->newString(key), engine()->newString(value));
    }
}

void UrlSearchParamsObject::initializeParams(ScopedArrayObject& params)
{
    Q_ASSERT(d()->params == nullptr);

    Scope scope(engine());

    uint len = params->getLength();
    auto *keys = engine()->newArrayObject(len);
    auto *values = engine()->newArrayObject(len);

    ScopedArrayObject scopedKeys(scope, keys);
    ScopedArrayObject scopedValues(scope, values);

    for (uint i = 0; i < len; i++)
    {
        QV4::Value pair = params->get(i);
        auto *pairArrayObject = pair.as<ArrayObject>();

        QV4::Value key = pairArrayObject->get(uint(0));
        QV4::Value value = pairArrayObject->get(uint(1));

        scopedKeys->put(i, key);
        scopedValues->put(i, value);
    }


    d()->params.set(engine(), params->d());
    d()->keys.set(engine(), keys);
    d()->values.set(engine(), values);
}

void UrlSearchParamsObject::initializeParams(ScopedObject& params)
{
    Q_ASSERT(d()->params == nullptr);

    initializeParams();

    Scope scope(engine());
    ObjectIterator it(scope, params, ObjectIterator::EnumerableOnly);

    ScopedValue name(scope);
    ScopedValue val(scope);

    while (true) {
        name = it.nextPropertyNameAsString(val);
        if (name->isNull())
            break;

        Heap::String *nameStr = name->as<String>()->d();
        Heap::String *valStr = val->toString(engine());

        append(nameStr, valStr);
    }
}

void UrlSearchParamsObject::setParams(QList<QStringList> params)
{
    auto *arrayObject = engine()->newArrayObject(0);
    auto *keys = engine()->newArrayObject(0);
    auto *values = engine()->newArrayObject(0);

    Scope scope(engine());

    ScopedArrayObject scopedArray(scope, arrayObject);

    ScopedArrayObject scopedKeys(scope, keys);
    ScopedArrayObject scopedValues(scope, values);

    uint len = 0;

    for (const QStringList& param : params) {

        auto *valuePair = engine()->newArrayObject(2);

        ScopedArrayObject valuePairObject(scope, valuePair);

        ScopedValue key(scope, Value::fromHeapObject(engine()->newString(param[0])));
        ScopedValue value(scope, Value::fromHeapObject(engine()->newString(param[1])));
        valuePairObject->put(uint(0), key);
        valuePairObject->put(uint(1), value);

        scopedKeys->put(len, key);
        scopedValues->put(len, value);

        scopedArray->put(len, valuePairObject);
        len++;
    }

    d()->params.set(engine(), arrayObject);
    d()->keys.set(engine(), keys);
    d()->values.set(engine(), values);
}

void UrlSearchParamsObject::setUrlObject(const UrlObject *url)
{
    d()->url.set(engine(), url->d());
}

void UrlSearchParamsObject::append(Heap::String *name, Heap::String *value)
{
    Scope scope(engine());

    ScopedArrayObject scopedArray(scope, d()->params);
    ScopedArrayObject scopedKeys(scope, d()->keys);
    ScopedArrayObject scopedValues(scope, d()->values);

    auto *valuePair = engine()->newArrayObject(2);

    ScopedArrayObject valuePairObject(scope, valuePair);

    ScopedValue keyScoped(scope, Value::fromHeapObject(name));
    ScopedValue valueScoped(scope, Value::fromHeapObject(value));
    valuePairObject->put(uint(0), keyScoped);
    valuePairObject->put(uint(1), valueScoped);

    uint len = scopedArray->getLength();

    scopedKeys->put(len, keyScoped);
    scopedValues->put(len, valueScoped);

    scopedArray->put(len, valuePairObject);
}

QList<QStringList> UrlSearchParamsObject::params() const
{
    auto *arrayObject = d()->params.get();
    Scope scope(engine());
    ScopedArrayObject scopedArray(scope, arrayObject);

    QList<QStringList> result;

    uint len = scopedArray->getLength();

    for (uint i = 0; i < len; i++) {
        QV4::Value pair = scopedArray->get(i);
        auto *pairArrayObject = pair.as<ArrayObject>();

        QV4::Value key = pairArrayObject->get(uint(0));
        QV4::Value value = pairArrayObject->get(uint(1));

        result << QStringList { key.toQString(), value.toQString() };
    }

    return result;
}

Heap::UrlObject *UrlSearchParamsObject::urlObject() const
{
    return d()->url.get();
}

QString UrlSearchParamsObject::searchString() const
{
    QString search = QLatin1String("");
    auto params = this->params();
    auto len = params.size();
    for (int i = 0; i < len; ++i) {
        const QStringList &param = params[i];
        search += param[0] + QLatin1Char('=') + param[1];
        if (i != len - 1)
            search += QLatin1Char('&');
    }
    return search;
}

int UrlSearchParamsObject::length() const
{
    auto *arrayObject = d()->params.get();
    Scope scope(engine());
    ScopedArrayObject scopedArray(scope, arrayObject);

    return scopedArray->getLength();
}

int UrlSearchParamsObject::indexOf(QString name, int last) const
{
    auto *arrayObject = d()->params.get();
    Scope scope(engine());
    ScopedArrayObject scopedArray(scope, arrayObject);

    int len = scopedArray->getLength();

    for (int i = last + 1; i < len; i++) {
        QV4::Value pair = scopedArray->get(i);
        auto *pairArrayObject = pair.as<ArrayObject>();

        QV4::Value key = pairArrayObject->get(uint(0));

        if (key.toQString() == name)
            return i;
    }

    return -1;
}

QString UrlSearchParamsObject::stringAt(int index, int pairIndex) const
{
    auto *arrayObject = d()->params.get();
    Scope scope(engine());
    ScopedArrayObject scopedArray(scope, arrayObject);

    if (index >= scopedArray->getLength())
        return {};

    QV4::Value pair = scopedArray->get(index);
    auto *pairArrayObject = pair.as<ArrayObject>();

    QV4::Value value = pairArrayObject->get(pairIndex);

    return value.toQString();
}

QV4::Heap::String * UrlSearchParamsObject::stringAtRaw(int index, int pairIndex) const
{
    auto *arrayObject = d()->params.get();
    Scope scope(engine());
    ScopedArrayObject scopedArray(scope, arrayObject);

    if (index >= scopedArray->getLength())
        return nullptr;

    QV4::Value pair = scopedArray->get(index);
    auto *pairArrayObject = pair.as<ArrayObject>();

    QV4::Value value = pairArrayObject->get(pairIndex);

    return value.as<String>()->d();
}

QString UrlSearchParamsObject::nameAt(int index) const
{
    return stringAt(index, 0);
}

QV4::Heap::String * UrlSearchParamsObject::nameAtRaw(int index) const
{
    return stringAtRaw(index, 0);
}


QString UrlSearchParamsObject::valueAt(int index) const
{
    return stringAt(index, 1);
}

QV4::Heap::String * UrlSearchParamsObject::valueAtRaw(int index) const
{
    return stringAtRaw(index, 1);
}


struct UrlSearchParamsObjectOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
{
    ~UrlSearchParamsObjectOwnPropertyKeyIterator() override = default;
    PropertyKey next(const QV4::Object *o, Property *pd = nullptr,
                     PropertyAttributes *attrs = nullptr) override;
};

PropertyKey UrlSearchParamsObjectOwnPropertyKeyIterator::next(const QV4::Object *o, Property *pd,
                                                              PropertyAttributes *attrs)
{
    const UrlSearchParamsObject *usp = static_cast<const UrlSearchParamsObject *>(o);

    Scope scope(usp);

    uint len = usp->length();
    if (arrayIndex < len) {
        uint index = arrayIndex;
        ++arrayIndex;
        if (attrs)
            *attrs = Attr_NotConfigurable | Attr_NotWritable;
        if (pd)
            pd->value = usp->engine()->newString(usp->nameAt(index));
        return PropertyKey::fromArrayIndex(index);
    }

    return ObjectOwnPropertyKeyIterator::next(o, pd, attrs);
}

OwnPropertyKeyIterator *UrlSearchParamsObject::virtualOwnPropertyKeys(const Object *m,
                                                                      Value *target)
{
    *target = *m;
    return new UrlSearchParamsObjectOwnPropertyKeyIterator;
}

PropertyAttributes UrlSearchParamsObject::virtualGetOwnProperty(const Managed *m, PropertyKey id,
                                                                Property *p)
{
    PropertyAttributes attributes = Object::virtualGetOwnProperty(m, id, p);
    if (attributes != Attr_Invalid)
        return attributes;

    if (id.isArrayIndex()) {
        const int index = id.asArrayIndex();
        const auto usp = static_cast<const UrlSearchParamsObject *>(m);
        if (index < usp->length()) {
            if (p)
                p->value = usp->engine()->newString(usp->nameAt(index));
            return Attr_NotConfigurable | Attr_NotWritable;
        }
    }

    return Object::virtualGetOwnProperty(m, id, p);
}

static bool checkSearchParamsType(ExecutionEngine *v4, const Scoped<UrlSearchParamsObject> &o)
{
    if (o)
        return true;

    v4->throwTypeError(QStringLiteral("Value of \"this\" must be of type URLSearchParams"));
    return false;
}

ReturnedValue UrlSearchParamsPrototype::method_toString(
        const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    auto params = o->params();

    QString value;

    for (const QStringList &pair : params)
        value += QLatin1String("%1=%2&").arg(QString::fromUtf8(QUrl::toPercentEncoding(pair[0])),
                                             QString::fromUtf8(QUrl::toPercentEncoding(pair[1])));

    value.chop(1);

    return Encode(v4->newString(value));
}

ReturnedValue UrlSearchParamsPrototype::method_sort(const FunctionObject *b, const Value *thisObject,
                                                    const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    QList<QStringList> params = o->params();
    std::stable_sort(params.begin(), params.end(), [](QStringList a, QStringList b) { return a[0] < b[0]; });

    o->setParams(params);

    return Encode::undefined();
}

ReturnedValue UrlSearchParamsPrototype::method_append(const FunctionObject *b, const Value *thisObject,
                                                      const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 2)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    ScopedValue argName(scope, argv[0]);
    ScopedValue argValue(scope, argv[1]);

    String *argNameString = argName->stringValue();

    if (argNameString == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid argument provided"));

    ScopedString name(scope, argName->as<String>());
    ScopedString value(scope, argValue->toString(v4));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    o->append(name->d(), value->d());

    return Encode::undefined();
}

ReturnedValue UrlSearchParamsPrototype::method_delete(const FunctionObject *b, const Value *thisObject,
                                                      const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 1)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    ScopedValue argName(scope, argv[0]);

    String *argNameString = argName->stringValue();

    if (argNameString == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid argument provided"));

    QString name = argNameString->toQString();

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    QList<QStringList> params = o->params();

    auto to_remove = std::remove_if(params.begin(), params.end(), [&name](QStringList pair) {
                                                                      return pair[0] == name;
                                                                  });

    params.erase(to_remove, params.end());

    o->setParams(params);

    return Encode::undefined();
}

ReturnedValue UrlSearchParamsPrototype::method_has(const FunctionObject *b, const Value *thisObject,
                                                   const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 1)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    ScopedValue argName(scope, argv[0]);

    String *argNameString = argName->stringValue();

    if (argNameString == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid argument provided"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    QString name = argNameString->toQString();

    return Encode(o->indexOf(name) != -1);
}

ReturnedValue UrlSearchParamsPrototype::method_set(const FunctionObject *b, const Value *thisObject,
                                                   const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 2)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    ScopedValue argName(scope, argv[0]);
    ScopedValue argValue(scope, argv[1]);

    String *argNameString = argName->stringValue();

    if (argNameString == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid argument provided"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    QString name = argNameString->toQString();
    QString value = argValue->toQString();

    auto params = o->params();

    bool matched = false;

    for (auto it = params.begin(); it != params.end();) {
        QStringList &param = *it;
        if (param[0] == name) {
            if (!matched) {
                param[1] = value;
                matched = true;
            } else {
                it = params.erase(it);
                continue;
            }
        }
        it++;
    }

    if (!matched)
        params << QStringList { name, value };

    o->setParams(params);

    Scoped<UrlObject> scopedUrlObject(scope, o->d()->url.get());
    if (scopedUrlObject)
        scopedUrlObject->setSearch(o->searchString());

    return Encode::undefined();
}

ReturnedValue UrlSearchParamsPrototype::method_get(const FunctionObject *b, const Value *thisObject,
                                                   const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 1)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    ScopedValue argName(scope, argv[0]);

    String *argNameString = argName->stringValue();

    if (argNameString == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid argument provided"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    QString name = argNameString->toQString();

    int index = o->indexOf(name);

    if (index == -1)
        return Encode::null();

    return Encode(o->valueAtRaw(index));
}

ReturnedValue UrlSearchParamsPrototype::method_getAll(const FunctionObject *b,
                                                      const Value *thisObject, const Value *argv,
                                                      int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 1)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    ScopedValue argName(scope, argv[0]);

    String *argNameString = argName->stringValue();

    if (argNameString == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid argument provided"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    QString name = argNameString->toQString();

    auto *arrayObject = v4->newArrayObject(0);
    ScopedArrayObject result(scope, arrayObject);

    int i = 0;
    for (int index = o->indexOf(name); index != -1; index = o->indexOf(name, index)) {
        ScopedValue value(scope, Value::fromHeapObject(o->valueAtRaw(index)));
        result->put(i++, value);
    }

    return Encode(arrayObject);
}

ReturnedValue UrlSearchParamsPrototype::method_forEach(const FunctionObject *b,
                                                       const Value *thisObject, const Value *argv,
                                                       int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 1)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    ScopedValue argFunc(scope, argv[0]);

    FunctionObject *func = argFunc->as<FunctionObject>();

    if (func == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid argument: must be a function"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    for (int i = 0; i < o->length(); i++) {
        Scoped<String> name(scope, o->nameAtRaw(i));
        Scoped<String> value(scope, o->valueAtRaw(i));

        QV4::JSCallArguments calldata(scope, 2);

        calldata.args[0] = value;
        calldata.args[1] = name;

        func->call(calldata);
    }

    return Encode::undefined();
}

ReturnedValue UrlSearchParamsPrototype::method_entries(const FunctionObject *b,
                                                       const Value *thisObject, const Value *,
                                                       int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 0)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    ScopedObject params(scope, o->d()->params.get());

    Scoped<ArrayIteratorObject> paramsIterator(scope, v4->newArrayIteratorObject(params));
    paramsIterator->d()->iterationKind = IteratorKind::KeyValueIteratorKind;
    return paramsIterator->asReturnedValue();
}

ReturnedValue UrlSearchParamsPrototype::method_keys(const FunctionObject *b,
                                                    const Value *thisObject, const Value *,
                                                    int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 0)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    ScopedObject keys(scope, o->d()->keys.get());

    Scoped<ArrayIteratorObject> keysIterator(scope, v4->newArrayIteratorObject(keys));
    keysIterator->d()->iterationKind = IteratorKind::KeyValueIteratorKind;
    return keysIterator->asReturnedValue();
}

ReturnedValue UrlSearchParamsPrototype::method_values(const FunctionObject *b,
                                                      const Value *thisObject, const Value *,
                                                      int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    if (argc != 0)
        return v4->throwError(QLatin1String("Bad amount of arguments"));

    Scoped<UrlSearchParamsObject> o(scope, thisObject);
    if (!checkSearchParamsType(v4, o))
        return Encode::undefined();

    ScopedObject values(scope, o->d()->values.get());

    Scoped<ArrayIteratorObject> valuesIterator(scope, v4->newArrayIteratorObject(values));
    valuesIterator->d()->iterationKind = IteratorKind::KeyValueIteratorKind;
    return valuesIterator->asReturnedValue();
}
