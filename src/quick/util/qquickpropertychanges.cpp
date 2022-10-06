// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpropertychanges_p.h"

#include <private/qqmlopenmetaobject_p.h>
#include <private/qqmlengine_p.h>

#include <qqmlinfo.h>
#include <private/qqmlcustomparser_p.h>
#include <qqmlexpression.h>
#include <private/qqmlbinding_p.h>
#include <qqmlcontext.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlcontext_p.h>
#include <private/qquickstate_p_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qqmlpropertybinding_p.h>
#include <private/qqmlirbuilder_p.h>

#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PropertyChanges
    \inqmlmodule QtQuick
    \ingroup qtquick-states
    \brief Describes new property bindings or values for a state.

    PropertyChanges is used to define the property values or bindings in a
    \l State. This enables an item's property values to be changed when it
    \l {Qt Quick States}{changes between states}.

    To create a PropertyChanges object, bind to properties of the target
    item like you would bind to local properties. This way you can define
    the new property values or bindings. For example:

    \snippet qml/propertychanges.qml import
    \codeline
    \snippet qml/propertychanges.qml 0

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
        myMouseArea.onClicked: doSomethingDifferent()
    }
    \endqml

    \note PropertyChanges can be used to change anchor margins, but not other anchor
    values; use AnchorChanges for this instead. Similarly, to change an \l Item's
    \l {Item::}{parent} value, use ParentChange instead.


    \section2 Resetting Property Values

    The \c undefined value can be used to reset the property value for a state.
    In the following example, when \c myText changes to the \e widerText
    state, its \c width property is reset, giving the text its natural width
    and displaying the whole string on a single line.

    \snippet qml/propertychanges.qml reset


    \section2 Immediate Property Changes in Transitions

    When \l{Animation and Transitions in Qt Quick}{Transitions} are used to animate
    state changes, they animate properties from their values in the current
    state to those defined in the new state (as defined by PropertyChanges
    objects). However, it is sometimes desirable to set a property value
    \e immediately during a \l Transition, without animation; in these cases,
    the PropertyAction type can be used to force an immediate property
    change.

    See the PropertyAction documentation for more details.

    \note The \l{Item::}{visible} and \l{Item::}{enabled} properties of \l Item do not behave
    exactly the same as other properties in PropertyChanges. Since these properties can be
    changed implicitly through their parent's state, they should be set explicitly in all PropertyChanges.
    An item will still not be enabled/visible if one of its parents is not enabled or visible.

    \note For backwards compatibility with Qt 5, you can also specify PropertyChanges using
    the \l target property and plain property names without IDs. For example:
    \c {PropertyChanges { target: myItem; x: 15 }}. The form with ID instead of \l target
    is recommended. You may also need to use the form with \l target if the file is to be
    edited with \l{Qt Design Studio}. Mind that \l{Qt Design Studio} also imposes a number
    of further restrictions on the files it can work with.

    \sa {Qt Quick Examples - Animation#States}{States example}, {Qt Quick States}, {Qt QML}
*/

/*!
    \qmlproperty QtObject QtQuick::PropertyChanges::target
    This property holds the object which contains the properties to be changed.

    \note You generally don't have to use this property. It only exists for
    compatibility with Qt 5 and for compatibility with \l{Qt Design Studio}.
*/

class QQuickReplaceSignalHandler : public QQuickStateActionEvent
{
public:
    QQuickReplaceSignalHandler() {}
    ~QQuickReplaceSignalHandler() {}

    EventType type() const override { return SignalHandler; }

    QQmlProperty property;
    QQmlRefPointer<QQmlBoundSignalExpression> expression;
    QQmlRefPointer<QQmlBoundSignalExpression> reverseExpression;
    QQmlRefPointer<QQmlBoundSignalExpression> rewindExpression;

    void execute() override {
        QQmlPropertyPrivate::setSignalExpression(property, expression.data());
    }

    bool isReversable() override { return true; }
    void reverse() override {
        QQmlPropertyPrivate::setSignalExpression(property, reverseExpression.data());
    }

    void saveOriginals() override {
        saveCurrentValues();
        reverseExpression = rewindExpression;
    }

    bool needsCopy() override { return true; }
    void copyOriginals(QQuickStateActionEvent *other) override
    {
        QQuickReplaceSignalHandler *rsh = static_cast<QQuickReplaceSignalHandler*>(other);
        saveCurrentValues();
        if (rsh == this)
            return;
        reverseExpression = rsh->reverseExpression;
    }

    void rewind() override {
        QQmlPropertyPrivate::setSignalExpression(property, rewindExpression.data());
    }
    void saveCurrentValues() override {
        rewindExpression = QQmlPropertyPrivate::signalExpression(property);
    }

    bool mayOverride(QQuickStateActionEvent *other) override {
        if (other == this)
            return true;
        if (other->type() != type())
            return false;
        if (static_cast<QQuickReplaceSignalHandler*>(other)->property == property)
            return true;
        return false;
    }
};


class QQuickPropertyChangesPrivate : public QQuickStateOperationPrivate
{
    Q_DECLARE_PUBLIC(QQuickPropertyChanges)
public:
    QQuickPropertyChangesPrivate() : decoded(true), restore(true),
                                isExplicit(false) {}

    QPointer<QObject> object;
    QList<const QV4::CompiledData::Binding *> bindings;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;

    bool decoded : 1;
    bool restore : 1;
    bool isExplicit : 1;

    void decode();
    void decodeBinding(const QString &propertyPrefix, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &qmlUnit, const QV4::CompiledData::Binding *binding);

    class ExpressionChange {
    public:
        ExpressionChange(const QString &_name,
                         const QV4::CompiledData::Binding *_binding,
                         QQmlBinding::Identifier _id,
                         const QString& _expr,
                         const QUrl &_url,
                         int _line,
                         int _column)
            : name(_name), binding(_binding), id(_id), expression(_expr), url(_url), line(_line), column(_column) {}
        QString name;
        const QV4::CompiledData::Binding *binding;
        QQmlBinding::Identifier id;
        QString expression;
        QUrl url;
        int line;
        int column;
    };

    QList<QPair<QString, QVariant> > properties;
    QList<ExpressionChange> expressions;
    QList<QQuickReplaceSignalHandler*> signalReplacements;

    QQmlProperty property(const QString &);
};

void QQuickPropertyChangesParser::verifyList(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QV4::CompiledData::Binding *binding)
{
    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_Object:
        error(compilationUnit->objectAt(binding->value.objectIndex),
              QQuickPropertyChanges::tr(
                      "PropertyChanges does not support creating state-specific objects."));
        break;
    case QV4::CompiledData::Binding::Type_GroupProperty:
    case QV4::CompiledData::Binding::Type_AttachedProperty: {
        const QV4::CompiledData::Object *subObj = compilationUnit->objectAt(binding->value.objectIndex);
        const QV4::CompiledData::Binding *subBinding = subObj->bindingTable();
        for (quint32 i = 0; i < subObj->nBindings; ++i, ++subBinding)
            verifyList(compilationUnit, subBinding);
        break;
    }
    default:
        break;
    }
}

