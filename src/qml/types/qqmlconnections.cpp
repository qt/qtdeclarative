// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlconnections_p.h"

#include <private/qqmlexpression_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlboundsignal_p.h>
#include <qqmlcontext.h>
#include <private/qqmlcontext_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <qqmlinfo.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQmlConnections, "qt.qml.connections")

class QQmlConnectionsPrivate : public QObjectPrivate
{
public:
    QList<QQmlBoundSignal*> boundsignals;
    QQmlGuard<QObject> target;

    bool enabled = true;
    bool targetSet = false;
    bool ignoreUnknownSignals = false;
    bool componentcomplete = true;

    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QList<const QV4::CompiledData::Binding *> bindings;
};

/*!
    \qmltype Connections
    \instantiates QQmlConnections
    \inqmlmodule QtQml
    \ingroup qtquick-interceptors
    \brief Describes generalized connections to signals.

    A Connections object creates a connection to a QML signal.

    When connecting to signals in QML, the usual way is to create an
    "on<Signal>" handler that reacts when a signal is received, like this:

    \qml
    MouseArea {
        onClicked: (mouse)=> { foo(mouse) }
    }
    \endqml

    However, it is not possible to connect to a signal in this way in some
    cases, such as when:

    \list
        \li Multiple connections to the same signal are required
        \li Creating connections outside the scope of the signal sender
        \li Connecting to targets not defined in QML
    \endlist

    When any of these are needed, the Connections type can be used instead.

    For example, the above code can be changed to use a Connections object,
    like this:

    \qml
    MouseArea {
        Connections {
            function onClicked(mouse) { foo(mouse) }
        }
    }
    \endqml

    More generally, the Connections object can be a child of some object other than
    the sender of the signal:

    \qml
    MouseArea {
        id: area
    }
    // ...
    \endqml
    \qml
    Connections {
        target: area
        function onClicked(mouse) { foo(mouse) }
    }
    \endqml

    \note For backwards compatibility you can also specify the signal handlers
    without \c{function}, like you would specify them directly in the target
    object. This is not recommended. If you specify one signal handler this way,
    then all signal handlers specified as \c{function} in the same Connections
    object are ignored.

    \sa {Qt QML}
*/
QQmlConnections::QQmlConnections(QObject *parent) :
    QObject(*(new QQmlConnectionsPrivate), parent)
{
}

/*!
    \qmlproperty QtObject QtQml::Connections::target
    This property holds the object that sends the signal.

    If this property is not set, the \c target defaults to the parent of the Connection.

    If set to null, no connection is made and any signal handlers are ignored
    until the target is not null.
*/
QObject *QQmlConnections::target() const
{
    Q_D(const QQmlConnections);
    return d->targetSet ? d->target.data() : parent();
}

class QQmlBoundSignalDeleter : public QObject
{
public:
    QQmlBoundSignalDeleter(QQmlBoundSignal *signal) : m_signal(signal) { m_signal->removeFromObject(); }
    ~QQmlBoundSignalDeleter() { delete m_signal; }

private:
    QQmlBoundSignal *m_signal;
};

void QQmlConnections::setTarget(QObject *obj)
{
    Q_D(QQmlConnections);
    if (d->targetSet && d->target == obj)
        return;
    d->targetSet = true; // even if setting to 0, it is *set*
    for (QQmlBoundSignal *s : std::as_const(d->boundsignals)) {
        // It is possible that target is being changed due to one of our signal
        // handlers -> use deleteLater().
        if (s->isNotifying())
            (new QQmlBoundSignalDeleter(s))->deleteLater();
        else
            delete s;
    }
    d->boundsignals.clear();
    d->target = obj;
    connectSignals();
    emit targetChanged();
}

/*!
    \qmlproperty bool QtQml::Connections::enabled
    \since 5.7

    This property holds whether the item accepts change events.

    By default, this property is \c true.
*/
bool QQmlConnections::isEnabled() const
{
    Q_D(const QQmlConnections);
    return d->enabled;
}

void QQmlConnections::setEnabled(bool enabled)
{
    Q_D(QQmlConnections);
    if (d->enabled == enabled)
        return;

    d->enabled = enabled;

    for (QQmlBoundSignal *s : std::as_const(d->boundsignals))
        s->setEnabled(d->enabled);

    emit enabledChanged();
}

/*!
    \qmlproperty bool QtQml::Connections::ignoreUnknownSignals

    Normally, a connection to a non-existent signal produces runtime errors.

    If this property is set to \c true, such errors are ignored.
    This is useful if you intend to connect to different types of objects, handling
    a different set of signals for each object.
*/
bool QQmlConnections::ignoreUnknownSignals() const
{
    Q_D(const QQmlConnections);
    return d->ignoreUnknownSignals;
}

void QQmlConnections::setIgnoreUnknownSignals(bool ignore)
{
    Q_D(QQmlConnections);
    d->ignoreUnknownSignals = ignore;
}

