// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstackview_p.h"
#include "qquickstackview_p_p.h"
#include "qquickstackelement_p_p.h"
#if QT_CONFIG(quick_viewtransitions)
#include "qquickstacktransition_p_p.h"
#endif

#include <QtCore/qscopedvaluerollback.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcomponent.h>

#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QQuickStackViewArg::QQuickStackViewArg(QQuickItem *item)
    : mItem(item)
{
}

QQuickStackViewArg::QQuickStackViewArg(const QUrl &url)
    : mUrl(url)
{
}

QQuickStackViewArg::QQuickStackViewArg(QQmlComponent *component)
    : mComponent(component)
{
}

QQuickStackViewArg::QQuickStackViewArg(const QVariantMap &properties)
    : mProperties(properties)
{
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QQuickStackViewArg &arg)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QQuickStackViewArg("
        << "mItem=" << arg.mItem
        << " mComponent=" << arg.mComponent
        << " mUrl=" << arg.mUrl
        << ")";
    return debug;
}
#endif

/*!
    \qmltype StackView
    \inherits Control
//!     \nativetype QQuickStackView
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-navigation
    \ingroup qtquickcontrols-containers
    \ingroup qtquickcontrols-focusscopes
    \brief Provides a stack-based navigation model.

    \image qtquickcontrols-stackview-wireframe.webp

    StackView can be used with a set of inter-linked information pages. For
    example, an email application with separate views to list the latest emails,
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
    other similar UI component. The stack can then be used by invoking its
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

    \section2 Pushing Items

    In the following animation, three \l Label controls are pushed onto a
    stack view with the \l push() function:

    \image qtquickcontrols-stackview-push.gif

    The stack now contains the following items: \c [A, B, C].

    \note When the stack is empty, a push() operation will not have a
    transition animation because there is nothing to transition from (typically
    on application start-up).

    \section2 Popping Items

    Continuing on from the example above, the topmost item on the stack is
    removed with a call to \l pop():

    \image qtquickcontrols-stackview-pop.gif

    The stack now contains the following items: \c [A, B].

    \note A pop() operation on a stack with depth 1 or 0 does nothing. In such
    cases, the stack can be emptied using the \l clear() method.

    \section3 Unwinding Items via Pop

    Sometimes, it is necessary to go back more than a single step in the stack.
    For example, to return to a "main" item or some kind of section item in the
    application. In such cases, it is possible to specify an item as a
    parameter for pop(). This is called an "unwind" operation, where the stack
    unwinds till the specified item. If the item is not found, stack unwinds
    until it is left with one item, which becomes the \l currentItem. To
    explicitly unwind to the bottom of the stack, it is recommended to use
    \l{pop()}{pop(null)}, although any non-existent item will do.

    In the following animation, we unwind the stack to the first item by
    calling \c pop(null):

    \image qtquickcontrols-stackview-unwind.gif

    The stack now contains a single item: \c [A].

    \section2 Replacing Items

    In the following animation, we \l replace the topmost item with \c D:

    \image qtquickcontrols-stackview-replace.gif

    The stack now contains the following items: \c [A, B, D].

    \section1 Deep Linking

    \e{Deep linking} means launching an application into a particular state. For
    example, a newspaper application could be launched into showing a
    particular article, bypassing the topmost item. In terms of StackView, deep
    linking means the ability to modify the state of the stack, so much so that
    it is possible to push a set of items to the top of the stack, or to
    completely reset the stack to a given state.

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
    previousItem = stackView.get(myItem.StackView.index - 1));
    \endcode

    \section1 Transitions

    For each push or pop operation, different transition animations are applied
    to entering and exiting items. These animations define how the entering item
    should animate in, and the exiting item should animate out. The animations
    can be customized by assigning different \l [QML] {Transition} {Transitions}
    for the \l pushEnter, \l pushExit, \l popEnter, \l popExit, replaceEnter,
    and \l replaceExit properties of StackView.

    \note The transition animations affect each others' transitional behavior.
    Customizing the animation for one and leaving the other may give unexpected
    results.

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

    \note Using anchors on the items added to a StackView is not supported.
          Typically push, pop, and replace transitions animate the position,
          which is not possible when anchors are applied. Notice that this
          only applies to the root of the item. Using anchors for its children
          works as expected.

    \section1 Item Ownership

    StackView only takes ownership of items that it creates itself. This means
    that any item pushed onto a StackView will never be destroyed by the
    StackView; only items that StackView creates from \l {Component}{Components}
    or \l [QML] {url}{URLs} are destroyed by the StackView. To illustrate this,
    the messages in the example below will only be printed when the StackView
    is destroyed, not when the items are popped off the stack:

    \qml
    Component {
        id: itemComponent

        Item {
            Component.onDestruction: print("Destroying second item")
        }
    }

    StackView {
        initialItem: Item {
            Component.onDestruction: print("Destroying initial item")
        }

        Component.onCompleted: push(itemComponent.createObject(window))
    }
    \endqml

    However, both of the items created from the URL and Component in the
    following example will be destroyed by the StackView when they are popped
    off of it:

    \qml
    Component {
        id: itemComponent

        Item {
            Component.onDestruction: print("Destroying second item")
        }
    }

    StackView {
        initialItem: "Item1.qml"

        Component.onCompleted: push(itemComponent)
    }
    \endqml

    \section1 Size

    StackView does not inherit an implicit size from items that are pushed onto
    it. This means that using it as the \l {Popup::}{contentItem} of a
    \l Dialog, for example, will not work as expected:

    \code
    Dialog {
        StackView {
            initialItem: Rectangle {
                width: 200
                height: 200
                color: "salmon"
            }
        }
    }
    \endcode

    There are several ways to ensure that StackView has a size in this
    situation:

    \list
        \li Set \l[QtQuick]{Item::}{implicitWidth} and
            \l[QtQuick]{Item::}{implicitHeight} on the StackView itself.
        \li Set \l[QtQuick]{Item::}{implicitWidth} and
            \l[QtQuick]{Item::}{implicitHeight} on the \l Rectangle.
        \li Set \l {Popup::}{contentWidth} and \l {Popup::}{contentHeight} on
            the Dialog.
        \li Give the Dialog a size.
    \endlist

    \sa {Customizing StackView}, {Navigating with StackView}, {Navigation Controls},
        {Container Controls}, {Focus Management in Qt Quick Controls}
*/

