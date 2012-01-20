/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "QtQuick1/private/qdeclarativepropertychanges_p.h"

#include "QtQuick1/private/qdeclarativeopenmetaobject_p.h"
#include "QtDeclarative/private/qdeclarativerewrite_p.h"
#include "QtDeclarative/private/qdeclarativeengine_p.h"
#include "QtDeclarative/private/qdeclarativecompiler_p.h"

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtDeclarative/private/qdeclarativecustomparser_p.h>
#include <QtDeclarative/private/qdeclarativescript_p.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/private/qdeclarativebinding_p.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>
#include <QtDeclarative/private/qdeclarativeproperty_p.h>
#include <QtDeclarative/private/qdeclarativecontext_p.h>
#include <QtQuick1/private/qdeclarativestate_p_p.h>

#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE



/*!
    \qmlclass PropertyChanges QDeclarative1PropertyChanges
    \inqmlmodule QtQuick 1
    \ingroup qml-state-elements
    \since QtQuick 1.0
    \brief The PropertyChanges element describes new property bindings or values for a state.

    PropertyChanges is used to define the property values or bindings in a 
    \l State. This enables an item's property values to be changed when it
    \l {QML States}{changes between states}.

    To create a PropertyChanges object, specify the \l target item whose 
    properties are to be modified, and define the new property values or
    bindings. For example:
    
    \snippet doc/src/snippets/qtquick1/propertychanges.qml import 
    \codeline
    \snippet doc/src/snippets/qtquick1/propertychanges.qml 0

    When the mouse is pressed, the \l Rectangle changes to the \e resized
    state. In this state, the PropertyChanges object sets the rectangle's 
    color to blue and the \c height value to that of \c container.height.

    Note this automatically binds \c rect.height to \c container.height 
    in the \e resized state. If a property binding should not be
    established, and the height should just be set to the value of
    \c container.height at the time of the state change, set the \l explicit
    property to \c true.
   
    A PropertyChanges object can also override the default signal handler
    for an object to implement a signal handler specific to the new state:

    \qml
    PropertyChanges {
        target: myMouseArea
        onClicked: doSomethingDifferent()
    }
    \endqml

    \note PropertyChanges can be used to change anchor margins, but not other anchor
    values; use AnchorChanges for this instead. Similarly, to change an \l Item's
    \l {Item::}{parent} value, use ParentChanges instead.


    \section2 Resetting property values

    The \c undefined value can be used to reset the property value for a state.
    In the following example, when \c theText changes to the \e widerText
    state, its \c width property is reset, giving the text its natural width
    and displaying the whole string on a single line.

    \snippet doc/src/snippets/qtquick1/propertychanges.qml reset


    \section2 Immediate property changes in transitions

    When \l{QML Animation and Transitions}{Transitions} are used to animate
    state changes, they animate properties from their values in the current
    state to those defined in the new state (as defined by PropertyChanges
    objects). However, it is sometimes desirable to set a property value
    \e immediately during a \l Transition, without animation; in these cases,
    the PropertyAction element can be used to force an immediate property
    change.

    See the PropertyAction documentation for more details.

    \sa {declarative/animation/states}{states example}, {qmlstate}{States}, QtDeclarative
*/

/*!
    \qmlproperty Object QtQuick1::PropertyChanges::target
    This property holds the object which contains the properties to be changed.
*/

class QDeclarative1ReplaceSignalHandler : public QDeclarative1ActionEvent
{
public:
    QDeclarative1ReplaceSignalHandler() : expression(0), reverseExpression(0),
                                rewindExpression(0), ownedExpression(0) {}
    ~QDeclarative1ReplaceSignalHandler() {
        delete ownedExpression;
    }

    virtual QString typeName() const { return QLatin1String("ReplaceSignalHandler"); }

    QDeclarativeProperty property;
    QDeclarativeExpression *expression;
    QDeclarativeExpression *reverseExpression;
    QDeclarativeExpression *rewindExpression;
    QDeclarativeGuard<QDeclarativeExpression> ownedExpression;

    virtual void execute(Reason) {
        ownedExpression = QDeclarativePropertyPrivate::setSignalExpression(property, expression);
        if (ownedExpression == expression)
            ownedExpression = 0;
    }

