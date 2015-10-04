/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype StackView
    \inherits Control
    \instantiates QQuickStackView
    \inqmlmodule Qt.labs.controls
    \ingroup navigation
    \brief Provides a stack-based navigation model.

    StackView can be used with a set of inter-linked information pages. For
    example, an email application with separate views to list latest emails,
    view a specific email, and list/view the attachments. The email list view
    is pushed onto the stack as users open an email, and popped out as they
    choose to go back.

    The following snippet demonstrates a simple use case, where the \c mainView
    is pushed onto and popped out of the stack on relevant button click:

    \qml
    ApplicationWindow {
        title: qsTr("Hello World")
        width: 640
        height: 480
        visible: true

        StackView {
            id: stack
            initialItem: mainView
            anchors.fill: parent
        }

        Component {
            id: mainView

            Row {
                spacing: 10

                Button {
                    text: "Push"
                    onClicked: stack.push(mainView)
                }
                Button {
                    text: "Pop"
                    enabled: stack.depth > 1
                    onClicked: stack.pop()

                }
                Text {
                    text: stack.depth
                }
            }
        }
    }
    \endqml

    \section1 Using StackView in an Application

    Using StackView in an application is as simple as adding it as a child to
    a Window. The stack is usually anchored to the edges of the window, except
    at the top or bottom where it might be anchored to a status bar, or some
    other similar UI component. The stack can then be  used by invoking its
    navigation methods. The first item to show in the StackView is the one
    that was assigned to \l initialItem, or the topmost item if \l initialItem
    is not set.

    \section1 Basic Navigation

    StackView supports three primary navigation operations: push(), pop(), and
    replace(). These correspond to classic stack operations where "push" adds
    an item to the top of a stack, "pop" removes the top item from the
    stack, and "replace" is like a pop followed by a push, which replaces the
    topmost item with the new item. The topmost item in the stack
    corresponds to the one that is \l{StackView::currentItem}{currently}
    visible on screen. Logically, "push" navigates forward or deeper into the
    application UI, "pop" navigates backward, and "replace" replaces the
    \l currentItem.

    Sometimes, it is necessary to go back more than a single step in the stack.
    For example, to return to a "main" item or some kind of section item in the
    application. In such cases, it is possible to specify an item as a
    parameter for pop(). This is called an "unwind" operation, where the stack
    unwinds till the specified item. If the item is not found, stack unwinds
    until it is left with one item, which becomes the \l currentItem. To
    explicitly unwind to the bottom of the stack, it is recommended to use
    \l{pop()}{pop(null)}, although any non-existent item will do.

    Given the stack [A, B, C]:

    \list
    \li \l{push()}{push(D)} => [A, B, C, D] - "push" transition animation
        between C and D
    \li pop() => [A, B] - "pop" transition animation between C and B
    \li \l{replace()}{replace(D)} => [A, B, D] - "replace" transition between
        C and D
    \li \l{pop()}{pop(A)} => [A] - "pop" transition between C and A
    \endlist

    \note When the stack is empty, a push() operation will not have a
    transition animation because there is nothing to transition from (typically
    on application start-up). A pop() operation on a stack with depth 1 or
    0 does nothing. In such cases, the stack can be emptied using the clear()
    method.

    \section1 Deep Linking

    \e{Deep linking} means launching an application into a particular state. For
    example, a newspaper application could be launched into showing a
    particular article, bypassing the topmost item. In terms of StackView, deep linking means the ability to modify
    the state of the stack, so much so that it is possible to push a set of
    items to the top of the stack, or to completely reset the stack to a given
    state.

    The API for deep linking in StackView is the same as for basic navigation.
    Pushing an array instead of a single item adds all the items in that array
    to the stack. The transition animation, however, is applied only for the
    last item in the array. The normal semantics of push() apply for deep
    linking, that is, it adds whatever is pushed onto the stack.

    \note Only the last item of the array is loaded. The rest of the items are
    loaded only when needed, either on subsequent calls to pop or on request to
    get an item using get().

    This gives us the following result, given the stack [A, B, C]:

    \list
    \li \l{push()}{push([D, E, F])} => [A, B, C, D, E, F] - "push" transition
        animation between C and F
    \li \l{replace()}{replace([D, E, F])} => [A, B, D, E, F] - "replace"
        transition animation between C and F
    \li \l{clear()} followed by \l{push()}{push([D, E, F])} => [D, E, F] - no
        transition animation for pushing items as the stack was empty.
    \endlist

    \section1 Finding Items

    An Item for which the application does not have a reference can be found
    by calling find(). The method needs a callback function, which is invoked
    for each item in the stack (starting at the top) until a match is found.
    If the callback returns \c true, find() stops and returns the matching
    item, otherwise \c null is returned.

    The code below searches the stack for an item named "order_id" and unwinds
    to that item.

    \badcode
    stackView.pop(stackView.find(function(item) {
        return item.name == "order_id";
    }));
    \endcode

    You can also get to an item in the stack using \l {get()}{get(index)}.

    \badcode
    previousItem = stackView.get(myItem.Stack.index - 1));
    \endcode

    \section1 Transitions

    For each push or pop operation, different transition animations are applied
    to entering and exiting items. These animations define how the entering item
    should animate in, and the exiting item should animate out. The animations
    can be customized by assigning different \l{Transition}s for the
    \l pushEnter, \l pushExit, \l popEnter, and \l popExit properties of
    StackView.

    \note The pop and push transition animations affect each others'
    transitional behavior. So customizing the animation for one and leaving
    the other may give unexpected results.

    The following snippet defines a simple fade transition for push and pop
    operations:

    \qml
    StackView {
        id: stackview
        anchors.fill: parent

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
        popEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
    }
    \endqml

