/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qdeclarativeparser_p.h"

#include "qdeclarativepropertyvaluesource.h"
#include "private/qdeclarativevme_p.h"
#include "qdeclarative.h"
#include "private/qdeclarativecomponent_p.h"
#include "qdeclarativecomponent.h"
#include "private/qmetaobjectbuilder_p.h"
#include "private/qdeclarativevmemetaobject_p.h"
#include "private/qdeclarativecompiler_p.h"
#include "parser/qdeclarativejsast_p.h"
#include "parser/qdeclarativejsengine_p.h"

#include <QStack>
#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QStringBuilder>
#include <QtDebug>

QT_BEGIN_NAMESPACE

using namespace QDeclarativeJS;
using namespace QDeclarativeParser;

QDeclarativeParser::Object::Object()
: type(-1), idIndex(-1), metatype(0), synthCache(0), defaultProperty(0), parserStatusCast(-1),
  componentCompileState(0), nextAliasingObject(0), nextIdObject(0)
{
}

QDeclarativeParser::Object::~Object() 
{ 
    if (synthCache) synthCache->release();
}

void Object::setBindingBit(int b)
{
    while (bindingBitmask.size() < 4 * (1 + b / 32))
        bindingBitmask.append(char(0));

    quint32 *bits = (quint32 *)bindingBitmask.data();
    bits[b / 32] |= (1 << (b % 32));
}

const QMetaObject *Object::metaObject() const
{
    if (!metadata.isEmpty() && metatype)
        return &extObject;
    else
        return metatype;
}

QDeclarativeParser::Property *Object::getDefaultProperty()
{
    if (!defaultProperty) {
        defaultProperty = pool()->New<Property>();
        defaultProperty->parent = this;
    }
    return defaultProperty;
}

void QDeclarativeParser::Object::addValueProperty(Property *p)
{
    valueProperties.append(p);
}

void QDeclarativeParser::Object::addSignalProperty(Property *p)
{
    signalProperties.append(p);
}

void QDeclarativeParser::Object::addAttachedProperty(Property *p)
{
    attachedProperties.append(p);
}

void QDeclarativeParser::Object::addGroupedProperty(Property *p)
{
    groupedProperties.append(p);
}

void QDeclarativeParser::Object::addValueTypeProperty(Property *p)
{
    valueTypeProperties.append(p);
}

void QDeclarativeParser::Object::addScriptStringProperty(Property *p)
{
    scriptStringProperties.append(p);
}

// This lookup is optimized for missing, and having to create a new property.
Property *QDeclarativeParser::Object::getProperty(const QHashedStringRef &name, bool create)
{
    if (create) {
        quint32 h = name.hash();
        if (propertiesHashField.testAndSet(h)) {
            for (Property *p = properties.first(); p; p = properties.next(p)) {
                if (p->name() == name)
                    return p;
            }
        }

        Property *property = pool()->New<Property>();
        property->parent = this;
        property->_name = name;
        property->isDefault = false;
        properties.prepend(property);
        return property;
    } else {
        for (Property *p = properties.first(); p; p = properties.next(p)) {
            if (p->name() == name)
                return p;
        }
    }
}

Property *QDeclarativeParser::Object::getProperty(const QStringRef &name, bool create)
{
    return getProperty(QHashedStringRef(name), create);
}

Property *QDeclarativeParser::Object::getProperty(const QString &name, bool create)
{
    for (Property *p = properties.first(); p; p = properties.next(p)) {
        if (p->name() == name)
            return p;
    }

    if (create) {
        Property *property = pool()->New<Property>();
        property->parent = this;
        property->_name = QStringRef(pool()->NewString(name));
        propertiesHashField.testAndSet(property->_name.hash());
        property->isDefault = false;
        properties.prepend(property);
        return property;
    } else {
        return 0;
    }
}

QDeclarativeParser::Object::DynamicProperty::DynamicProperty()
: isDefaultProperty(false), type(Variant), defaultValue(0), nextProperty(0), 
  resolvedCustomTypeName(0)
{
}

QDeclarativeParser::Object::DynamicSignal::DynamicSignal()
: nextSignal(0)
{
}

// Returns length in utf8 bytes
int QDeclarativeParser::Object::DynamicSignal::parameterTypesLength() const
{
    int rv = 0;
    for (int ii = 0; ii < parameterTypes.count(); ++ii)
        rv += parameterTypes.at(ii).length();
    return rv;
}

// Returns length in utf8 bytes
int QDeclarativeParser::Object::DynamicSignal::parameterNamesLength() const
{
    int rv = 0;
    for (int ii = 0; ii < parameterNames.count(); ++ii)
        rv += parameterNames.at(ii).utf8length();
    return rv;
}

QDeclarativeParser::Object::DynamicSlot::DynamicSlot()
: nextSlot(0)
{
}

int QDeclarativeParser::Object::DynamicSlot::parameterNamesLength() const
{
    int rv = 0;
    for (int ii = 0; ii < parameterNames.count(); ++ii)
        rv += parameterNames.at(ii).length();
    return rv;
}