    virtual bool isReversable() { return true; }
    virtual void reverse(Reason) {
        ownedExpression = QDeclarativePropertyPrivate::setSignalExpression(property, reverseExpression);
        if (ownedExpression == reverseExpression)
            ownedExpression = 0;
    }

    virtual void saveOriginals() {
        saveCurrentValues();
        reverseExpression = rewindExpression;
    }

    virtual bool needsCopy() { return true; }
    virtual void copyOriginals(QDeclarative1ActionEvent *other)
    {
        QDeclarative1ReplaceSignalHandler *rsh = static_cast<QDeclarative1ReplaceSignalHandler*>(other);
        saveCurrentValues();
        if (rsh == this)
            return;
        reverseExpression = rsh->reverseExpression;
        if (rsh->ownedExpression == reverseExpression) {
            ownedExpression = rsh->ownedExpression;
            rsh->ownedExpression = 0;
        }
    }

    virtual void rewind() {
        ownedExpression = QDeclarativePropertyPrivate::setSignalExpression(property, rewindExpression);
        if (ownedExpression == rewindExpression)
            ownedExpression = 0;
    }
    virtual void saveCurrentValues() { 
        rewindExpression = QDeclarativePropertyPrivate::signalExpression(property);
    }

    virtual bool override(QDeclarative1ActionEvent*other) {
        if (other == this)
            return true;
        if (other->typeName() != typeName())
            return false;
        if (static_cast<QDeclarative1ReplaceSignalHandler*>(other)->property == property)
            return true;
        return false;
    }
};


class QDeclarative1PropertyChangesPrivate : public QDeclarative1StateOperationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1PropertyChanges)
public:
    QDeclarative1PropertyChangesPrivate() : decoded(true), restore(true),
                                isExplicit(false) {}

    QDeclarativeGuard<QObject> object;
    QByteArray data;

    bool decoded : 1;
    bool restore : 1;
    bool isExplicit : 1;

    void decode();

    class ExpressionChange {
    public:
        ExpressionChange(const QString &_name,
                         QDeclarativeBinding::Identifier _id,
                         QDeclarativeExpression *_expr)
            : name(_name), id(_id), expression(_expr) {}
        QString name;
        QDeclarativeBinding::Identifier id;
        QDeclarativeExpression *expression;
    };

    QList<QPair<QString, QVariant> > properties;
    QList<ExpressionChange> expressions;
    QList<QDeclarative1ReplaceSignalHandler*> signalReplacements;

    QDeclarativeProperty property(const QString &);
};

void
QDeclarative1PropertyChangesParser::compileList(QList<QPair<QString, QVariant> > &list,
                                     const QString &pre,
                                     const QDeclarativeCustomParserProperty &prop)
{
    QString propName = pre + prop.name();

    QList<QVariant> values = prop.assignedValues();
    for (int ii = 0; ii < values.count(); ++ii) {
        const QVariant &value = values.at(ii);

        if (value.userType() == qMetaTypeId<QDeclarativeCustomParserNode>()) {
            error(qvariant_cast<QDeclarativeCustomParserNode>(value),
                  QDeclarative1PropertyChanges::tr("PropertyChanges does not support creating state-specific objects."));
            continue;
        } else if(value.userType() == qMetaTypeId<QDeclarativeCustomParserProperty>()) {

            QDeclarativeCustomParserProperty prop =
                qvariant_cast<QDeclarativeCustomParserProperty>(value);
            QString pre = propName + QLatin1Char('.');
            compileList(list, pre, prop);

        } else {
            list << qMakePair(propName, value);
        }
    }
}