void QQuickPropertyChangesPrivate::decode()
{
    if (decoded)
        return;

    for (const QV4::CompiledData::Binding *binding : std::as_const(bindings))
        decodeBinding(QString(), compilationUnit, binding);

    bindings.clear();

    decoded = true;
}

void QQuickPropertyChangesPrivate::decodeBinding(const QString &propertyPrefix, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QV4::CompiledData::Binding *binding)
{
    Q_Q(QQuickPropertyChanges);

    QString propertyName = propertyPrefix + compilationUnit->stringAt(binding->propertyNameIndex);

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_GroupProperty:
    case QV4::CompiledData::Binding::Type_AttachedProperty: {
        QString pre = propertyName + QLatin1Char('.');
        const QV4::CompiledData::Object *subObj = compilationUnit->objectAt(binding->value.objectIndex);
        const QV4::CompiledData::Binding *subBinding = subObj->bindingTable();
        for (quint32 i = 0; i < subObj->nBindings; ++i, ++subBinding) {
            decodeBinding(pre, compilationUnit, subBinding);
        }
        return;
    }
    default:
        break;
    }

    if (binding->isSignalHandler() || QmlIR::IRBuilder::isSignalPropertyName(propertyName)) {
        QQmlProperty prop = property(propertyName);
        if (prop.isSignalProperty()) {
            QQuickReplaceSignalHandler *handler = new QQuickReplaceSignalHandler;
            handler->property = prop;
            handler->expression.adopt(
                        new QQmlBoundSignalExpression(
                            prop.object(), QQmlPropertyPrivate::get(prop)->signalIndex(),
                            QQmlContextData::get(qmlContext(q)), prop.object(),
                            compilationUnit->runtimeFunctions.at(binding->value.compiledScriptIndex)));
            signalReplacements << handler;
            return;
        }
    }

    if (binding->type() == QV4::CompiledData::Binding::Type_Script
            || binding->isTranslationBinding()) {
        QUrl url = QUrl();
        int line = -1;
        int column = -1;

        QQmlData *ddata = QQmlData::get(q);
        if (ddata && ddata->outerContext && !ddata->outerContext->url().isEmpty()) {
            url = ddata->outerContext->url();
            line = ddata->lineNumber;
            column = ddata->columnNumber;
        }

        QString expression;
        QQmlBinding::Identifier id = QQmlBinding::Invalid;

        if (!binding->isTranslationBinding()) {
            expression = compilationUnit->bindingValueAsString(binding);
            id = binding->value.compiledScriptIndex;
        }
        expressions << ExpressionChange(propertyName, binding, id, expression, url, line, column);
        return;
    }

    QVariant var;
    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_Script:
    case QV4::CompiledData::Binding::Type_Translation:
    case QV4::CompiledData::Binding::Type_TranslationById:
        Q_UNREACHABLE();
    case QV4::CompiledData::Binding::Type_String:
        var = compilationUnit->bindingValueAsString(binding);
        break;
    case QV4::CompiledData::Binding::Type_Number:
        var = compilationUnit->bindingValueAsNumber(binding);
        break;
    case QV4::CompiledData::Binding::Type_Boolean:
        var = binding->valueAsBoolean();
        break;
    case QV4::CompiledData::Binding::Type_Null:
        var = QVariant::fromValue(nullptr);
        break;
    default:
        break;
    }

    properties << qMakePair(propertyName, var);
}

