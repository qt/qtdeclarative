// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4propertykey_p.h"

#include <QtCore/qstring.h>
#include <qv4string_p.h>
#include <qv4engine_p.h>
#include <qv4scopedvalue_p.h>

QV4::Heap::StringOrSymbol *QV4::PropertyKey::toStringOrSymbol(QV4::ExecutionEngine *e)
{
    if (isArrayIndex())
        return Value::fromUInt32(asArrayIndex()).toString(e);
    return asStringOrSymbol();
}

bool QV4::PropertyKey::isString() const {
    Heap::StringOrSymbol *s = asStringOrSymbol();
    return s && s->internalClass->vtable->isString;
}

bool QV4::PropertyKey::isSymbol() const {
    Heap::StringOrSymbol *s = asStringOrSymbol();
    return s && !s->internalClass->vtable->isString && s->internalClass->vtable->isStringOrSymbol;
}

bool QV4::PropertyKey::isCanonicalNumericIndexString() const
{
    if (isArrayIndex())
        return true;
    if (isSymbol())
        return false;
    Heap::String *s = static_cast<Heap::String *>(asStringOrSymbol());
    Scope scope(s->internalClass->engine);
    ScopedString str(scope, s);
    double d = str->toNumber();
    if (d == 0. && std::signbit(d))
        return true;
    ScopedString converted(scope, Value::fromDouble(d).toString(scope.engine));
    if (converted->equals(str))
        return true;
    return false;
}

QString QV4::PropertyKey::toQString() const
{
    if (isArrayIndex())
        return QString::number(asArrayIndex());
    Heap::StringOrSymbol *s = asStringOrSymbol();
    Q_ASSERT(s->internalClass->vtable->isStringOrSymbol);
    return s->toQString();
}

QV4::Heap::String *QV4::PropertyKey::asFunctionName(ExecutionEngine *engine, FunctionNamePrefix prefix) const
{
    QString n;
    if (prefix == Getter)
        n = QStringLiteral("get ");
    else if (prefix == Setter)
        n = QStringLiteral("set ");
    if (isArrayIndex())
        n += QString::number(asArrayIndex());
    else {
        Heap::StringOrSymbol *s = asStringOrSymbol();
        QString str = s->toQString();
        if (s->internalClass->vtable->isString)
            n += s->toQString();
        else if (str.size() > 1)
            n += QChar::fromLatin1('[') + QStringView{str}.mid(1) + QChar::fromLatin1(']');
    }
    return engine->newString(n);
}