void QQmlConnectionsParser::verifyBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &props)
{
    for (int ii = 0; ii < props.size(); ++ii) {
        const QV4::CompiledData::Binding *binding = props.at(ii);
        const QString &propName = compilationUnit->stringAt(binding->propertyNameIndex);

        const bool thirdCharacterIsValid = (propName.size() >= 2)
                && (propName.at(2).isUpper() || propName.at(2) == u'_');
        if (!propName.startsWith(QLatin1String("on")) || !thirdCharacterIsValid) {
            error(props.at(ii), QQmlConnections::tr("Cannot assign to non-existent property \"%1\"").arg(propName));
            return;
        }

        if (binding->type() == QV4::CompiledData::Binding::Type_Script)
            continue;

        if (binding->type() >= QV4::CompiledData::Binding::Type_Object) {
            const QV4::CompiledData::Object *target = compilationUnit->objectAt(binding->value.objectIndex);
            if (!compilationUnit->stringAt(target->inheritedTypeNameIndex).isEmpty())
                error(binding, QQmlConnections::tr("Connections: nested objects not allowed"));
            else
                error(binding, QQmlConnections::tr("Connections: syntax error"));
            return;
        }

        error(binding, QQmlConnections::tr("Connections: script expected"));
        return;
    }
}

void QQmlConnectionsParser::applyBindings(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings)
{
    QQmlConnectionsPrivate *p =
        static_cast<QQmlConnectionsPrivate *>(QObjectPrivate::get(object));
    p->compilationUnit = compilationUnit;
    p->bindings = bindings;
}

void QQmlConnections::connectSignals()
{
    Q_D(QQmlConnections);
    if (!d->componentcomplete || (d->targetSet && !target()))
        return;

    if (d->bindings.isEmpty()) {
        connectSignalsToMethods();
    } else {
        if (lcQmlConnections().isWarningEnabled()) {
            qmlWarning(this) << tr("Implicitly defined onFoo properties in Connections are deprecated. "
                                    "Use this syntax instead: function onFoo(<arguments>) { ... }");
        }
        connectSignalsToBindings();
    }
}

void QQmlConnections::connectSignalsToMethods()
{
    Q_D(QQmlConnections);

    QObject *target = this->target();
    QQmlData *ddata = QQmlData::get(this);
    if (!ddata)
        return;

    QV4::ExecutionEngine *engine = ddata->context->engine()->handle();

    QQmlRefPointer<QQmlContextData> ctxtdata = ddata->outerContext;
    for (int i = ddata->propertyCache->methodOffset(),
             end = ddata->propertyCache->methodOffset() + ddata->propertyCache->methodCount();
         i < end;
         ++i) {

        const QQmlPropertyData *handler = ddata->propertyCache->method(i);
        if (!handler || !handler->isVMEFunction())
            continue;

        const QString propName = handler->name(this);

        QQmlProperty prop(target, propName);
        if (prop.isValid() && (prop.type() & QQmlProperty::SignalProperty)) {
            int signalIndex = QQmlPropertyPrivate::get(prop)->signalIndex();
            auto *signal = new QQmlBoundSignal(target, signalIndex, this, qmlEngine(this));
            signal->setEnabled(d->enabled);

            QV4::Scope scope(engine);
            QV4::ScopedContext global(scope, engine->rootContext());

            QQmlVMEMetaObject *vmeMetaObject = QQmlVMEMetaObject::get(this);
            Q_ASSERT(vmeMetaObject); // the fact we found the property above should guarentee this

            QV4::ScopedFunctionObject method(scope, vmeMetaObject->vmeMethod(handler->coreIndex()));

            QQmlBoundSignalExpression *expression =
                    ctxtdata ? new QQmlBoundSignalExpression(
                                       target, signalIndex, ctxtdata, this,
                                       method->as<QV4::FunctionObject>()->function())
                             : nullptr;

            signal->takeExpression(expression);
            d->boundsignals += signal;
        } else if (!d->ignoreUnknownSignals
                   && propName.startsWith(QLatin1String("on")) && propName.size() > 2
                   && propName.at(2).isUpper()) {
            qmlWarning(this) << tr("Detected function \"%1\" in Connections element. "
                                   "This is probably intended to be a signal handler but no "
                                   "signal of the target matches the name.").arg(propName);
        }
    }
}

// TODO: Drop this as soon as we can
void QQmlConnections::connectSignalsToBindings()
{
    Q_D(QQmlConnections);
    QObject *target = this->target();
    QQmlData *ddata = QQmlData::get(this);
    QQmlRefPointer<QQmlContextData> ctxtdata = ddata ? ddata->outerContext : nullptr;

    for (const QV4::CompiledData::Binding *binding : std::as_const(d->bindings)) {
        Q_ASSERT(binding->type() == QV4::CompiledData::Binding::Type_Script);
        QString propName = d->compilationUnit->stringAt(binding->propertyNameIndex);

        QQmlProperty prop(target, propName);
        if (prop.isValid() && (prop.type() & QQmlProperty::SignalProperty)) {
            int signalIndex = QQmlPropertyPrivate::get(prop)->signalIndex();
            QQmlBoundSignal *signal =
                new QQmlBoundSignal(target, signalIndex, this, qmlEngine(this));
            signal->setEnabled(d->enabled);

            auto f = d->compilationUnit->runtimeFunctions[binding->value.compiledScriptIndex];
            QQmlBoundSignalExpression *expression =
                    ctxtdata ? new QQmlBoundSignalExpression(target, signalIndex, ctxtdata, this, f)
                             : nullptr;
            signal->takeExpression(expression);
            d->boundsignals += signal;
        } else {
            if (!d->ignoreUnknownSignals)
                qmlWarning(this) << tr("Cannot assign to non-existent property \"%1\"").arg(propName);
        }
    }
}

void QQmlConnections::classBegin()
{
    Q_D(QQmlConnections);
    d->componentcomplete=false;
}

void QQmlConnections::componentComplete()
{
    Q_D(QQmlConnections);
    d->componentcomplete=true;
    connectSignals();
}

QT_END_NAMESPACE

#include "moc_qqmlconnections_p.cpp"