QByteArray
QDeclarative1PropertyChangesParser::compile(const QList<QDeclarativeCustomParserProperty> &props)
{
    QList<QPair<QString, QVariant> > data;
    for(int ii = 0; ii < props.count(); ++ii)
        compileList(data, QString(), props.at(ii));

    QByteArray rv;
    QDataStream ds(&rv, QIODevice::WriteOnly);

    ds << data.count();
    for(int ii = 0; ii < data.count(); ++ii) {
        QDeclarativeScript::Variant v = qvariant_cast<QDeclarativeScript::Variant>(data.at(ii).second);
        QVariant var;
        bool isScript = v.isScript();
        QDeclarativeBinding::Identifier id = 0;
        switch(v.type()) {
        case QDeclarativeScript::Variant::Boolean:
            var = QVariant(v.asBoolean());
            break;
        case QDeclarativeScript::Variant::Number:
            var = QVariant(v.asNumber());
            break;
        case QDeclarativeScript::Variant::String:
            var = QVariant(v.asString());
            break;
        case QDeclarativeScript::Variant::Invalid:
        case QDeclarativeScript::Variant::Script:
            var = QVariant(v.asScript());
            {
                id = rewriteBinding(v, data.at(ii).first);
            }
            break;
        }

        ds << data.at(ii).first << isScript << var;
        if (isScript)
            ds << id;
    }

    return rv;
}

void QDeclarative1PropertyChangesPrivate::decode()
{
    Q_Q(QDeclarative1PropertyChanges);
    if (decoded)
        return;

    QDataStream ds(&data, QIODevice::ReadOnly);

    int count;
    ds >> count;
    for (int ii = 0; ii < count; ++ii) {
        QString name;
        bool isScript;
        QVariant data;
        QDeclarativeBinding::Identifier id = QDeclarativeBinding::Invalid;
        ds >> name;
        ds >> isScript;
        ds >> data;
        if (isScript)
            ds >> id;

        QDeclarativeProperty prop = property(name);      //### better way to check for signal property?
        if (prop.type() & QDeclarativeProperty::SignalProperty) {
            QDeclarativeExpression *expression = new QDeclarativeExpression(qmlContext(q), object, data.toString());
            QDeclarativeData *ddata = QDeclarativeData::get(q);
            if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
                expression->setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber, ddata->columnNumber);
            QDeclarative1ReplaceSignalHandler *handler = new QDeclarative1ReplaceSignalHandler;
            handler->property = prop;
            handler->expression = expression;
            signalReplacements << handler;
        } else if (isScript) {
            QDeclarativeExpression *expression = new QDeclarativeExpression(qmlContext(q), object, data.toString());
            QDeclarativeData *ddata = QDeclarativeData::get(q);
            if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
                expression->setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber, ddata->columnNumber);
            expressions << ExpressionChange(name, id, expression);
        } else {
            properties << qMakePair(name, data);
        }
    }

    decoded = true;
    data.clear();
}

void QDeclarative1PropertyChangesParser::setCustomData(QObject *object,
                                            const QByteArray &data)
{
    QDeclarative1PropertyChangesPrivate *p =
        static_cast<QDeclarative1PropertyChangesPrivate *>(QObjectPrivate::get(object));
    p->data = data;
    p->decoded = false;
}

QDeclarative1PropertyChanges::QDeclarative1PropertyChanges()
: QDeclarative1StateOperation(*(new QDeclarative1PropertyChangesPrivate))
{
}

QDeclarative1PropertyChanges::~QDeclarative1PropertyChanges()
{
    Q_D(QDeclarative1PropertyChanges);
    for(int ii = 0; ii < d->expressions.count(); ++ii)
        delete d->expressions.at(ii).expression;
    for(int ii = 0; ii < d->signalReplacements.count(); ++ii)
        delete d->signalReplacements.at(ii);
}

QObject *QDeclarative1PropertyChanges::object() const
{
    Q_D(const QDeclarative1PropertyChanges);
    return d->object;
}

void QDeclarative1PropertyChanges::setObject(QObject *o)
{
    Q_D(QDeclarative1PropertyChanges);
    d->object = o;
}

/*!
    \qmlproperty bool QtQuick1::PropertyChanges::restoreEntryValues

    This property holds whether the previous values should be restored when
    leaving the state. 

    The default value is \c true. Setting this value to \c false creates a
    temporary state that has permanent effects on property values.
*/
bool QDeclarative1PropertyChanges::restoreEntryValues() const
{
    Q_D(const QDeclarative1PropertyChanges);
    return d->restore;
}

void QDeclarative1PropertyChanges::setRestoreEntryValues(bool v)
{
    Q_D(QDeclarative1PropertyChanges);
    d->restore = v;
}

