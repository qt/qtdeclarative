/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickstackview_p.h"
#include "qquickstackview_p_p.h"

#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype StackView
    \inherits Container
    \instantiates QQuickStackView
    \inqmlmodule QtQuick.Controls
    \ingroup navigation
    \brief A stack view control.

    TODO
*/

QQuickStackView::QQuickStackView(QQuickItem *parent) :
    QQuickContainer(*(new QQuickStackViewPrivate), parent)
{
    setFlag(ItemIsFocusScope);
}

QQuickStackView::~QQuickStackView()
{
    Q_D(QQuickStackView);
    if (d->transitioner) {
        d->transitioner->setChangeListener(Q_NULLPTR);
        delete d->transitioner;
    }
    qDeleteAll(d->elements);
}

QQuickStackAttached *QQuickStackView::qmlAttachedProperties(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        qmlInfo(object) << "StackView must be attached to an Item";
        return Q_NULLPTR;
    }
    return new QQuickStackAttached(item);
}

/*!
    \qmlproperty bool QtQuickControls2::StackView::busy
    \readonly
    This property holds whether a transition is running.
*/
bool QQuickStackView::busy() const
{
    Q_D(const QQuickStackView);
    return d->transitioner && !d->transitioner->runningJobs.isEmpty();
}

/*!
    \qmlproperty int QtQuickControls2::StackView::depth
    \readonly
    This property holds the number of items currently pushed onto the stack.
*/
int QQuickStackView::depth() const
{
    Q_D(const QQuickStackView);
    return d->elements.count();
}

/*!
    \qmlproperty Item QtQuickControls2::StackView::currentItem
    \readonly
    This property holds the current top-most item in the stack.
*/
QQuickItem *QQuickStackView::currentItem() const
{
    Q_D(const QQuickStackView);
    return d->currentItem;
}

/*!
    \qmlmethod Item QtQuickControls2::StackView::get(index, behavior = DontLoad)

    Supported behavior values:
    \li StackView.DontLoad
    \li StackView.ForceLoad

    TODO
*/
QQuickItem *QQuickStackView::get(int index, LoadBehavior behavior)
{
    Q_D(QQuickStackView);
    QQuickStackElement *element = d->elements.value(index);
    if (element) {
        if (behavior == ForceLoad)
            element->load(this);
        return element->item;
    }
    return Q_NULLPTR;
}

/*!
    \qmlmethod Item QtQuickControls2::StackView::find(callback, behavior = DontLoad)

    Supported behavior values:
    \li StackView.DontLoad
    \li StackView.ForceLoad

    TODO
*/
QQuickItem *QQuickStackView::find(const QJSValue &callback, LoadBehavior behavior)
{
    Q_D(QQuickStackView);
    QJSValue func(callback);
    QQmlEngine *engine = qmlEngine(this);
    if (!engine || !func.isCallable()) // TODO: warning?
        return Q_NULLPTR;

    for (int i = d->elements.count() - 1; i >= 0; --i) {
        QQuickStackElement *element = d->elements.at(i);
        if (behavior == ForceLoad)
            element->load(this);
        if (element->item) {
            QJSValue rv = func.call(QJSValueList() << engine->newQObject(element->item) << i);
            if (rv.toBool())
                return element->item;
        }
    }

    return Q_NULLPTR;
}