*/

QQuickStackView::QQuickStackView(QQuickItem *parent) :
    QQuickControl(*(new QQuickStackViewPrivate), parent)
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
    qDeleteAll(d->removals);
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
    \qmlproperty bool Qt.labs.controls::StackView::busy
    \readonly
    This property holds whether a transition is running.
*/
bool QQuickStackView::busy() const
{
    Q_D(const QQuickStackView);
    return d->transitioner && !d->transitioner->runningJobs.isEmpty();
}

/*!
    \qmlproperty int Qt.labs.controls::StackView::depth
    \readonly
    This property holds the number of items currently pushed onto the stack.
*/
int QQuickStackView::depth() const
{
    Q_D(const QQuickStackView);
    return d->elements.count();
}

/*!
    \qmlproperty Item Qt.labs.controls::StackView::currentItem
    \readonly
    This property holds the current top-most item in the stack.
*/
QQuickItem *QQuickStackView::currentItem() const
{
    Q_D(const QQuickStackView);
    return d->currentItem;
}

/*!
    \qmlmethod Item Qt.labs.controls::StackView::get(index, behavior = DontLoad)

    Supported behavior values:
    \value StackView.DontLoad
    \value StackView.ForceLoad

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
    \qmlmethod Item Qt.labs.controls::StackView::find(callback, behavior = DontLoad)

    Supported behavior values:
    \value StackView.DontLoad
    \value StackView.ForceLoad

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
    \qmlmethod Item Qt.labs.controls::StackView::push(item, properties, operation)

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
    \qmlmethod Item Qt.labs.controls::StackView::pop(item = null, operation = Transition)

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
    \qmlmethod Item Qt.labs.controls::StackView::replace(item, properties, operation = Transition)

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
    \qmlmethod Item Qt.labs.controls::StackView::clear()

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
    \qmlproperty var Qt.labs.controls::StackView::initialItem

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
    \qmlproperty Transition Qt.labs.controls::StackView::popEnter

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
    \qmlproperty Transition Qt.labs.controls::StackView::popExit

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
    \qmlproperty Transition Qt.labs.controls::StackView::pushEnter

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
    \qmlproperty Transition Qt.labs.controls::StackView::pushExit

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
    QQuickControl::componentComplete();

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
    QQuickControl::geometryChanged(newGeometry, oldGeometry);

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

bool QQuickStackView::childMouseEventFilter(QQuickItem *, QEvent *)
{
    // busy should be true if this function gets called
    return true;
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
    QQuickStackView *oldView = element ? element->view : Q_NULLPTR;
    QQuickStackView::Status oldStatus = element ? element->status : QQuickStackView::Inactive;

    element = Q_NULLPTR;

    if (oldIndex != -1)
        emit q->indexChanged();
    if (oldView)
        emit q->viewChanged();
    if (oldStatus != QQuickStackView::Inactive)
        emit q->statusChanged();
}

QQuickStackAttached::QQuickStackAttached(QQuickItem *parent) :
    QObject(*(new QQuickStackAttachedPrivate), parent)
{
}

/*!
    \qmlattachedproperty int Qt.labs.controls::StackView::index

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
    \qmlattachedproperty StackView Qt.labs.controls::StackView::view

    TODO
*/
QQuickStackView *QQuickStackAttached::view() const
{
    Q_D(const QQuickStackAttached);
    if (!d->initialized)
        const_cast<QQuickStackAttachedPrivate *>(d)->init();
    return d->element ? d->element->view : Q_NULLPTR;
}

/*!
    \qmlattachedproperty enumeration Qt.labs.controls::StackView::status

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