QDeclarativeProperty
QDeclarative1PropertyChangesPrivate::property(const QString &property)
{
    Q_Q(QDeclarative1PropertyChanges);
    QDeclarativeProperty prop(object, property, qmlContext(q));
    if (!prop.isValid()) {
        qmlInfo(q) << QDeclarative1PropertyChanges::tr("Cannot assign to non-existent property \"%1\"").arg(property);
        return QDeclarativeProperty();
    } else if (!(prop.type() & QDeclarativeProperty::SignalProperty) && !prop.isWritable()) {
        qmlInfo(q) << QDeclarative1PropertyChanges::tr("Cannot assign to read-only property \"%1\"").arg(property);
        return QDeclarativeProperty();
    }
    return prop;
}

QDeclarative1PropertyChanges::ActionList QDeclarative1PropertyChanges::actions()
{
    Q_D(QDeclarative1PropertyChanges);

    d->decode();

    ActionList list;

    for (int ii = 0; ii < d->properties.count(); ++ii) {

        QDeclarative1Action a(d->object, d->properties.at(ii).first,
                 qmlContext(this), d->properties.at(ii).second);

        if (a.property.isValid()) {
            a.restore = restoreEntryValues();
            list << a;
        }
    }

    for (int ii = 0; ii < d->signalReplacements.count(); ++ii) {

        QDeclarative1ReplaceSignalHandler *handler = d->signalReplacements.at(ii);

        if (handler->property.isValid()) {
            QDeclarative1Action a;
            a.event = handler;
            list << a;
        }
    }

    for (int ii = 0; ii < d->expressions.count(); ++ii) {

        const QString &property = d->expressions.at(ii).name;
        QDeclarativeProperty prop = d->property(property);

        if (prop.isValid()) {
            QDeclarative1Action a;
            a.restore = restoreEntryValues();
            a.property = prop;
            a.fromValue = a.property.read();
            a.specifiedObject = d->object;
            a.specifiedProperty = property;

            if (d->isExplicit) {
                a.toValue = d->expressions.at(ii).expression->evaluate();
            } else {
                QDeclarativeExpression *e = d->expressions.at(ii).expression;

                QDeclarativeBinding::Identifier id = d->expressions.at(ii).id;
                QDeclarativeBinding *newBinding = id != QDeclarativeBinding::Invalid ? QDeclarativeBinding::createBinding(id, object(), qmlContext(this), e->sourceFile(), e->lineNumber()) : 0;
                if (!newBinding) {
                    newBinding = new QDeclarativeBinding(e->expression(), object(), qmlContext(this));
                    newBinding->setSourceLocation(e->sourceFile(), e->lineNumber(), e->columnNumber());
                }
                newBinding->setTarget(prop);
                a.toBinding = newBinding;
                a.deletableToBinding = true;
            }

            list << a;
        }
    }

    return list;
}

/*!
    \qmlproperty bool QtQuick1::PropertyChanges::explicit

    If explicit is set to true, any potential bindings will be interpreted as
    once-off assignments that occur when the state is entered.

    In the following example, the addition of explicit prevents \c myItem.width from
    being bound to \c parent.width. Instead, it is assigned the value of \c parent.width
    at the time of the state change.
    \qml
    PropertyChanges {
        target: myItem
        explicit: true
        width: parent.width
    }
    \endqml

    By default, explicit is false.
*/
bool QDeclarative1PropertyChanges::isExplicit() const
{
    Q_D(const QDeclarative1PropertyChanges);
    return d->isExplicit;
}

void QDeclarative1PropertyChanges::setIsExplicit(bool e)
{
    Q_D(QDeclarative1PropertyChanges);
    d->isExplicit = e;
}