void QQuickPropertyChangesParser::verifyBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &props)
{
    for (int ii = 0; ii < props.size(); ++ii)
        verifyList(compilationUnit, props.at(ii));
}

void QQuickPropertyChangesParser::applyBindings(QObject *obj, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings)
{
    QQuickPropertyChangesPrivate *p =
        static_cast<QQuickPropertyChangesPrivate *>(QObjectPrivate::get(obj));
    p->bindings = bindings;
    p->compilationUnit = compilationUnit;
    p->decoded = false;

    QQmlData *data = QQmlData::get(obj);
    Q_ASSERT(data && !data->wasDeleted(obj));
    data->releaseDeferredData();
}

QQuickPropertyChanges::QQuickPropertyChanges()
: QQuickStateOperation(*(new QQuickPropertyChangesPrivate))
{
}

QQuickPropertyChanges::~QQuickPropertyChanges()
{
    Q_D(QQuickPropertyChanges);
    for(int ii = 0; ii < d->signalReplacements.size(); ++ii)
        delete d->signalReplacements.at(ii);
}

QObject *QQuickPropertyChanges::object() const
{
    Q_D(const QQuickPropertyChanges);
    return d->object;
}

void QQuickPropertyChanges::setObject(QObject *o)
{
    Q_D(QQuickPropertyChanges);
    if (o != d->object) {
        d->object = o;
        emit objectChanged();
    }
}

/*!
    \qmlproperty bool QtQuick::PropertyChanges::restoreEntryValues

    This property holds whether the previous values should be restored when
    leaving the state.

    The default value is \c true. Setting this value to \c false creates a
    temporary state that has permanent effects on property values.
*/
bool QQuickPropertyChanges::restoreEntryValues() const
{
    Q_D(const QQuickPropertyChanges);
    return d->restore;
}

