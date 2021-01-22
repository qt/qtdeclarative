/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include <private/qv4iterator_p.h>
#include <private/qv4stringiterator_p.h>
#include <private/qv4symbol_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(StringIteratorObject);

void StringIteratorPrototype::init(ExecutionEngine *e)
{
    defineDefaultProperty(QStringLiteral("next"), method_next, 0);

    Scope scope(e);
    ScopedString val(scope, e->newString(QLatin1String("String Iterator")));
    defineReadonlyConfigurableProperty(e->symbol_toStringTag(), val);
}

ReturnedValue StringIteratorPrototype::method_next(const FunctionObject *b, const Value *that, const Value *, int)
{
    Scope scope(b);
    const StringIteratorObject *thisObject = that->as<StringIteratorObject>();
    if (!thisObject)
        return scope.engine->throwTypeError(QLatin1String("Not an String Iterator instance"));

    ScopedString s(scope, thisObject->d()->iteratedString);
    if (!s) {
        QV4::Value undefined = Value::undefinedValue();
        return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
    }

    quint32 index = thisObject->d()->nextIndex;

    QString str = s->toQString();
    quint32 len = str.length();

    if (index >= len) {
        thisObject->d()->iteratedString.set(scope.engine, nullptr);
        QV4::Value undefined = Value::undefinedValue();
        return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
    }

    QChar ch = str.at(index);
    int num = 1;
    if (ch.unicode() >= 0xd800 && ch.unicode() <= 0xdbff && index + 1 != len) {
        ch = str.at(index + 1);
        if (ch.unicode() >= 0xdc00 && ch.unicode() <= 0xdfff)
            num = 2;
    }

    thisObject->d()->nextIndex += num;

    ScopedString resultString(scope, scope.engine->newString(s->toQString().mid(index, num)));
    return IteratorPrototype::createIterResultObject(scope.engine, resultString, false);
}