QDeclarativeParser::Property::Property()
: parent(0), type(0), index(-1), value(0), isDefault(true), isDeferred(false), 
  isValueTypeSubProperty(false), isAlias(false), scriptStringScope(-1), nextMainProperty(0),
  nextProperty(0)
{
}

QDeclarativeParser::Object *QDeclarativeParser::Property::getValue(const LocationSpan &l)
{
    if (!value) { value = pool()->New<Object>(); value->location = l; }
    return value;
}

void QDeclarativeParser::Property::addValue(Value *v)
{
    values.append(v);
}

void QDeclarativeParser::Property::addOnValue(Value *v)
{
    onValues.append(v);
}

bool QDeclarativeParser::Property::isEmpty() const
{
    return !value && values.isEmpty() && onValues.isEmpty();
}

QDeclarativeParser::Value::Value()
: type(Unknown), object(0), bindingReference(0), signalExpressionContextStack(0), nextValue(0)
{
}

QDeclarativeParser::Variant::Variant()
: t(Invalid)
{
}

QDeclarativeParser::Variant::Variant(const Variant &o)
: t(o.t), d(o.d), asWritten(o.asWritten)
{
}

QDeclarativeParser::Variant::Variant(bool v)
: t(Boolean), b(v)
{
}

QDeclarativeParser::Variant::Variant(double v, const QStringRef &asWritten)
: t(Number), d(v), asWritten(asWritten)
{
}

QDeclarativeParser::Variant::Variant(QDeclarativeJS::AST::StringLiteral *v)
: t(String), l(v)
{
}

QDeclarativeParser::Variant::Variant(const QStringRef &asWritten, QDeclarativeJS::AST::Node *n)
: t(Script), n(n), asWritten(asWritten)
{
}

QDeclarativeParser::Variant &QDeclarativeParser::Variant::operator=(const Variant &o)
{
    t = o.t;
    d = o.d;
    asWritten = o.asWritten;
    return *this;
}

QDeclarativeParser::Variant::Type QDeclarativeParser::Variant::type() const
{
    return t;
}

bool QDeclarativeParser::Variant::asBoolean() const
{
    return b;
}

QString QDeclarativeParser::Variant::asString() const
{
    if (t == String) {
        // XXX aakenned
        return l->value.toString();
    } else {
        return asWritten.toString();
    }
}

double QDeclarativeParser::Variant::asNumber() const
{
    return d;
}

//reverse of Lexer::singleEscape()
QString escapedString(const QString &string)
{
    QString tmp = QLatin1String("\"");
    for (int i = 0; i < string.length(); ++i) {
        const QChar &c = string.at(i);
        switch(c.unicode()) {
        case 0x08:
            tmp += QLatin1String("\\b");
            break;
        case 0x09:
            tmp += QLatin1String("\\t");
            break;
        case 0x0A:
            tmp += QLatin1String("\\n");
            break;
        case 0x0B:
            tmp += QLatin1String("\\v");
            break;
        case 0x0C:
            tmp += QLatin1String("\\f");
            break;
        case 0x0D:
            tmp += QLatin1String("\\r");
            break;
        case 0x22:
            tmp += QLatin1String("\\\"");
            break;
        case 0x27:
            tmp += QLatin1String("\\\'");
            break;
        case 0x5C:
            tmp += QLatin1String("\\\\");
            break;
        default:
            tmp += c;
            break;
        }
    }
    tmp += QLatin1Char('\"');
    return tmp;
}

QString QDeclarativeParser::Variant::asScript() const
{
    switch(type()) { 
    default:
    case Invalid:
        return QString();
    case Boolean:
        return b?QLatin1String("true"):QLatin1String("false");
    case Number:
        if (asWritten.isEmpty())
            return QString::number(d);
        else 
            return asWritten.toString();
    case String:
        return escapedString(asString());
    case Script:
        if (AST::IdentifierExpression *i = AST::cast<AST::IdentifierExpression *>(n)) {
            // XXX aakenned
            return i->name.toString();
        } else
            return asWritten.toString();
    }
}

QDeclarativeJS::AST::Node *QDeclarativeParser::Variant::asAST() const
{
    if (type() == Script)
        return n;
    else
        return 0;
}

bool QDeclarativeParser::Variant::isStringList() const
{
    if (isString())
        return true;

    if (type() != Script || !n)
        return false;

    AST::ArrayLiteral *array = AST::cast<AST::ArrayLiteral *>(n);
    if (!array)
        return false;

    AST::ElementList *elements = array->elements;

    while (elements) {

        if (!AST::cast<AST::StringLiteral *>(elements->expression))
            return false;

        elements = elements->next;
    }

    return true;
}

QStringList QDeclarativeParser::Variant::asStringList() const
{
    QStringList rv;
    if (isString()) {
        rv << asString();
        return rv;
    }

    AST::ArrayLiteral *array = AST::cast<AST::ArrayLiteral *>(n);
    if (!array)
        return rv;

    AST::ElementList *elements = array->elements;
    while (elements) {

        AST::StringLiteral *string = AST::cast<AST::StringLiteral *>(elements->expression);
        if (!string)
            return QStringList();
        rv.append(string->value.toString());

        elements = elements->next;
    }

    return  rv;
}

QT_END_NAMESPACE
