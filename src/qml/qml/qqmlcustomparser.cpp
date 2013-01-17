/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlcustomparser_p.h"
#include "qqmlcustomparser_p_p.h"

#include "qqmlcompiler_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

using namespace QQmlScript;

/*!
    \class QQmlCustomParser
    \brief The QQmlCustomParser class allows you to add new arbitrary types to QML.
    \internal

    By subclassing QQmlCustomParser, you can add a parser for
    building a particular type.

    The subclass must implement compile() and setCustomData(), and register
    itself in the meta type system by calling the macro:

    \code
    QML_REGISTER_CUSTOM_TYPE(Module, MajorVersion, MinorVersion, Name, TypeClass, ParserClass)
    \endcode
*/

/*
    \fn QByteArray QQmlCustomParser::compile(const QList<QQmlCustomParserProperty> & properties)

    The custom parser processes \a properties, and returns
    a QByteArray containing data meaningful only to the
    custom parser; the type engine will pass this same data to
    setCustomData() when making an instance of the data.

    Errors must be reported via the error() functions.

    The QByteArray may be cached between executions of the system, so
    it must contain correctly-serialized data (not, for example,
    pointers to stack objects).
*/

/*
    \fn void QQmlCustomParser::setCustomData(QObject *object, const QByteArray &data)

    This function sets \a object to have the properties defined
    by \a data, which is a block of data previously returned by a call
    to compile().

    Errors should be reported using qmlInfo(object).

    The \a object will be an instance of the TypeClass specified by QML_REGISTER_CUSTOM_TYPE.
*/

QQmlCustomParserNode 
QQmlCustomParserNodePrivate::fromObject(QQmlScript::Object *root)
{
    QQmlCustomParserNode rootNode;
    if (root->typeReference)
        rootNode.d->name = root->typeReference->name;
    rootNode.d->location = root->location.start;

    for (Property *p = root->properties.first(); p; p = root->properties.next(p)) {
        rootNode.d->properties << fromProperty(p);
    }

    if (root->defaultProperty)
        rootNode.d->properties << fromProperty(root->defaultProperty);

    return rootNode;
}

QQmlCustomParserProperty 
QQmlCustomParserNodePrivate::fromProperty(QQmlScript::Property *p)
{
    QQmlCustomParserProperty prop;
    prop.d->name = p->name().toString();
    prop.d->isList = p->values.isMany();
    prop.d->location = p->location.start;

    if (p->value) {
        QQmlCustomParserNode node = fromObject(p->value);
        QList<QQmlCustomParserProperty> props = node.properties();
        for (int ii = 0; ii < props.count(); ++ii)
            prop.d->values << QVariant::fromValue(props.at(ii));
    } else {
        for (QQmlScript::Value *v = p->values.first(); v; v = p->values.next(v)) {
            v->type = QQmlScript::Value::Literal;

            if(v->object) {
                QQmlCustomParserNode node = fromObject(v->object);
                prop.d->values << QVariant::fromValue(node);
            } else {
                prop.d->values << QVariant::fromValue(v->value);
            }

        }
    }

    return prop;
}

QQmlCustomParserNode::QQmlCustomParserNode()
: d(new QQmlCustomParserNodePrivate)
{
}

QQmlCustomParserNode::QQmlCustomParserNode(const QQmlCustomParserNode &other)
: d(new QQmlCustomParserNodePrivate)
{
    *this = other;
}

QQmlCustomParserNode &QQmlCustomParserNode::operator=(const QQmlCustomParserNode &other)
{
    d->name = other.d->name;
    d->properties = other.d->properties;
    d->location = other.d->location;
    return *this;
}

QQmlCustomParserNode::~QQmlCustomParserNode()
{
    delete d; d = 0;
}

QString QQmlCustomParserNode::name() const
{
    return d->name;
}

QList<QQmlCustomParserProperty> QQmlCustomParserNode::properties() const
{
    return d->properties;
}

QQmlScript::Location QQmlCustomParserNode::location() const
{
    return d->location;
}