QQuickStackView::QQuickStackView(QQuickItem *parent)
    : QQuickControl(*(new QQuickStackViewPrivate), parent)
{
    setFlag(ItemIsFocusScope);

    Q_D(QQuickStackView);
    d->setSizePolicy(QLayoutPolicy::Preferred, QLayoutPolicy::Preferred);
}

QQuickStackView::~QQuickStackView()
{
    Q_D(QQuickStackView);
#if QT_CONFIG(quick_viewtransitions)
    if (d->transitioner) {
        d->transitioner->setChangeListener(nullptr);
        delete d->transitioner;
    }
#endif
    qDeleteAll(d->removing);
    qDeleteAll(d->removed);
    qDeleteAll(d->elements);
}

QQuickStackViewAttached *QQuickStackView::qmlAttachedProperties(QObject *object)
{
    return new QQuickStackViewAttached(object);
}

/*!
    \qmlproperty bool QtQuick.Controls::StackView::busy
    \readonly
    This property holds whether a transition is running.
*/
bool QQuickStackView::isBusy() const
{
    Q_D(const QQuickStackView);
    return d->busy;
}

/*!
    \qmlproperty int QtQuick.Controls::StackView::depth
    \readonly
    This property holds the number of items currently pushed onto the stack.
*/
int QQuickStackView::depth() const
{
    Q_D(const QQuickStackView);
    return d->elements.size();
}