/*!
    \qmlmethod Item QtQuickControls2::StackView::push(item, properties, operation)

    TODO
*/
void QQuickStackView::push(QQmlV4Function *args)
{
    Q_D(QQuickStackView);
    if (args->length() <= 0) {
        qmlInfo(this) << "push: missing arguments";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    Operation operation = d->elements.isEmpty() ? Immediate : Transition;
    QV4::ScopedValue lastArg(scope, (*args)[args->length() - 1]);
    if (lastArg->isInt32())
        operation = static_cast<Operation>(lastArg->toInt32());

    QList<QQuickStackElement *> elements = d->parseElements(args);
    if (elements.isEmpty()) {
        qmlInfo(this) << "push: nothing to push";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QQuickStackElement *exit = Q_NULLPTR;
    if (!d->elements.isEmpty())
        exit = d->elements.top();

    if (d->pushElements(elements)) {
        emit depthChanged();
        QQuickStackElement *enter = d->elements.top();
        d->pushTransition(enter, exit, boundingRect(), operation == Immediate);
        d->setCurrentItem(enter->item);
    }

    if (d->currentItem) {
        QV4::ScopedValue rv(scope, QV4::QObjectWrapper::wrap(v4, d->currentItem));
        args->setReturnValue(rv->asReturnedValue());
    } else {
        args->setReturnValue(QV4::Encode::null());
    }
}

/*!
    \qmlmethod Item QtQuickControls2::StackView::pop(item = null, operation = Transition)

    TODO
*/
void QQuickStackView::pop(QQmlV4Function *args)
{
    Q_D(QQuickStackView);
    int argc = args->length();
    if (d->elements.count() <= 1 || argc > 2) {
        if (argc > 2)
            qmlInfo(this) << "pop: too many arguments";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QQuickStackElement *exit = d->elements.pop();
    QQuickStackElement *enter = d->elements.top();

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    if (argc > 0) {
        QV4::ScopedValue value(scope, (*args)[0]);
        if (value->isNull()) {
            enter = d->elements.value(0);
        } else if (!value->isUndefined() && !value->isInt32()) {
            enter = d->findElement(value);
            if (!enter) {
                qmlInfo(this) << "pop: unknown argument: " << value->toQString(); // TODO: safe?
                args->setReturnValue(QV4::Encode::null());
                d->elements.push(exit); // restore
                return;
            }
        }
    }

    Operation operation = Transition;
    if (argc > 0) {
        QV4::ScopedValue lastArg(scope, (*args)[argc - 1]);
        if (lastArg->isInt32())
            operation = static_cast<Operation>(lastArg->toInt32());
    }

    QQuickItem *previousItem = Q_NULLPTR;

    if (d->popElements(enter)) {
        if (exit)
            previousItem = exit->item;
        emit depthChanged();
        d->popTransition(enter, exit, boundingRect(), operation == Immediate);
        d->setCurrentItem(enter->item);
    }

    if (previousItem) {
        QV4::ScopedValue rv(scope, QV4::QObjectWrapper::wrap(v4, previousItem));
        args->setReturnValue(rv->asReturnedValue());
    } else {
        args->setReturnValue(QV4::Encode::null());
    }
}

/*!
    \qmlmethod Item QtQuickControls2::StackView::push(item, properties, operation = Transition)

    TODO
*/
void QQuickStackView::replace(QQmlV4Function *args)
{
    Q_D(QQuickStackView);
    if (args->length() <= 0) {
        qmlInfo(this) << "replace: missing arguments";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    Operation operation = d->elements.isEmpty() ? Immediate : Transition;
    QV4::ScopedValue lastArg(scope, (*args)[args->length() - 1]);
    if (lastArg->isInt32())
        operation = static_cast<Operation>(lastArg->toInt32());

    QQuickStackElement *target = Q_NULLPTR;
    QV4::ScopedValue firstArg(scope, (*args)[0]);
    if (firstArg->isNull())
        target = d->elements.value(0);
    else if (!firstArg->isInt32())
        target = d->findElement(firstArg);

    QList<QQuickStackElement *> elements = d->parseElements(args, target ? 1 : 0);
    if (elements.isEmpty()) {
        qmlInfo(this) << "replace: nothing to push";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    int depth = d->elements.count();
    QQuickStackElement* exit = Q_NULLPTR;
    if (!d->elements.isEmpty())
        exit = d->elements.pop();

    if (d->replaceElements(target, elements)) {
        if (depth != d->elements.count())
            emit depthChanged();
        QQuickStackElement *enter = d->elements.top();
        d->replaceTransition(enter, exit, boundingRect(), operation == Immediate);
        d->setCurrentItem(enter->item);
    }

    if (d->currentItem) {
        QV4::ScopedValue rv(scope, QV4::QObjectWrapper::wrap(v4, d->currentItem));
        args->setReturnValue(rv->asReturnedValue());
    } else {
        args->setReturnValue(QV4::Encode::null());
    }
}

/*!
    \qmlmethod Item QtQuickControls2::StackView::clear()

    TODO
*/
void QQuickStackView::clear()
{
    Q_D(QQuickStackView);
    d->setCurrentItem(Q_NULLPTR);
    qDeleteAll(d->elements);
    d->elements.clear();
    emit depthChanged();
}

/*!
    \qmlproperty var QtQuickControls2::StackView::initialItem

    This property holds the initial item.

    \sa push()
*/
QVariant QQuickStackView::initialItem() const
{
    Q_D(const QQuickStackView);
    return d->initialItem;
}

void QQuickStackView::setInitialItem(const QVariant &item)
{
    Q_D(QQuickStackView);
    d->initialItem = item;
}

/*!
    \qmlproperty Transition QtQuickControls2::StackView::popEnter

    TODO
*/
QQuickTransition *QQuickStackView::popEnter() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->removeDisplacedTransition;
    return Q_NULLPTR;
}

void QQuickStackView::setPopEnter(QQuickTransition *enter)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->removeDisplacedTransition != enter) {
        d->transitioner->removeDisplacedTransition = enter;
        emit popEnterChanged();
    }
}

/*!
    \qmlproperty Transition QtQuickControls2::StackView::popExit

    TODO
*/
QQuickTransition *QQuickStackView::popExit() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->removeTransition;
    return Q_NULLPTR;
}

void QQuickStackView::setPopExit(QQuickTransition *exit)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->removeTransition != exit) {
        d->transitioner->removeTransition = exit;
        emit popExitChanged();
    }
}

