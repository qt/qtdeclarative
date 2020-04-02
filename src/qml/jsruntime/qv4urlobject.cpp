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

#include "qv4urlobject_p.h"

#include <QtCore/QUrl>

using namespace QV4;

DEFINE_OBJECT_VTABLE(UrlObject);
DEFINE_OBJECT_VTABLE(UrlCtor);

void Heap::UrlCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QLatin1String("URL"));
}

void UrlPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Q_UNUSED(ctor)

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
    QUrl url(href);

    if (!url.isValid() || url.isRelative())
        return false;

    d()->hash.set(engine(), engine()->newString(url.fragment()));
    d()->hostname.set(engine(), engine()->newString(url.host()));
    d()->href.set(engine(), engine()->newString(url.toString()));
    d()->password.set(engine(), engine()->newString(url.password()));
    d()->pathname.set(engine(), engine()->newString(url.path()));
    d()->port.set(engine(),
                  engine()->newString(url.port() == -1 ? QLatin1String("")
                                                       : QString::number(url.port())));
    d()->protocol.set(engine(), engine()->newString(url.scheme()));
    d()->search.set(engine(), engine()->newString(url.query()));
    d()->username.set(engine(), engine()->newString(url.userName()));

    updateOrigin();
    updateHost();

    return true;
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

bool UrlObject::setProtocol(QString protocol)
{
    QUrl url = toQUrl();
    url.setScheme(protocol);

    if (!url.isValid())
        return false;

    d()->protocol.set(engine(), engine()->newString(url.scheme()));
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

ReturnedValue UrlPrototype::method_getHash(const FunctionObject *b, const Value *thisObject,
                                           const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    r->setHash(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getHost(const FunctionObject *b, const Value *thisObject,
                                           const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    return Encode(v4->newString(r->origin()));
}

ReturnedValue UrlPrototype::method_getPassword(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    r->setPassword(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getPathname(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    r->setPathname(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getPort(const FunctionObject *b, const Value *thisObject,
                                           const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    if (!r->setPort(port))
        return v4->throwTypeError(QLatin1String("Invalid port: %1").arg(port));

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getProtocol(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    r->setProtocol(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getSearch(const FunctionObject *b, const Value *thisObject,
                                             const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    r->setSearch(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlPrototype::method_getUsername(const FunctionObject *b, const Value *thisObject,
                                               const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

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

    Scoped<UrlObject> r(scope, thisObject->as<UrlObject>());

    r->setUsername(stringValue->toQString());

    return Encode::undefined();
}

ReturnedValue UrlCtor::virtualCallAsConstructor(const FunctionObject *that, const Value *argv,
                                                int argc, const Value *newTarget)
{
    ExecutionEngine *v4 = that->engine();

    if (argc < 1 || argc > 2)
        return v4->throwError(QLatin1String("Invalid amount of arguments"));

    Scope scope(v4);

    ScopedValue arg1(scope, argv[0]);
    String *arg1StringValue = arg1->stringValue();

    if (arg1StringValue == nullptr)
        return v4->throwTypeError(QLatin1String("Invalid parameter provided"));

    QString arg1String = arg1StringValue->toQString();
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