QQmlCustomParserProperty::QQmlCustomParserProperty()
: d(new QQmlCustomParserPropertyPrivate)
{
}

QQmlCustomParserProperty::QQmlCustomParserProperty(const QQmlCustomParserProperty &other)
: d(new QQmlCustomParserPropertyPrivate)
{
    *this = other;
}

QQmlCustomParserProperty &QQmlCustomParserProperty::operator=(const QQmlCustomParserProperty &other)
{
    d->name = other.d->name;
    d->isList = other.d->isList;
    d->values = other.d->values;
    d->location = other.d->location;
    return *this;
}

QQmlCustomParserProperty::~QQmlCustomParserProperty()
{
    delete d; d = 0;
}

QString QQmlCustomParserProperty::name() const
{
    return d->name;
}

bool QQmlCustomParserProperty::isList() const
{
    return d->isList;
}

QQmlScript::Location QQmlCustomParserProperty::location() const
{
    return d->location;
}

QList<QVariant> QQmlCustomParserProperty::assignedValues() const
{
    return d->values;
}

void QQmlCustomParser::clearErrors()
{
    exceptions.clear();
}

/*!
    Reports an error with the given \a description.

    This can only be used during the compile() step. For errors during setCustomData(), use qmlInfo().

    An error is generated referring to the position of the element in the source file.
*/
void QQmlCustomParser::error(const QString& description)
{
    Q_ASSERT(object);
    QQmlError error;
    QString exceptionDescription;
    error.setLine(object->location.start.line);
    error.setColumn(object->location.start.column);
    error.setDescription(description);
    exceptions << error;
}

/*!
    Reports an error in parsing \a prop, with the given \a description.

    An error is generated referring to the position of \a node in the source file.
*/
void QQmlCustomParser::error(const QQmlCustomParserProperty& prop, const QString& description)
{
    QQmlError error;
    QString exceptionDescription;
    error.setLine(prop.location().line);
    error.setColumn(prop.location().column);
    error.setDescription(description);
    exceptions << error;
}

/*!
    Reports an error in parsing \a node, with the given \a description.

    An error is generated referring to the position of \a node in the source file.
*/
void QQmlCustomParser::error(const QQmlCustomParserNode& node, const QString& description)
{
    QQmlError error;
    QString exceptionDescription;
    error.setLine(node.location().line);
    error.setColumn(node.location().column);
    error.setDescription(description);
    exceptions << error;
}

/*!
    If \a script is a simple enumeration expression (eg. Text.AlignLeft),
    returns the integer equivalent (eg. 1), and sets \a ok to true.

    Otherwise sets \a ok to false.

    A valid \a ok must be provided, or the function will assert.
*/
int QQmlCustomParser::evaluateEnum(const QByteArray& script, bool *ok) const
{
    Q_ASSERT_X(ok, "QQmlCustomParser::evaluateEnum", "ok must not be a null pointer");
    *ok = false;
    int dot = script.indexOf('.');
    if (dot == -1)
        return -1;

    return compiler->evaluateEnum(QString::fromUtf8(script.left(dot)), script.mid(dot+1), ok);
}

/*!
    Resolves \a name to a type, or 0 if it is not a type. This can be used
    to type-check object nodes.
*/
const QMetaObject *QQmlCustomParser::resolveType(const QString& name) const
{
    return compiler->resolveType(name);
}

/*!
    Rewrites \a value and returns an identifier that can be
    used to construct the binding later. \a name
    is used as the name of the rewritten function.
*/
QQmlBinding::Identifier QQmlCustomParser::rewriteBinding(const QQmlScript::Variant &value, const QString& name)
{
    return compiler->rewriteBinding(value, name);
}

/*!
    Returns a rewritten \a handler. \a name
    is used as the name of the rewritten function.
*/
QString QQmlCustomParser::rewriteSignalHandler(const QQmlScript::Variant &value, const QString &name)
{
    return compiler->rewriteSignalHandler(value , name);
}

QT_END_NAMESPACE