/*!
    \qmlproperty Transition QtQuickControls2::StackView::pushEnter

    TODO
*/
QQuickTransition *QQuickStackView::pushEnter() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->addTransition;
    return Q_NULLPTR;
}

void QQuickStackView::setPushEnter(QQuickTransition *enter)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->addTransition != enter) {
        d->transitioner->addTransition = enter;
        emit pushEnterChanged();
    }
}

/*!
    \qmlproperty Transition QtQuickControls2::StackView::pushExit

    TODO
*/
QQuickTransition *QQuickStackView::pushExit() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->addDisplacedTransition;
    return Q_NULLPTR;
}

void QQuickStackView::setPushExit(QQuickTransition *exit)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->addDisplacedTransition != exit) {
        d->transitioner->addDisplacedTransition = exit;
        emit pushExitChanged();
    }
}

void QQuickStackView::componentComplete()
{
    QQuickContainer::componentComplete();

    Q_D(QQuickStackView);
    QQuickStackElement *element = Q_NULLPTR;
    if (QObject *o = d->initialItem.value<QObject *>())
        element = QQuickStackElement::fromObject(o, this);
    else if (d->initialItem.canConvert<QString>())
        element = QQuickStackElement::fromString(d->initialItem.toString(), this);
    if (d->pushElement(element)) {
        emit depthChanged();
        d->setCurrentItem(element->item);
    }
}

void QQuickStackView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);

    Q_D(QQuickStackView);
    foreach (QQuickStackElement *element, d->elements) {
        if (element->item) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(element->item);
            if (!p->widthValid) {
                element->item->setWidth(newGeometry.width());
                p->widthValid = false;
            }
            if (!p->heightValid) {
                element->item->setHeight(newGeometry.height());
                p->heightValid = false;
            }
        }
    }
}

void QQuickStackAttachedPrivate::init()
{
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item) {
        QQuickStackView *view = qobject_cast<QQuickStackView *>(item->parentItem());
        if (view) {
            element = QQuickStackViewPrivate::get(view)->findElement(item);
            if (element)
                initialized = true;
        }
    }
}

void QQuickStackAttachedPrivate::reset()
{
    Q_Q(QQuickStackAttached);
    int oldIndex = element ? element->index : -1;
    QQuickStackView::Status oldStatus = element ? element->status : QQuickStackView::Inactive;

    element = Q_NULLPTR;

    if (oldIndex != -1)
        emit q->indexChanged();
    if (oldStatus != QQuickStackView::Inactive)
        emit q->statusChanged();
}

QQuickStackAttached::QQuickStackAttached(QQuickItem *parent) :
    QObject(*(new QQuickStackAttachedPrivate), parent)
{
}

/*!
    \qmlattachedproperty int QtQuickControls2::StackView::index

    TODO
*/
int QQuickStackAttached::index() const
{
    Q_D(const QQuickStackAttached);
    if (!d->initialized)
        const_cast<QQuickStackAttachedPrivate *>(d)->init();
    return d->element ? d->element->index : -1;
}

/*!
    \qmlattachedproperty enumeration QtQuickControls2::StackView::status

    TODO
*/
QQuickStackView::Status QQuickStackAttached::status() const
{
    Q_D(const QQuickStackAttached);
    if (!d->initialized)
        const_cast<QQuickStackAttachedPrivate *>(d)->init();
    return d->element ? d->element->status : QQuickStackView::Inactive;
}

QT_END_NAMESPACE