void QQuickPropertyChanges::setRestoreEntryValues(bool v)
{
    Q_D(QQuickPropertyChanges);
    if (v != d->restore) {
        d->restore = v;
        emit restoreEntryValuesChanged();
    }
}

QQmlProperty
QQuickPropertyChangesPrivate::property(const QString &property)
{
    Q_Q(QQuickPropertyChanges);
    QQmlData *ddata = QQmlData::get(q);
    QQmlProperty prop = QQmlPropertyPrivate::create(
                object, property, ddata ? ddata->outerContext : QQmlRefPointer<QQmlContextData>(),
                QQmlPropertyPrivate::InitFlag::AllowId | QQmlPropertyPrivate::InitFlag::AllowSignal);
    if (!prop.isValid()) {
        qmlWarning(q) << QQuickPropertyChanges::tr("Cannot assign to non-existent property \"%1\"").arg(property);
        return QQmlProperty();
    } else if (!(prop.type() & QQmlProperty::SignalProperty) && !prop.isWritable()) {
        qmlWarning(q) << QQuickPropertyChanges::tr("Cannot assign to read-only property \"%1\"").arg(property);
        return QQmlProperty();
    }
    return prop;
}

QQuickPropertyChanges::ActionList QQuickPropertyChanges::actions()
{
    Q_D(QQuickPropertyChanges);

    d->decode();

    ActionList list;

    for (int ii = 0; ii < d->properties.size(); ++ii) {
        QQmlProperty prop = d->property(d->properties.at(ii).first);

        QQuickStateAction a(d->object, prop, d->properties.at(ii).first,
                            d->properties.at(ii).second);

        if (a.property.isValid()) {
            a.restore = restoreEntryValues();
            list << a;
        }
    }

    for (int ii = 0; ii < d->signalReplacements.size(); ++ii) {
        QQuickReplaceSignalHandler *handler = d->signalReplacements.at(ii);

        if (handler->property.isValid()) {
            QQuickStateAction a;
            a.event = handler;
            list << a;
        }
    }

    for (int ii = 0; ii < d->expressions.size(); ++ii) {

        QQuickPropertyChangesPrivate::ExpressionChange e = d->expressions.at(ii);
        const QString &property = e.name;
        QQmlProperty prop = d->property(property);

        if (prop.isValid()) {
            QQuickStateAction a;
            a.restore = restoreEntryValues();
            a.property = prop;
            a.fromValue = a.property.read();
            a.specifiedObject = d->object;
            a.specifiedProperty = property;

            QQmlRefPointer<QQmlContextData> context = QQmlContextData::get(qmlContext(this));
            QV4::Scope scope(qmlEngine(this)->handle());
            QV4::Scoped<QV4::QmlContext> qmlCtxt(scope, QV4::QmlContext::create(scope.engine->rootContext(), context, object()));

            if (d->isExplicit) {
                // in this case, we don't want to assign a binding, per se,
                // so we evaluate the expression and assign the result.
                // XXX TODO: add a static QQmlJavaScriptExpression::evaluate(QString)
                // so that we can avoid creating then destroying the binding in this case.
                std::unique_ptr<QQmlBinding> newBinding = nullptr;
                if (e.binding && e.binding->isTranslationBinding()) {
                    newBinding.reset(QQmlBinding::createTranslationBinding(d->compilationUnit, e.binding, object(), context));
                } else if (e.id != QQmlBinding::Invalid) {
                    newBinding.reset(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, d->compilationUnit->runtimeFunctions.at(e.id), object(), context, qmlCtxt));
                } else {
                    newBinding.reset(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, e.expression, object(), context, e.url.toString(), e.line));
                }
                a.toValue = newBinding->evaluate();
            } else {
                QQmlAnyBinding newBinding = nullptr;
                if (e.binding && e.binding->isTranslationBinding()) {
                    newBinding = QQmlAnyBinding::createTranslationBinding(prop, d->compilationUnit, e.binding, object(), context);
                } else if (e.id != QQmlBinding::Invalid) {
                    newBinding = QQmlAnyBinding::createFromFunction(prop,
                                                                       d->compilationUnit->runtimeFunctions.at(e.id),
                                                                       object(), context, qmlCtxt);
                } else {
                    newBinding = QQmlAnyBinding::createFromCodeString(prop, e.expression, object(), context, e.url.toString(), e.line);
                }

                a.toBinding = newBinding;
                a.deletableToBinding = true;
            }

            list << a;
        }
    }

    return list;
}