bool QDeclarative1PropertyChanges::containsValue(const QString &name) const
{
    Q_D(const QDeclarative1PropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;

    QListIterator<PropertyEntry> propertyIterator(d->properties);
    while (propertyIterator.hasNext()) {
        const PropertyEntry &entry = propertyIterator.next();
        if (entry.first == name) {
            return true;
        }
    }

    return false;
}

bool QDeclarative1PropertyChanges::containsExpression(const QString &name) const
{
    Q_D(const QDeclarative1PropertyChanges);
    typedef QDeclarative1PropertyChangesPrivate::ExpressionChange ExpressionEntry;

    QListIterator<ExpressionEntry> expressionIterator(d->expressions);
    while (expressionIterator.hasNext()) {
        const ExpressionEntry &entry = expressionIterator.next();
        if (entry.name == name) {
            return true;
        }
    }

    return false;
}

bool QDeclarative1PropertyChanges::containsProperty(const QString &name) const
{
    return containsValue(name) || containsExpression(name);
}

void QDeclarative1PropertyChanges::changeValue(const QString &name, const QVariant &value)
{
    Q_D(QDeclarative1PropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;
    typedef QDeclarative1PropertyChangesPrivate::ExpressionChange ExpressionEntry;

    QMutableListIterator<ExpressionEntry> expressionIterator(d->expressions);
    while (expressionIterator.hasNext()) {
        const ExpressionEntry &entry = expressionIterator.next();
        if (entry.name == name) {
            expressionIterator.remove();
            if (state() && state()->isStateActive()) {
                QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(d->property(name));
                if (oldBinding) {
                    QDeclarativePropertyPrivate::setBinding(d->property(name), 0);
                    oldBinding->destroy();
                }
                d->property(name).write(value);
            }

            d->properties.append(PropertyEntry(name, value));
            return;
        }
    }

    QMutableListIterator<PropertyEntry> propertyIterator(d->properties);
    while (propertyIterator.hasNext()) {
        PropertyEntry &entry = propertyIterator.next();
        if (entry.first == name) {
            entry.second = value;
            if (state() && state()->isStateActive())
                d->property(name).write(value);
            return;
        }
    }

    QDeclarative1Action action;
    action.restore = restoreEntryValues();
    action.property = d->property(name);
    action.fromValue = action.property.read();
    action.specifiedObject = object();
    action.specifiedProperty = name;
    action.toValue = value;

    propertyIterator.insert(PropertyEntry(name, value));
    if (state() && state()->isStateActive()) {
        state()->addEntryToRevertList(action);
        QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(action.property);
        if (oldBinding)
            oldBinding->setEnabled(false, QDeclarativePropertyPrivate::DontRemoveBinding | QDeclarativePropertyPrivate::BypassInterceptor);
        d->property(name).write(value);
    }
}

void QDeclarative1PropertyChanges::changeExpression(const QString &name, const QString &expression)
{
    Q_D(QDeclarative1PropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;
    typedef QDeclarative1PropertyChangesPrivate::ExpressionChange ExpressionEntry;

    bool hadValue = false;

    QMutableListIterator<PropertyEntry> propertyIterator(d->properties);
    while (propertyIterator.hasNext()) {
        PropertyEntry &entry = propertyIterator.next();
        if (entry.first == name) {
            propertyIterator.remove();
            hadValue = true;
            break;
        }
    }

    QMutableListIterator<ExpressionEntry> expressionIterator(d->expressions);
    while (expressionIterator.hasNext()) {
        const ExpressionEntry &entry = expressionIterator.next();
        if (entry.name == name) {
            entry.expression->setExpression(expression);
            if (state() && state()->isStateActive()) {
                QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(d->property(name));
                if (oldBinding) {
                       QDeclarativePropertyPrivate::setBinding(d->property(name), 0);
                       oldBinding->destroy();
                }

                QDeclarativeBinding *newBinding = new QDeclarativeBinding(expression, object(), qmlContext(this));
                newBinding->setTarget(d->property(name));
                QDeclarativePropertyPrivate::setBinding(d->property(name), newBinding, QDeclarativePropertyPrivate::DontRemoveBinding | QDeclarativePropertyPrivate::BypassInterceptor);
            }
            return;
        }
    }

    QDeclarativeExpression *newExpression = new QDeclarativeExpression(qmlContext(this), d->object, expression);
    expressionIterator.insert(ExpressionEntry(name, QDeclarativeBinding::Invalid, newExpression));

    if (state() && state()->isStateActive()) {
        if (hadValue) {
            QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(d->property(name));
            if (oldBinding) {
                oldBinding->setEnabled(false, QDeclarativePropertyPrivate::DontRemoveBinding | QDeclarativePropertyPrivate::BypassInterceptor);
                state()->changeBindingInRevertList(object(), name, oldBinding);
            }

            QDeclarativeBinding *newBinding = new QDeclarativeBinding(expression, object(), qmlContext(this));
            newBinding->setTarget(d->property(name));
            QDeclarativePropertyPrivate::setBinding(d->property(name), newBinding, QDeclarativePropertyPrivate::DontRemoveBinding | QDeclarativePropertyPrivate::BypassInterceptor);
        } else {
            QDeclarative1Action action;
            action.restore = restoreEntryValues();
            action.property = d->property(name);
            action.fromValue = action.property.read();
            action.specifiedObject = object();
            action.specifiedProperty = name;


            if (d->isExplicit) {
                action.toValue = newExpression->evaluate();
            } else {
                QDeclarativeBinding *newBinding = new QDeclarativeBinding(newExpression->expression(), object(), qmlContext(this));
                newBinding->setTarget(d->property(name));
                action.toBinding = newBinding;
                action.deletableToBinding = true;

                state()->addEntryToRevertList(action);
                QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(action.property);
                if (oldBinding)
                    oldBinding->setEnabled(false, QDeclarativePropertyPrivate::DontRemoveBinding | QDeclarativePropertyPrivate::BypassInterceptor);

                QDeclarativePropertyPrivate::setBinding(action.property, newBinding, QDeclarativePropertyPrivate::DontRemoveBinding | QDeclarativePropertyPrivate::BypassInterceptor);
            }
        }
    }
    // what about the signal handler?
}

QVariant QDeclarative1PropertyChanges::property(const QString &name) const
{
    Q_D(const QDeclarative1PropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;
    typedef QDeclarative1PropertyChangesPrivate::ExpressionChange ExpressionEntry;

    QListIterator<PropertyEntry> propertyIterator(d->properties);
    while (propertyIterator.hasNext()) {
        const PropertyEntry &entry = propertyIterator.next();
        if (entry.first == name) {
            return entry.second;
        }
    }

    QListIterator<ExpressionEntry> expressionIterator(d->expressions);
    while (expressionIterator.hasNext()) {
        const ExpressionEntry &entry = expressionIterator.next();
        if (entry.name == name) {
            return QVariant(entry.expression->expression());
        }
    }

    return QVariant();
}

void QDeclarative1PropertyChanges::removeProperty(const QString &name)
{
    Q_D(QDeclarative1PropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;
    typedef QDeclarative1PropertyChangesPrivate::ExpressionChange ExpressionEntry;

    QMutableListIterator<ExpressionEntry> expressionIterator(d->expressions);
    while (expressionIterator.hasNext()) {
        const ExpressionEntry &entry = expressionIterator.next();
        if (entry.name == name) {
            expressionIterator.remove();
            state()->removeEntryFromRevertList(object(), name);
            return;
        }
    }

    QMutableListIterator<PropertyEntry> propertyIterator(d->properties);
    while (propertyIterator.hasNext()) {
        const PropertyEntry &entry = propertyIterator.next();
        if (entry.first == name) {
            propertyIterator.remove();
            state()->removeEntryFromRevertList(object(), name);
            return;
        }
    }
}

QVariant QDeclarative1PropertyChanges::value(const QString &name) const
{
    Q_D(const QDeclarative1PropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;

    QListIterator<PropertyEntry> propertyIterator(d->properties);
    while (propertyIterator.hasNext()) {
        const PropertyEntry &entry = propertyIterator.next();
        if (entry.first == name) {
            return entry.second;
        }
    }

    return QVariant();
}

QString QDeclarative1PropertyChanges::expression(const QString &name) const
{
    Q_D(const QDeclarative1PropertyChanges);
    typedef QDeclarative1PropertyChangesPrivate::ExpressionChange ExpressionEntry;

    QListIterator<ExpressionEntry> expressionIterator(d->expressions);
    while (expressionIterator.hasNext()) {
        const ExpressionEntry &entry = expressionIterator.next();
        if (entry.name == name) {
            return entry.expression->expression();
        }
    }

    return QString();
}

void QDeclarative1PropertyChanges::detachFromState()
{
    if (state())
        state()->removeAllEntriesFromRevertList(object());
}

void QDeclarative1PropertyChanges::attachToState()
{
    if (state())
        state()->addEntriesToRevertList(actions());
}



QT_END_NAMESPACE