/*!
    \qmlproperty Item QtQuick.Controls::StackView::currentItem
    \readonly
    This property holds the current top-most item in the stack.
*/
QQuickItem *QQuickStackView::currentItem() const
{
    Q_D(const QQuickStackView);
    return d->currentItem;
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::get(index, behavior)

    Returns the item at position \a index in the stack, or \c null if the index
    is out of bounds.

    Supported \a behavior values:
    \value StackView.DontLoad The item is not forced to load (and \c null is returned if not yet loaded).
    \value StackView.ForceLoad The item is forced to load.
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
    return nullptr;
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::find(callback, behavior)

    Search for a specific item inside the stack. The \a callback function is called
    for each item in the stack (with the item and index as arguments) until the callback
    function returns \c true. The return value is the item found. For example:

    \code
    stackView.find(function(item, index) {
        return item.isTheOne
    })
    \endcode

    Supported \a behavior values:
    \value StackView.DontLoad Unloaded items are skipped (the callback function is not called for them).
    \value StackView.ForceLoad Unloaded items are forced to load.
*/
QQuickItem *QQuickStackView::find(const QJSValue &callback, LoadBehavior behavior)
{
    Q_D(QQuickStackView);
    QJSValue func(callback);
    QQmlEngine *engine = qmlEngine(this);
    if (!engine || !func.isCallable()) // TODO: warning?
        return nullptr;

    for (int i = d->elements.size() - 1; i >= 0; --i) {
        QQuickStackElement *element = d->elements.at(i);
        if (behavior == ForceLoad)
            element->load(this);
        if (element->item) {
            QJSValue rv = func.call(QJSValueList() << engine->newQObject(element->item) << i);
            if (rv.toBool())
                return element->item;
        }
    }

    return nullptr;
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::push(item, properties, operation)

    Pushes an \a item onto the stack using an optional \a operation, and
    optionally applies a set of \a properties on the item. The item can be
    an \l Item, \l Component, or a \l [QML] url. Returns the item that became
    current.

    StackView creates an instance automatically if the pushed item is a \l Component,
    or a \l [QML] url, and the instance will be destroyed when it is popped
    off the stack. See \l {Item Ownership} for more information.

    The optional \a properties argument specifies a map of initial
    property values for the pushed item. For dynamically created items, these values
    are applied before the creation is finalized. This is more efficient than setting
    property values after creation, particularly where large sets of property values
    are defined, and also allows property bindings to be set up (using \l{Qt::binding}
    {Qt.binding()}) before the item is created.

    Pushing a single item:
    \code
    stackView.push(rect)

    // or with properties:
    stackView.push(rect, {"color": "red"})
    \endcode

    Multiple items can be pushed at the same time either by passing them as
    additional arguments, or as an array. The last item becomes the current
    item. Each item can be followed by a set of properties to apply.

    Passing a variable amount of arguments:
    \code
    stackView.push(rect1, rect2, rect3)

    // or with properties:
    stackView.push(rect1, {"color": "red"}, rect2, {"color": "green"}, rect3, {"color": "blue"})
    \endcode

    Pushing an array of items:
    \code
    stackView.push([rect1, rect2, rect3])

    // or with properties:
    stackView.push([rect1, {"color": "red"}, rect2, {"color": "green"}, rect3, {"color": "blue"}])
    \endcode

    An \a operation can be optionally specified as the last argument. Supported
    operations:

    \value StackView.Immediate An immediate operation without transitions.
    \value StackView.PushTransition An operation with push transitions (since QtQuick.Controls 2.1).
    \value StackView.ReplaceTransition An operation with replace transitions (since QtQuick.Controls 2.1).
    \value StackView.PopTransition An operation with pop transitions (since QtQuick.Controls 2.1).

    If no operation is provided, \c Immediate will be used if the stack is
    empty, and \c PushTransition otherwise.

    \note Items that already exist in the stack are not pushed.

    \note If you are \l {The QML script compiler}{compiling QML}, use the
    strongly-typed \l pushItem or \l pushItems functions instead.

    \sa initialItem, {Pushing Items}
*/
void QQuickStackView::push(QQmlV4FunctionPtr args)
{
    Q_D(QQuickStackView);
    const QString operationName = QStringLiteral("push");
    if (d->modifyingElements) {
        d->warnOfInterruption(operationName);
        return;
    }

    QScopedValueRollback<bool> modifyingElements(d->modifyingElements, true);
    QScopedValueRollback<QString> operationNameRollback(d->operation, operationName);
    if (args->length() <= 0) {
        d->warn(QStringLiteral("missing arguments"));
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

#if QT_CONFIG(quick_viewtransitions)
    Operation operation = d->elements.isEmpty() ? Immediate : PushTransition;
    QV4::ScopedValue lastArg(scope, (*args)[args->length() - 1]);
    if (lastArg->isInt32())
        operation = static_cast<Operation>(lastArg->toInt32());
#endif

    QStringList errors;
    QList<QQuickStackElement *> elements = d->parseElements(0, args, &errors);
    // Remove any items that are already in the stack, as they can't be in two places at once.
    // not using erase_if, as we have to delete the elements first
    auto removeIt = std::remove_if(elements.begin(), elements.end(), [&](QQuickStackElement *element) {
        return element->item && d->findElement(element->item);
    });
    for (auto it = removeIt, end = elements.end(); it != end; ++it)
        delete *it;
    elements.erase(removeIt, elements.end());

    if (!errors.isEmpty() || elements.isEmpty()) {
        if (!errors.isEmpty()) {
            for (const QString &error : std::as_const(errors))
                d->warn(error);
        } else {
            d->warn(QStringLiteral("nothing to push"));
        }
        args->setReturnValue(QV4::Encode::null());
        return;
    }

#if QT_CONFIG(quick_viewtransitions)
    QQuickStackElement *exit = nullptr;
    if (!d->elements.isEmpty())
        exit = d->elements.top();
#endif

    int oldDepth = d->elements.size();
    if (d->pushElements(elements)) {
        d->depthChange(d->elements.size(), oldDepth);
        QQuickStackElement *enter = d->elements.top();
#if QT_CONFIG(quick_viewtransitions)
        d->startTransition(QQuickStackTransition::pushEnter(operation, enter, this),
                           QQuickStackTransition::pushExit(operation, exit, this),
                           operation == Immediate);
#endif
        d->setCurrentItem(enter);
    }

    if (d->currentItem) {
        QV4::ScopedValue rv(scope, QV4::QObjectWrapper::wrap(v4, d->currentItem));
        args->setReturnValue(rv->asReturnedValue());
    } else {
        args->setReturnValue(QV4::Encode::null());
    }
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::pop(item, operation)

    Pops one or more items off the stack. Returns the last item removed from the stack.

    If the \a item argument is specified, all items down to (but not
    including) \a item will be popped. If \a item is \c null, all
    items down to (but not including) the first item is popped.
    If not specified, only the current item is popped.

    \note A pop() operation on a stack with depth 1 or 0 does nothing. In such
    cases, the stack can be emptied using the \l clear() method.

    \include qquickstackview.qdocinc pop-ownership

    An \a operation can be optionally specified as the last argument. Supported
    operations:

    \value StackView.Immediate An immediate operation without transitions.
    \value StackView.PushTransition An operation with push transitions (since QtQuick.Controls 2.1).
    \value StackView.ReplaceTransition An operation with replace transitions (since QtQuick.Controls 2.1).
    \value StackView.PopTransition An operation with pop transitions (since QtQuick.Controls 2.1).

    If no operation is provided, \c PopTransition will be used.

    Examples:
    \code
    stackView.pop()
    stackView.pop(someItem, StackView.Immediate)
    stackView.pop(StackView.Immediate)
    stackView.pop(null)
    \endcode

    \note If you are \l {The QML script compiler}{compiling QML}, use the
    strongly-typed \l popToItem, \l popToIndex or \l popCurrentItem functions
    instead.

    \sa clear(), {Popping Items}, {Unwinding Items via Pop}
*/
void QQuickStackView::pop(QQmlV4FunctionPtr args)
{
    Q_D(QQuickStackView);
    const QString operationName = QStringLiteral("pop");
    if (d->modifyingElements) {
        d->warnOfInterruption(operationName);
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QScopedValueRollback<bool> modifyingElements(d->modifyingElements, true);
    QScopedValueRollback<QString> operationNameRollback(d->operation, operationName);
    int argc = args->length();
    if (d->elements.size() <= 1 || argc > 2) {
        if (argc > 2)
            d->warn(QStringLiteral("too many arguments"));
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    int oldDepth = d->elements.size();
    QQuickStackElement *exit = d->elements.pop();
    QQuickStackElement *enter = d->elements.top();

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    if (argc > 0) {
        QV4::ScopedValue value(scope, (*args)[0]);
        if (value->isNull()) {
            enter = d->elements.value(0);
        } else if (const QV4::QObjectWrapper *o = value->as<QV4::QObjectWrapper>()) {
            QQuickItem *item = qobject_cast<QQuickItem *>(o->object());
            enter = d->findElement(item);
            if (!enter) {
                if (item != d->currentItem)
                    d->warn(QStringLiteral("can't find item to pop: ") + value->toQString());
                args->setReturnValue(QV4::Encode::null());
                d->elements.push(exit); // restore
                return;
            }
        }
    }

#if QT_CONFIG(quick_viewtransitions)
    Operation operation = PopTransition;
    if (argc > 0) {
        QV4::ScopedValue lastArg(scope, (*args)[argc - 1]);
        if (lastArg->isInt32())
            operation = static_cast<Operation>(lastArg->toInt32());
    }
#endif

    QPointer<QQuickItem> previousItem;

    if (d->popElements(enter)) {
        if (exit) {
            exit->removal = true;
            d->removing.insert(exit);
            previousItem = exit->item;
        }
        d->depthChange(d->elements.size(), oldDepth);
#if QT_CONFIG(quick_viewtransitions)
        d->startTransition(QQuickStackTransition::popExit(operation, exit, this),
                           QQuickStackTransition::popEnter(operation, enter, this),
                           operation == Immediate);
#endif
        d->setCurrentItem(enter);
    }

    if (previousItem) {
        QV4::ScopedValue rv(scope, QV4::QObjectWrapper::wrap(v4, previousItem));
        args->setReturnValue(rv->asReturnedValue());
    } else {
        args->setReturnValue(QV4::Encode::null());
    }
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::replace(target, item, properties, operation)

    Replaces one or more items on the stack with the specified \a item and
    optional \a operation, and optionally applies a set of \a properties on the
    item. The item can be an \l Item, \l Component, or a \l [QML] url.
    Returns the item that became current.

    \include qquickstackview.qdocinc pop-ownership

    If the \a target argument is specified, all items down to the \a target
    item will be replaced. If \a target is \c null, all items in the stack
    will be replaced. If not specified, only the top item will be replaced.

    StackView creates an instance automatically if the replacing item is a \l Component,
    or a \l [QML] url. The optional \a properties argument specifies a map of initial
    property values for the replacing item. For dynamically created items, these values
    are applied before the creation is finalized. This is more efficient than setting
    property values after creation, particularly where large sets of property values
    are defined, and also allows property bindings to be set up (using \l{Qt::binding}
    {Qt.binding()}) before the item is created.

    Replace the top item:
    \code
    stackView.replace(rect)

    // or with properties:
    stackView.replace(rect, {"color": "red"})
    \endcode

    Multiple items can be replaced at the same time either by passing them as
    additional arguments, or as an array. Each item can be followed by a set
    of properties to apply.

    Passing a variable amount of arguments:
    \code
    stackView.replace(rect1, rect2, rect3)

    // or with properties:
    stackView.replace(rect1, {"color": "red"}, rect2, {"color": "green"}, rect3, {"color": "blue"})
    \endcode

    Replacing an array of items:
    \code
    stackView.replace([rect1, rect2, rect3])

    // or with properties:
    stackView.replace([rect1, {"color": "red"}, rect2, {"color": "green"}, rect3, {"color": "blue"}])
    \endcode

    An \a operation can be optionally specified as the last argument. Supported
    operations:

    \value StackView.Immediate An immediate operation without transitions.
    \value StackView.PushTransition An operation with push transitions (since QtQuick.Controls 2.1).
    \value StackView.ReplaceTransition An operation with replace transitions (since QtQuick.Controls 2.1).
    \value StackView.PopTransition An operation with pop transitions (since QtQuick.Controls 2.1).

    If no operation is provided, \c Immediate will be used if the stack is
    empty, and \c ReplaceTransition otherwise.

    The following example illustrates the use of push and pop transitions with replace().

    \code
    StackView {
        id: stackView

        initialItem: Component {
            id: page

            Page {
                Row {
                    spacing: 20
                    anchors.centerIn: parent

                    Button {
                        text: "<"
                        onClicked: stackView.replace(page, StackView.PopTransition)
                    }
                    Button {
                        text: ">"
                        onClicked: stackView.replace(page, StackView.PushTransition)
                    }
                }
            }
        }
    }
    \endcode

    \note If you are \l {The QML script compiler}{compiling QML}, use the
    strongly-typed \l replaceCurrentItem functions instead.

    \sa push(), {Replacing Items}
*/
void QQuickStackView::replace(QQmlV4FunctionPtr args)
{
    Q_D(QQuickStackView);
    const QString operationName = QStringLiteral("replace");
    if (d->modifyingElements) {
        d->warnOfInterruption(operationName);
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QScopedValueRollback<bool> modifyingElements(d->modifyingElements, true);
    QScopedValueRollback<QString> operationNameRollback(d->operation, operationName);
    if (args->length() <= 0) {
        d->warn(QStringLiteral("missing arguments"));
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

#if QT_CONFIG(quick_viewtransitions)
    Operation operation = d->elements.isEmpty() ? Immediate : ReplaceTransition;
    QV4::ScopedValue lastArg(scope, (*args)[args->length() - 1]);
    if (lastArg->isInt32())
        operation = static_cast<Operation>(lastArg->toInt32());
#endif

    QQuickStackElement *target = nullptr;
    QV4::ScopedValue firstArg(scope, (*args)[0]);
    if (firstArg->isNull())
        target = d->elements.value(0);
    else if (!firstArg->isInt32())
        target = d->findElement(firstArg);

    QStringList errors;
    QList<QQuickStackElement *> elements = d->parseElements(target ? 1 : 0, args, &errors);
    if (!errors.isEmpty() || elements.isEmpty()) {
        if (!errors.isEmpty()) {
            for (const QString &error : std::as_const(errors))
                d->warn(error);
        } else {
            d->warn(QStringLiteral("nothing to push"));
        }
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    int oldDepth = d->elements.size();
    QQuickStackElement* exit = nullptr;
    if (!d->elements.isEmpty())
        exit = d->elements.pop();

    if (exit != target ? d->replaceElements(target, elements) : d->pushElements(elements)) {
        d->depthChange(d->elements.size(), oldDepth);
        if (exit) {
            exit->removal = true;
            d->removing.insert(exit);
        }
        QQuickStackElement *enter = d->elements.top();
#if QT_CONFIG(quick_viewtransitions)
        d->startTransition(QQuickStackTransition::replaceExit(operation, exit, this),
                           QQuickStackTransition::replaceEnter(operation, enter, this),
                           operation == Immediate);
#endif
        d->setCurrentItem(enter);
    }

    if (d->currentItem) {
        QV4::ScopedValue rv(scope, QV4::QObjectWrapper::wrap(v4, d->currentItem));
        args->setReturnValue(rv->asReturnedValue());
    } else {
        args->setReturnValue(QV4::Encode::null());
    }
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::pushItems(items, operation)
    \since 6.7

    Pushes \a items onto the stack using an optional \a operation, and
    optionally applies a set of properties on each element. \a items is an array
    of elements. Each element can be
    an \l Item, \l Component, or \l [QML] url and can be followed by an optional
    properties argument (see below). Returns the item that became
    current (the last item).

    StackView creates an instance automatically if the pushed element is a
    \l Component or \l [QML] url, and the instance will be destroyed when it is
    popped off the stack. See \l {Item Ownership} for more information.

    \include qquickstackview.qdocinc optional-properties-after-each-item

    \code
    stackView.push([item, rectComponent, Qt.resolvedUrl("MyItem.qml")])

    // With properties:
    stackView.pushItems([
        item, { "color": "red" },
        rectComponent, { "color": "green" },
        Qt.resolvedUrl("MyItem.qml"), { "color": "blue" }
    ])

    // With properties for only some items:
    stackView.pushItems([
        item, { "color": "yellow" },
        rectComponent
    ])
    \endcode

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c PushTransition will be used.

    To push a single item, use the relevant \c pushItem function:
    \list
    \li \l {stackview-pushitem-item-overload}
        {pushItem}(item, properties, operation)
    \li \l {stackview-pushitem-component-overload}
        {pushItem}(component, properties, operation)
    \li \l {stackview-pushitem-url-overload}
        {pushItem}(url, properties, operation)
    \endlist

    \note Items that already exist in the stack are not pushed.

    \sa initialItem, pushItem, {Pushing Items}
*/
QQuickItem *QQuickStackView::pushItems(QList<QQuickStackViewArg> args, Operation operation)
{
    Q_D(QQuickStackView);
    const QString operationName = QStringLiteral("pushItem");
    if (d->modifyingElements) {
        d->warnOfInterruption(operationName);
        return nullptr;
    }

    QScopedValueRollback<bool> modifyingElements(d->modifyingElements, true);
    QScopedValueRollback<QString> operationNameRollback(d->operation, operationName);

    const QList<QQuickStackElement *> stackElements = d->parseElements(args);

#if QT_CONFIG(quick_viewtransitions)
    QQuickStackElement *exit = nullptr;
    if (!d->elements.isEmpty())
        exit = d->elements.top();
#endif

    const int oldDepth = d->elements.size();
    if (d->pushElements(stackElements)) {
        d->depthChange(d->elements.size(), oldDepth);
        QQuickStackElement *enter = d->elements.top();
#if QT_CONFIG(quick_viewtransitions)
        d->startTransition(QQuickStackTransition::pushEnter(operation, enter, this),
            QQuickStackTransition::pushExit(operation, exit, this),
            operation == Immediate);
#endif
        d->setCurrentItem(enter);
    }

    return d->currentItem;
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::pushItem(item, properties, operation)
    \keyword stackview-pushitem-item-overload
    \since 6.7

    Pushes an \a item onto the stack, optionally applying a set of
    \a properties, using the optional \a operation. Returns the item that
    became current (the last item).

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c PushTransition will be used.

    To push several items onto the stack, use \l pushItems().

    \sa initialItem, {Pushing Items}
*/
QQuickItem *QQuickStackView::pushItem(QQuickItem *item, const QVariantMap &properties, Operation operation)
{
    return pushItems({ item, properties }, operation);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::pushItem(component, properties, operation)
    \overload pushItem()
    \keyword stackview-pushitem-component-overload
    \since 6.7

    Pushes a \a component onto the stack, optionally applying a set of
    \a properties, using the optional \a operation. Returns the item that
    became current (the last item).

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c PushTransition will be used.

    To push several items onto the stack, use \l pushItems().

    \sa initialItem, {Pushing Items}
*/
QQuickItem *QQuickStackView::pushItem(QQmlComponent *component, const QVariantMap &properties, Operation operation)
{
    return pushItems({ component, properties }, operation);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::pushItem(url, properties, operation)
    \overload pushItem()
    \keyword stackview-pushitem-url-overload
    \since 6.7

    Pushes a \a url onto the stack, optionally applying a set of
    \a properties, using the optional \a operation. Returns the item that
    became current (the last item).

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c PushTransition will be used.

    To push several items onto the stack, use \l pushItems().

    \sa initialItem, {Pushing Items}
*/
QQuickItem *QQuickStackView::pushItem(const QUrl &url, const QVariantMap &properties, Operation operation)
{
    return pushItems({ url, properties }, operation);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::popToItem(item, operation)
    \since 6.7

    Pops all items down to (but not including) \a item. Returns the last item
    removed from the stack.

    If \a item is \c null, a warning is produced and \c null is returned.

    \include qquickstackview.qdocinc pop-ownership

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c PopTransition will be used.

    \code
    stackView.popToItem(someItem, StackView.Immediate)
    \endcode

    \sa clear(), {Popping Items}, {Unwinding Items via Pop}
*/
QQuickItem *QQuickStackView::popToItem(QQuickItem *item, Operation operation)
{
    Q_D(QQuickStackView);
    return d->popToItem(item, operation, QQuickStackViewPrivate::CurrentItemPolicy::DoNotPop);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::popToIndex(index, operation)
    \since 6.7

    Pops all items down to (but not including) \a index. Returns the last item
    removed from the stack.

    If \a index is out of bounds, a warning is produced and \c null is
    returned.

    \include qquickstackview.qdocinc pop-ownership

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c PopTransition will be used.

    \code
    stackView.popToIndex(stackView.depth - 2, StackView.Immediate)
    \endcode

    \sa clear(), {Popping Items}, {Unwinding Items via Pop}
*/
QQuickItem *QQuickStackView::popToIndex(int index, Operation operation)
{
    Q_D(QQuickStackView);
    if (index < 0 || index >= d->elements.size()) {
        d->warn(QString::fromLatin1("popToIndex: index %1 is out of bounds (%2 item(s))")
            .arg(index).arg(d->elements.size()));
        return nullptr;
    }

    if (index == d->elements.size() - 1) {
        // This would pop down to the current item, which is a no-op.
        return nullptr;
    }

    QQuickStackElement *element = d->elements.at(index);
    element->load(this);
    return d->popToItem(element->item, operation, QQuickStackViewPrivate::CurrentItemPolicy::Pop);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::popCurrentItem(operation)
    \since 6.7

    Pops \l currentItem from the stack. Returns the last item removed from the
    stack, or \c null if \l depth was \c 1.

    \include qquickstackview.qdocinc pop-ownership

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c PopTransition will be used.

    This function is equivalent to \c popToIndex(stackView.currentIndex - 1).

    \sa clear(), {Popping Items}, {Unwinding Items via Pop}
*/
QQuickItem *QQuickStackView::popCurrentItem(Operation operation)
{
    Q_D(QQuickStackView);
    if (d->elements.size() == 1) {
        auto lastItemRemoved = d->elements.last()->item;
        clear(operation);
        return lastItemRemoved;
    }
    return d->popToItem(d->currentItem, operation, QQuickStackViewPrivate::CurrentItemPolicy::Pop);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::replaceCurrentItem(items, operation)
    \keyword stackview-replacecurrentitem-items-overload
    \since 6.7

    Pops \l currentItem from the stack and pushes \a items. If the optional
    \a operation is specified, the relevant transition will be used. Each item
    can be followed by an optional set of properties that will be applied to
    that item. Returns the item that became current.

    \include qquickstackview.qdocinc optional-properties-after-each-item

    \include qquickstackview.qdocinc pop-ownership

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c ReplaceTransition will be used.

    \code
    stackView.replaceCurrentItem([item, rectComponent, Qt.resolvedUrl("MyItem.qml")])

    // With properties:
    stackView.replaceCurrentItem([
        item, { "color": "red" },
        rectComponent, { "color": "green" },
        Qt.resolvedUrl("MyItem.qml"), { "color": "blue" }
    ])
    \endcode

    To push a single item, use the relevant overload:
    \list
    \li \l {stackview-replacecurrentitem-item-overload}
        {replaceCurrentItem}(item, properties, operation)
    \li \l {stackview-replacecurrentitem-component-overload}
        {replaceCurrentItem}(component, properties, operation)
    \li \l {stackview-replacecurrentitem-url-overload}
        {replaceCurrentItem}(url, properties, operation)
    \endlist

    \sa push(), {Replacing Items}
*/
QQuickItem *QQuickStackView::replaceCurrentItem(const QList<QQuickStackViewArg> &args,
    Operation operation)
{
    Q_D(QQuickStackView);
    const QString operationName = QStringLiteral("replace");
    if (d->modifyingElements) {
        d->warnOfInterruption(operationName);
        return nullptr;
    }

    QScopedValueRollback<bool> modifyingElements(d->modifyingElements, true);
    QScopedValueRollback<QString> operationNameRollback(d->operation, operationName);

    QQuickStackElement *currentElement = !d->elements.isEmpty() ? d->elements.top() : nullptr;

    const QList<QQuickStackElement *> stackElements = d->parseElements(args);

    int oldDepth = d->elements.size();
    QQuickStackElement* exit = nullptr;
    if (!d->elements.isEmpty())
        exit = d->elements.pop();

    const bool successfullyReplaced = exit != currentElement
        ? d->replaceElements(currentElement, stackElements)
        : d->pushElements(stackElements);
    if (successfullyReplaced) {
        d->depthChange(d->elements.size(), oldDepth);
        if (exit) {
            exit->removal = true;
            d->removing.insert(exit);
        }
        QQuickStackElement *enter = d->elements.top();
#if QT_CONFIG(quick_viewtransitions)
        d->startTransition(QQuickStackTransition::replaceExit(operation, exit, this),
            QQuickStackTransition::replaceEnter(operation, enter, this),
            operation == Immediate);
#endif
        d->setCurrentItem(enter);
    }

    return d->currentItem;
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::replaceCurrentItem(item, properties, operation)
    \overload replaceCurrentItem()
    \keyword stackview-replacecurrentitem-item-overload
    \since 6.7

    \include qquickstackview.qdocinc {replaceCurrentItem} {item}

    \include qquickstackview.qdocinc pop-ownership

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c ReplaceTransition will be used.

    To push several items onto the stack, use
    \l {stackview-replacecurrentitem-items-overload}
    {replaceCurrentItem}(items, operation).

    \sa {Replacing Items}
*/
QQuickItem *QQuickStackView::replaceCurrentItem(QQuickItem *item, const QVariantMap &properties,
    Operation operation)
{
    const QList<QQuickStackViewArg> args = { QQuickStackViewArg(item), QQuickStackViewArg(properties) };
    return replaceCurrentItem(args, operation);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::replaceCurrentItem(component, properties, operation)
    \overload replaceCurrentItem()
    \keyword stackview-replacecurrentitem-component-overload
    \since 6.7

    \include qquickstackview.qdocinc {replaceCurrentItem} {component}

    \include qquickstackview.qdocinc pop-ownership

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c ReplaceTransition will be used.

    To push several items onto the stack, use
    \l {stackview-replacecurrentitem-items-overload}
    {replaceCurrentItem}(items, operation).

    \sa {Replacing Items}
*/
QQuickItem *QQuickStackView::replaceCurrentItem(QQmlComponent *component, const QVariantMap &properties,
    Operation operation)
{
    const QList<QQuickStackViewArg> args = { QQuickStackViewArg(component), QQuickStackViewArg(properties) };
    return replaceCurrentItem(args, operation);
}

/*!
    \qmlmethod Item QtQuick.Controls::StackView::replaceCurrentItem(url, properties, operation)
    \keyword stackview-replacecurrentitem-url-overload
    \overload replaceCurrentItem()
    \since 6.7

    \include qquickstackview.qdocinc {replaceCurrentItem} {url}

    \include qquickstackview.qdocinc pop-ownership

    \include qquickstackview.qdocinc operation-values

    If no operation is provided, \c ReplaceTransition will be used.

    To push several items onto the stack, use
    \l {stackview-replacecurrentitem-items-overload}
    {replaceCurrentItem}(items, operation).

    \sa {Replacing Items}
*/
QQuickItem *QQuickStackView::replaceCurrentItem(const QUrl &url, const QVariantMap &properties,
    Operation operation)
{
    const QList<QQuickStackViewArg> args = { QQuickStackViewArg(url), QQuickStackViewArg(properties) };
    return replaceCurrentItem(args, operation);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::StackView::empty
    \readonly

    This property holds whether the stack is empty.

    \sa depth
*/
bool QQuickStackView::isEmpty() const
{
    Q_D(const QQuickStackView);
    return d->elements.isEmpty();
}

/*!
    \qmlmethod void QtQuick.Controls::StackView::clear(transition)

    Removes all items from the stack.

    \include qquickstackview.qdocinc pop-ownership

    Since QtQuick.Controls 2.3, a \a transition can be optionally specified. Supported transitions:

    \value StackView.Immediate Clear the stack immediately without any transition (default).
    \value StackView.PushTransition Clear the stack with a push transition.
    \value StackView.ReplaceTransition Clear the stack with a replace transition.
    \value StackView.PopTransition Clear the stack with a pop transition.
*/
void QQuickStackView::clear(Operation operation)
{
#if !QT_CONFIG(quick_viewtransitions)
    Q_UNUSED(operation)
#endif
    Q_D(QQuickStackView);
    if (d->elements.isEmpty())
        return;

    const QString operationName = QStringLiteral("clear");
    if (d->modifyingElements) {
        d->warnOfInterruption(operationName);
        return;
    }

    const int oldDepth = d->elements.size();

    QScopedValueRollback<bool> modifyingElements(d->modifyingElements, true);
    QScopedValueRollback<QString> operationNameRollback(d->operation, operationName);
#if QT_CONFIG(quick_viewtransitions)
    if (operation != Immediate) {
        QQuickStackElement *exit = d->elements.pop();
        exit->removal = true;
        d->removing.insert(exit);
        d->startTransition(QQuickStackTransition::popExit(operation, exit, this),
                           QQuickStackTransition::popEnter(operation, nullptr, this), false);
    }
#endif

    d->setCurrentItem(nullptr);
    qDeleteAll(d->elements);
    d->elements.clear();
    d->depthChange(0, oldDepth);
}

/*!
    \qmlproperty var QtQuick.Controls::StackView::initialItem

    This property holds the initial item that should be shown when the StackView
    is created. The initial item can be an \l Item, \l Component, or a \l [QML] url.
    Specifying an initial item is equivalent to:
    \code
    Component.onCompleted: stackView.push(myInitialItem)
    \endcode

    \sa push()
*/
QJSValue QQuickStackView::initialItem() const
{
    Q_D(const QQuickStackView);
    return d->initialItem;
}

void QQuickStackView::setInitialItem(const QJSValue &item)
{
    Q_D(QQuickStackView);
    d->initialItem = item;
}

#if QT_CONFIG(quick_viewtransitions)
/*!
    \qmlproperty Transition QtQuick.Controls::StackView::popEnter

    This property holds the transition that is applied to the item that
    enters the stack when another item is popped off of it.

    \sa {Customizing StackView}
*/
QQuickTransition *QQuickStackView::popEnter() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->removeDisplacedTransition;
    return nullptr;
}

void QQuickStackView::setPopEnter(QQuickTransition *enter)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->removeDisplacedTransition == enter)
        return;

    d->transitioner->removeDisplacedTransition = enter;
    emit popEnterChanged();
}

/*!
    \qmlproperty Transition QtQuick.Controls::StackView::popExit

    This property holds the transition that is applied to the item that
    exits the stack when the item is popped off of it.

    \sa {Customizing StackView}
*/
QQuickTransition *QQuickStackView::popExit() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->removeTransition;
    return nullptr;
}

void QQuickStackView::setPopExit(QQuickTransition *exit)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->removeTransition == exit)
        return;

    d->transitioner->removeTransition = exit;
    emit popExitChanged();
}

/*!
    \qmlproperty Transition QtQuick.Controls::StackView::pushEnter

    This property holds the transition that is applied to the item that
    enters the stack when the item is pushed onto it.

    \sa {Customizing StackView}
*/
QQuickTransition *QQuickStackView::pushEnter() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->addTransition;
    return nullptr;
}

void QQuickStackView::setPushEnter(QQuickTransition *enter)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->addTransition == enter)
        return;

    d->transitioner->addTransition = enter;
    emit pushEnterChanged();
}

/*!
    \qmlproperty Transition QtQuick.Controls::StackView::pushExit

    This property holds the transition that is applied to the item that
    exits the stack when another item is pushed onto it.

    \sa {Customizing StackView}
*/
QQuickTransition *QQuickStackView::pushExit() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->addDisplacedTransition;
    return nullptr;
}

void QQuickStackView::setPushExit(QQuickTransition *exit)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->addDisplacedTransition == exit)
        return;

    d->transitioner->addDisplacedTransition = exit;
    emit pushExitChanged();
}

/*!
    \qmlproperty Transition QtQuick.Controls::StackView::replaceEnter

    This property holds the transition that is applied to the item that
    enters the stack when another item is replaced by it.

    \sa {Customizing StackView}
*/
QQuickTransition *QQuickStackView::replaceEnter() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->moveTransition;
    return nullptr;
}

void QQuickStackView::setReplaceEnter(QQuickTransition *enter)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->moveTransition == enter)
        return;

    d->transitioner->moveTransition = enter;
    emit replaceEnterChanged();
}

/*!
    \qmlproperty Transition QtQuick.Controls::StackView::replaceExit

    This property holds the transition that is applied to the item that
    exits the stack when it is replaced by another item.

    \sa {Customizing StackView}
*/
QQuickTransition *QQuickStackView::replaceExit() const
{
    Q_D(const QQuickStackView);
    if (d->transitioner)
        return d->transitioner->moveDisplacedTransition;
    return nullptr;
}

void QQuickStackView::setReplaceExit(QQuickTransition *exit)
{
    Q_D(QQuickStackView);
    d->ensureTransitioner();
    if (d->transitioner->moveDisplacedTransition == exit)
        return;

    d->transitioner->moveDisplacedTransition = exit;
    emit replaceExitChanged();
}
#endif

void QQuickStackView::componentComplete()
{
    QQuickControl::componentComplete();

    Q_D(QQuickStackView);
    QScopedValueRollback<QString> operationNameRollback(d->operation, QStringLiteral("initialItem"));
    QQuickStackElement *element = nullptr;
    QString error;
    int oldDepth = d->elements.size();
    if (QObject *o = d->initialItem.toQObject())
        element = QQuickStackElement::fromObject(o, this, &error);
    else if (d->initialItem.isString())
        element = QQuickStackElement::fromString(d->initialItem.toString(), this, &error);
    if (!error.isEmpty()) {
        d->warn(error);
        delete element;
    } else if (d->pushElement(element)) {
        d->depthChange(d->elements.size(), oldDepth);
        d->setCurrentItem(element);
        element->setStatus(QQuickStackView::Active);
    }
}

void QQuickStackView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickControl::geometryChange(newGeometry, oldGeometry);

    Q_D(QQuickStackView);
    for (QQuickStackElement *element : std::as_const(d->elements)) {
        if (element->item) {
            if (!element->widthValid)
                element->item->setWidth(newGeometry.width());
            if (!element->heightValid)
                element->item->setHeight(newGeometry.height());
        }
    }
}

bool QQuickStackView::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    // in order to block accidental user interaction while busy/transitioning,
    // StackView filters out childrens' mouse events. therefore we block all
    // press events. however, since push() may be called from signal handlers
    // such as onPressed or onDoubleClicked, we must let the current mouse
    // grabber item receive the respective mouse release event to avoid
    // breaking its state (QTBUG-50305).
    if (event->type() == QEvent::MouseButtonPress)
        return true;
    if (event->type() == QEvent::UngrabMouse)
        return false;
    QQuickWindow *window = item->window();
    return window && !window->mouseGrabberItem();
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickStackView::touchEvent(QTouchEvent *event)
{
    event->ignore(); // QTBUG-65084
}
#endif

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickStackView::accessibleRole() const
{
    return QAccessible::LayeredPane;
}
#endif

void QQuickStackViewAttachedPrivate::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    Q_Q(QQuickStackViewAttached);
    int oldIndex = element ? element->index : -1;
    QQuickStackView *oldView = element ? element->view : nullptr;
    QQuickStackView::Status oldStatus = element ? element->status : QQuickStackView::Inactive;

    QQuickStackView *newView = qobject_cast<QQuickStackView *>(parent);
    element = newView ? QQuickStackViewPrivate::get(newView)->findElement(item) : nullptr;

    int newIndex = element ? element->index : -1;
    QQuickStackView::Status newStatus = element ? element->status : QQuickStackView::Inactive;

    if (oldIndex != newIndex)
        emit q->indexChanged();
    if (oldView != newView)
        emit q->viewChanged();
    if (oldStatus != newStatus)
        emit q->statusChanged();
}

QQuickStackViewAttached::QQuickStackViewAttached(QObject *parent)
    : QObject(*(new QQuickStackViewAttachedPrivate), parent)
{
    Q_D(QQuickStackViewAttached);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item) {
        connect(item, &QQuickItem::visibleChanged, this, &QQuickStackViewAttached::visibleChanged);
        QQuickItemPrivate::get(item)->addItemChangeListener(d, QQuickItemPrivate::Parent);
        d->itemParentChanged(item, item->parentItem());
    } else if (parent) {
        qmlWarning(parent) << "StackView attached property must be attached to an object deriving from Item";
    }
}

QQuickStackViewAttached::~QQuickStackViewAttached()
{
    Q_D(QQuickStackViewAttached);
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        QQuickItemPrivate::get(parentItem)->removeItemChangeListener(d, QQuickItemPrivate::Parent);
}

/*!
    \qmlattachedproperty int QtQuick.Controls::StackView::index
    \readonly

    This attached property holds the stack index of the item it's
    attached to, or \c -1 if the item is not in a stack.
*/
int QQuickStackViewAttached::index() const
{
    Q_D(const QQuickStackViewAttached);
    return d->element ? d->element->index : -1;
}

/*!
    \qmlattachedproperty StackView QtQuick.Controls::StackView::view
    \readonly

    This attached property holds the stack view of the item it's
    attached to, or \c null if the item is not in a stack.
*/
QQuickStackView *QQuickStackViewAttached::view() const
{
    Q_D(const QQuickStackViewAttached);
    return d->element ? d->element->view : nullptr;
}

/*!
    \qmlattachedproperty enumeration QtQuick.Controls::StackView::status
    \readonly

    This attached property holds the stack status of the item it's
    attached to, or \c StackView.Inactive if the item is not in a stack.

    Available values:
    \value StackView.Inactive The item is inactive (or not in a stack).
    \value StackView.Deactivating The item is being deactivated (popped off).
    \value StackView.Activating The item is being activated (becoming the current item).
    \value StackView.Active The item is active, that is, the current item.
*/
QQuickStackView::Status QQuickStackViewAttached::status() const
{
    Q_D(const QQuickStackViewAttached);
    return d->element ? d->element->status : QQuickStackView::Inactive;
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlattachedproperty bool QtQuick.Controls::StackView::visible

    This attached property holds the visibility of the item it's attached to.
    The value follows the value of \l Item::visible.

    By default, StackView shows incoming items when the enter transition begins,
    and hides outgoing items when the exit transition ends. Setting this property
    explicitly allows the default behavior to be overridden, making it possible
    to keep items that are below the top-most item visible.

    \note The default transitions of most styles slide outgoing items outside the
          view, and may also animate their opacity. In order to keep a full stack
          of items visible, consider customizing the \l transitions so that the
          items underneath can be seen.

    \image qtquickcontrols-stackview-visible.png

    \snippet qtquickcontrols-stackview-visible.qml 1
*/
bool QQuickStackViewAttached::isVisible() const
{
    const QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    return parentItem && parentItem->isVisible();
}

void QQuickStackViewAttached::setVisible(bool visible)
{
    Q_D(QQuickStackViewAttached);
    d->explicitVisible = true;
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        parentItem->setVisible(visible);
}

void QQuickStackViewAttached::resetVisible()
{
    Q_D(QQuickStackViewAttached);
    d->explicitVisible = false;
    if (!d->element || !d->element->view)
        return;

    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        parentItem->setVisible(parentItem == d->element->view->currentItem());
}

/*!
    \qmlattachedsignal QtQuick.Controls::StackView::activated()
    \since QtQuick.Controls 2.1 (Qt 5.8)

    This attached signal is emitted when the item it's attached to is activated in the stack.

    \sa status
*/

/*!
    \qmlattachedsignal QtQuick.Controls::StackView::deactivated()
    \since QtQuick.Controls 2.1 (Qt 5.8)

    This attached signal is emitted when the item it's attached to is deactivated in the stack.

    \sa status
*/

/*!
    \qmlattachedsignal QtQuick.Controls::StackView::activating()
    \since QtQuick.Controls 2.1 (Qt 5.8)

    This attached signal is emitted when the item it's attached to is in the process of being
    activated in the stack.

    \sa status
*/

/*!
    \qmlattachedsignal QtQuick.Controls::StackView::deactivating()
    \since QtQuick.Controls 2.1 (Qt 5.8)

    This attached signal is emitted when the item it's attached to is in the process of being
    dectivated in the stack.

    \sa status
*/

/*!
    \qmlattachedsignal QtQuick.Controls::StackView::removed()
    \since QtQuick.Controls 2.1 (Qt 5.8)

    This attached signal is emitted when the item it's attached to has been
    removed from the stack. It can be used to safely destroy an Item that was
    pushed onto the stack, for example:

    \code
    Item {
        StackView.onRemoved: destroy() // Will be destroyed sometime after this call.
    }
    \endcode

    \sa status
*/

QT_END_NAMESPACE

#include "moc_qquickstackview_p.cpp"