/*!
    \qmlproperty bool QtQuick::PropertyChanges::explicit

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
bool QQuickPropertyChanges::isExplicit() const
{
    Q_D(const QQuickPropertyChanges);
    return d->isExplicit;
}

void QQuickPropertyChanges::setIsExplicit(bool e)
{
    Q_D(QQuickPropertyChanges);
    if (e != d->isExplicit) {
        d->isExplicit = e;
        emit isExplicitChanged();
    }
}

bool QQuickPropertyChanges::containsValue(const QString &name) const
{
    Q_D(const QQuickPropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;

    for (const PropertyEntry &entry : d->properties) {
        if (entry.first == name) {
            return true;
        }
    }

    return false;
}

bool QQuickPropertyChanges::containsExpression(const QString &name) const
{
    Q_D(const QQuickPropertyChanges);
    typedef QQuickPropertyChangesPrivate::ExpressionChange ExpressionEntry;

    for (const ExpressionEntry &entry : d->expressions) {
        if (entry.name == name) {
            return true;
        }
    }

    return false;
}

bool QQuickPropertyChanges::containsProperty(const QString &name) const
{
    return containsValue(name) || containsExpression(name);
}

void QQuickPropertyChanges::changeValue(const QString &name, const QVariant &value)
{
    Q_D(QQuickPropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;

    for (auto it = d->expressions.begin(), end = d->expressions.end(); it != end; ++it) {
        if (it->name == name) {
            d->expressions.erase(it);
            if (state() && state()->isStateActive()) {
                QQmlPropertyPrivate::removeBinding(d->property(name));
                d->property(name).write(value);
            }

            d->properties.append(PropertyEntry(name, value));
            return;
        }
    }

    for (auto it = d->properties.begin(), end = d->properties.end(); it != end; ++it) {
        if (it->first == name) {
            it->second = value;
            if (state() && state()->isStateActive())
                d->property(name).write(value);
            return;
        }
    }

    QQuickStateAction action;
    action.restore = restoreEntryValues();
    action.property = d->property(name);
    action.fromValue = action.property.read();
    action.specifiedObject = object();
    action.specifiedProperty = name;
    action.toValue = value;

    d->properties.append(PropertyEntry(name, value));
    if (state() && state()->isStateActive()) {
        state()->addEntryToRevertList(action);
        QQmlAbstractBinding *oldBinding = QQmlPropertyPrivate::binding(action.property);
        if (oldBinding)
            oldBinding->setEnabled(false, QQmlPropertyData::DontRemoveBinding | QQmlPropertyData::BypassInterceptor);
        d->property(name).write(value);
    }
}

void QQuickPropertyChanges::changeExpression(const QString &name, const QString &expression)
{
    Q_D(QQuickPropertyChanges);
    typedef QQuickPropertyChangesPrivate::ExpressionChange ExpressionEntry;

    bool hadValue = false;

    for (auto it = d->properties.begin(), end = d->properties.end(); it != end; ++it) {
        if (it->first == name) {
            d->properties.erase(it);
            hadValue = true;
            break;
        }
    }

    for (auto it = d->expressions.begin(), end = d->expressions.end(); it != end; ++it) {
        if (it->name == name) {
            it->expression = expression;
            if (state() && state()->isStateActive()) {
                auto prop = d->property(name);
                auto context = QQmlContextData::get(qmlContext(this));
                QString url;
                int lineNumber = 0;
                QQmlAnyBinding::createFromCodeString(prop, expression, object(), context, url, lineNumber).installOn(prop);
            }
            return;
        }
    }

    // adding a new expression.
    d->expressions.append(ExpressionEntry(name, nullptr, QQmlBinding::Invalid, expression, QUrl(), -1, -1));

    const QString url;
    const quint16 lineNumber = 0;
    if (state() && state()->isStateActive()) {
        if (hadValue) {
            auto prop = d->property(name);
            QQmlAnyBinding oldBinding = QQmlAnyBinding::takeFrom(prop);
            if (oldBinding)
                state()->changeBindingInRevertList(object(), name, oldBinding);

            QQmlAnyBinding::createFromCodeString(prop, expression, object(), QQmlContextData::get(qmlContext(this)), url, lineNumber).installOn(prop);
        } else {
            QQuickStateAction action;
            action.restore = restoreEntryValues();
            action.property = d->property(name);
            action.fromValue = action.property.read();
            action.specifiedObject = object();
            action.specifiedProperty = name;

            QQmlAnyBinding newBinding;
            if (d->isExplicit) {
                newBinding = QQmlBinding::create(
                        &QQmlPropertyPrivate::get(action.property)->core, expression,
                        object(), QQmlContextData::get(qmlContext(this)));
            } else {
                const auto prop = action.property;
                const auto context = QQmlContextData::get(qmlContext(this));
                newBinding = QQmlAnyBinding::createFromCodeString(prop, expression, object(), context, url, lineNumber);
            }
            if (d->isExplicit) {
                // don't assign the binding, merely evaluate the expression.
                // XXX TODO: add a static QQmlJavaScriptExpression::evaluate(QString)
                // so that we can avoid creating then destroying the binding in this case.
                action.toValue = static_cast<QQmlBinding *>(newBinding.asAbstractBinding())->evaluate();
            } else {
                // TODO: replace binding would be more efficient for new-style properties
                QQmlAnyBinding::removeBindingFrom(action.property);
                newBinding.installOn(action.property);
                action.toBinding = newBinding;
                action.deletableToBinding = true;
                state()->addEntryToRevertList(action);
            }
        }
    }
    // what about the signal handler?
}

QVariant QQuickPropertyChanges::property(const QString &name) const
{
    Q_D(const QQuickPropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;
    typedef QQuickPropertyChangesPrivate::ExpressionChange ExpressionEntry;

    for (const PropertyEntry &entry : d->properties) {
        if (entry.first == name) {
            return entry.second;
        }
    }

    for (const ExpressionEntry &entry : d->expressions) {
        if (entry.name == name) {
            return QVariant(entry.expression);
        }
    }

    return QVariant();
}

void QQuickPropertyChanges::removeProperty(const QString &name)
{
    Q_D(QQuickPropertyChanges);

    for (auto it = d->expressions.begin(), end = d->expressions.end(); it != end; ++it) {
        if (it->name == name) {
            d->expressions.erase(it);
            state()->removeEntryFromRevertList(object(), name);
            return;
        }
    }

    for (auto it = d->properties.begin(), end = d->properties.end(); it != end; ++it) {
        if (it->first == name) {
            d->properties.erase(it);
            state()->removeEntryFromRevertList(object(), name);
            return;
        }
    }
}

QVariant QQuickPropertyChanges::value(const QString &name) const
{
    Q_D(const QQuickPropertyChanges);
    typedef QPair<QString, QVariant> PropertyEntry;

    for (const PropertyEntry &entry : d->properties) {
        if (entry.first == name) {
            return entry.second;
        }
    }

    return QVariant();
}

QString QQuickPropertyChanges::expression(const QString &name) const
{
    Q_D(const QQuickPropertyChanges);
    typedef QQuickPropertyChangesPrivate::ExpressionChange ExpressionEntry;

    for (const ExpressionEntry &entry : d->expressions) {
        if (entry.name == name) {
            return entry.expression;
        }
    }

    return QString();
}

void QQuickPropertyChanges::detachFromState()
{
    if (state())
        state()->removeAllEntriesFromRevertList(object());
}

void QQuickPropertyChanges::attachToState()
{
    if (state())
        state()->addEntriesToRevertList(actions());
}

QT_END_NAMESPACE

#include "moc_qquickpropertychanges_p.cpp"
