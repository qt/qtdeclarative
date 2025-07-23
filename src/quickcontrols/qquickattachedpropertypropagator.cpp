// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickattachedpropertypropagator.h"

#include <QtCore/qpointer.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquickpopupitem_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupwindow_p_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcAttached, "qt.quick.controls.attachedpropertypropagator")

/*!
    \class QQuickAttachedPropertyPropagator
    \brief The QQuickAttachedPropertyPropagator class provides a way to
    propagate attached properties.
    \inmodule QtQuickControls2
    \since 6.5

    In QML, it is possible to
    \l {Attached Properties and Attached Signal Handlers}{attach properties and
    signal handlers} to objects. \l {Providing Attached Properties} goes into more
    detail about how to expose your own C++ attached types.

    QQuickAttachedPropertyPropagator provides an API to propagate attached
    properties from a parent object to its children, similar to
    \l {Control::}{font} and \l {Item::}{palette} propagation. It supports
    propagation through \l {Item}{items}, \l {Popup}{popups}, and
    \l {Window}{windows}.

    If propagation of properties is not important, consider using a
    \l {QML_SINGLETON}{C++} or
    \l {Contents of a Module Definition qmldir File}{QML} singleton instead,
    as it is better suited for that use case, and is more efficient in that
    it only requires one QObject.

    To implement a custom attached property:

    \list 1
    \li Derive a class that exposes the attached property from
        QQuickAttachedPropertyPropagator.

        For example, to implement an attached \c {MyStyle.theme} property,
        declare the \c {MyStyle} class:

        \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.h
        \skipto class
        \printto {

    \li Call \l initialize() in the constructor of your class:

        \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
        \skipto MyStyle::MyStyle
        \printuntil }

    \li Define set/inherit/propagate/reset functions for the attached property
        as needed. For example, to define a \c theme attached property:

        \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
        \skipto theme()
        \printto MyStyle::themeChange()

    \li Reimplement \l attachedParentChange() to handle property inheritance:

        \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
        \skipto attachedParentChange
        \printuntil /^\}/

    \li Implement a static \c qmlAttachedProperties function and declare the
        type as an attached QML type with \l QML_ELEMENT and \l QML_ATTACHED,
        as detailed in \l {Providing Attached Properties}:

        \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
        \skipto qmlAttachedProperties
        \printuntil }

    \endlist

    The complete implementation is available in
    \l {Qt Quick Controls - Attached Style Properties Example}.

    \sa {Styling Qt Quick Controls}
*/

static QQuickAttachedPropertyPropagator *attachedObject(const QMetaObject *type, QObject *object, bool create = false)
{
    if (!object)
        return nullptr;
    auto func = qmlAttachedPropertiesFunction(object, type);
    return qobject_cast<QQuickAttachedPropertyPropagator *>(qmlAttachedPropertiesObject(object, func, create));
}

/*!
    \internal

    Tries to find a QQuickAttachedPropertyPropagator whose type is \a ourAttachedType
    and is attached to an ancestor of \a objectWeAreAttachedTo.

    QQuickAttachedPropertyPropagator needs to know who its parent attached object is in
    order to inherit attached property values from it. This is called when an
    instance of QQuickAttachedPropertyPropagator is created, and whenever
    \c {objectWeAreAttachedTo}'s parent changes, for example.
*/
static QQuickAttachedPropertyPropagator *findAttachedParent(const QMetaObject *ourAttachedType, QObject *objectWeAreAttachedTo)
{
    qCDebug(lcAttached).noquote() << "findAttachedParent called with" << ourAttachedType->className() << objectWeAreAttachedTo;
    /*
        In the Material ComboBox.qml, we have code like this:

        popup: T.Popup {
            // ...
            Material.theme: control.Material.theme
            // ...

            background: Rectangle {
                //...
                color: parent.Material.dialogColor

         The Material attached object has to be accessed this way due to
         deferred execution limitations (see 3e87695fb4b1a5d503c744046e6d9f43a2ae18a6).
         However, since parent here refers to QQuickPopupItem and not the popup,
         the color will actually come from the window. If a dark theme was set on
         the ComboBox, it will not be respected in the background if we don't have
         the check below.
    */
    auto popupItem = qobject_cast<QQuickPopupItem *>(objectWeAreAttachedTo);
    if (popupItem) {
        qCDebug(lcAttached).noquote() << "- attachee is a popup item" << popupItem << "- checking if it has an attached object";
        auto popupItemPrivate = QQuickPopupItemPrivate::get(popupItem);
        QQuickAttachedPropertyPropagator *popupAttached = attachedObject(ourAttachedType, popupItemPrivate->popup);
        if (popupAttached) {
            qCDebug(lcAttached).noquote() << "- popup item has attached object" << popupAttached << "- returning";
            return popupAttached;
        } else {
            qCDebug(lcAttached).noquote() << "- popup item does not have attached object";
        }
    } else {
        qCDebug(lcAttached).noquote() << "- attachee is not a popup item";
    }

    QQuickItem *item = qobject_cast<QQuickItem *>(objectWeAreAttachedTo);
    if (item) {
        qCDebug(lcAttached).noquote() << "- attachee is an item; checking its parent items and popups";
        // lookup parent items and popups
        QQuickItem *parent = item->parentItem();
        while (parent) {
            qCDebug(lcAttached).noquote() << "  - checking parent item" << parent;
            QQuickAttachedPropertyPropagator *attached = attachedObject(ourAttachedType, parent);
            if (attached) {
                qCDebug(lcAttached).noquote() << "  - parent item has attached object" << attached << "- returning";
                return attached;
            }

            QQuickPopup *popup = qobject_cast<QQuickPopup *>(parent->parent());
            if (popup) {
                qCDebug(lcAttached).noquote() << "  - parent popup has attached object" << attached << "- returning";
                return attachedObject(ourAttachedType, popup);
            }

            parent = parent->parentItem();
        }

        // fallback to item's window
        qCDebug(lcAttached).noquote() << "- checking parent window" << item->window();
        QQuickAttachedPropertyPropagator *attached = attachedObject(ourAttachedType, item->window());
        if (attached) {
            qCDebug(lcAttached).noquote() << "- parent window has attached object" << attached << "- returning";
            return attached;
        }
    } else {
        // lookup popup's window
        QQuickPopup *popup = qobject_cast<QQuickPopup *>(objectWeAreAttachedTo);
        if (popup) {
            qCDebug(lcAttached).noquote() << "- attachee is a popup; checking its window";
            auto* popupWindow = popup->popupItem()->window();
            auto *object = attachedObject(ourAttachedType, popupWindow);
            // Check if the attached object exists for the popup window. If it
            // doesn't, use transient parent attached object
            if (!object && qobject_cast<QQuickPopupWindow *>(popupWindow))
                return attachedObject(ourAttachedType, popupWindow->transientParent());
            return object;
        }
    }

    // lookup parent window
    QQuickWindow *window = qobject_cast<QQuickWindow *>(objectWeAreAttachedTo);
    if (window) {
        // It doesn't seem like a parent window can be anything but transient in Qt Quick.
        QQuickWindow *parentWindow = qobject_cast<QQuickWindow *>(window->transientParent());
        qCDebug(lcAttached).noquote() << "- attachee is a window; checking its parent window" << parentWindow;
        if (parentWindow) {
            QQuickAttachedPropertyPropagator *attached = attachedObject(ourAttachedType, parentWindow);
            if (attached) {
                qCDebug(lcAttached).noquote() << "- parent window has attached object" << attached << "- returning";
                return attached;
            }
        }
    }

    // fallback to engine (global)
    if (objectWeAreAttachedTo) {
        QQmlEngine *engine = qmlEngine(objectWeAreAttachedTo);
        qCDebug(lcAttached).noquote() << "- falling back to engine" << engine;
        if (engine) {
            QByteArray name = QByteArray("_q_") + ourAttachedType->className();
            QQuickAttachedPropertyPropagator *attached = engine->property(name).value<QQuickAttachedPropertyPropagator *>();
            if (!attached) {
                attached = attachedObject(ourAttachedType, engine, true);
                engine->setProperty(name, QVariant::fromValue(attached));
            }
            return attached;
        }
    }

    return nullptr;
}

static QList<QQuickAttachedPropertyPropagator *> findAttachedChildren(const QMetaObject *type, QObject *object)
{
    QList<QQuickAttachedPropertyPropagator *> children;

    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
        if (window)
            item = window->contentItem();
    }

    if (!item)
        return children;

    // At this point, "item" could either be an item that the attached object is
    // attached to directly, or the contentItem of a window that the attached object
    // is attached to.

    // Look for attached properties on items.
    const auto childItems = item->childItems();
    for (QQuickItem *child : childItems) {
        QQuickAttachedPropertyPropagator *attached = attachedObject(type, child);
        if (attached)
            children += attached;
        else
            children += findAttachedChildren(type, child);
    }

    // Look for attached properties on windows. Windows declared in QML
    // as children of a Window are QObject-parented to the contentItem (see
    // QQuickWindowPrivate::data_append()). Windows declared as children
    // of items are QObject-parented to those items.
    const auto &windowChildren = item->children();
    for (QObject *child : windowChildren) {
        QQuickWindow *childWindow = qobject_cast<QQuickWindow *>(child);
        if (childWindow) {
            QQuickAttachedPropertyPropagator *attached = attachedObject(type, childWindow);
            if (attached)
                children += attached;
        }
    }

    return children;
}

static QQuickItem *findAttachedItem(QObject *parent)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (!item) {
        QQuickPopup *popup = qobject_cast<QQuickPopup *>(parent);
        if (popup)
            item = popup->popupItem();
    }
    return item;
}

class QQuickAttachedPropertyPropagatorPrivate : public QObjectPrivate,
                                                public QSafeQuickItemChangeListener<QQuickAttachedPropertyPropagatorPrivate>
{
public:
    Q_DECLARE_PUBLIC(QQuickAttachedPropertyPropagator)

    static QQuickAttachedPropertyPropagatorPrivate *get(QQuickAttachedPropertyPropagator *attachedObject)
    {
        return attachedObject->d_func();
    }

    void attachTo(QObject *object);
    void detachFrom(QObject *object);
    void setAttachedParent(QQuickAttachedPropertyPropagator *parent);

    void itemWindowChanged(QQuickWindow *window);
    void transientParentWindowChanged(QWindow *newTransientParent);
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) override;

    QList<QQuickAttachedPropertyPropagator *> attachedChildren;
    QPointer<QQuickAttachedPropertyPropagator> attachedParent;
};

void QQuickAttachedPropertyPropagatorPrivate::attachTo(QObject *object)
{
    if (QQuickItem *item = findAttachedItem(object)) {
        connect(item, &QQuickItem::windowChanged, this, &QQuickAttachedPropertyPropagatorPrivate::itemWindowChanged);
        QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Parent);
    } else if (auto *window = qobject_cast<QQuickWindow *>(object)) {
        QObjectPrivate::connect(window, &QWindow::transientParentChanged, this,
            &QQuickAttachedPropertyPropagatorPrivate::transientParentWindowChanged);
    }
}

void QQuickAttachedPropertyPropagatorPrivate::detachFrom(QObject *object)
{
    if (QQuickItem *item = findAttachedItem(object)) {
        disconnect(item, &QQuickItem::windowChanged, this, &QQuickAttachedPropertyPropagatorPrivate::itemWindowChanged);
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Parent);
    } else if (auto *window = qobject_cast<QQuickWindow *>(object)) {
        QObjectPrivate::disconnect(window, &QWindow::transientParentChanged,
            this, &QQuickAttachedPropertyPropagatorPrivate::transientParentWindowChanged);
    }
}

/*!
    \internal

    This function sets the attached parent of this attached object.

    Currently it is called when:
    \list
    \li The target item's parent changes.
    \li The target item's window changes.
    \li The attached object is constructed, to set the attached parent
        and the attached parent of the attached object children.
    \li The attached object is destructed.
    \endlist

    \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
    \skipto MyStyle::resetTheme
    \printuntil }
*/
void QQuickAttachedPropertyPropagatorPrivate::setAttachedParent(QQuickAttachedPropertyPropagator *parent)
{
    Q_Q(QQuickAttachedPropertyPropagator);
    if (attachedParent == parent)
        return;

    QQuickAttachedPropertyPropagator *oldParent = attachedParent;
    qCDebug(lcAttached).noquote() << "setAttachedParent called on" << q << "with parent" << parent;
    if (attachedParent) {
        qCDebug(lcAttached).noquote() << "- removing ourselves as an attached child of" << attachedParent;
        QQuickAttachedPropertyPropagatorPrivate::get(attachedParent)->attachedChildren.removeOne(q);
    }
    attachedParent = parent;
    if (parent) {
        qCDebug(lcAttached).noquote() << "- adding ourselves as an attached child of" << parent;
        QQuickAttachedPropertyPropagatorPrivate::get(parent)->attachedChildren.append(q);
    }
    q->attachedParentChange(parent, oldParent);
}

/*
    If there's e.g. code like this:

        Behavior on Material.elevation {}

    The meta type will be something like QQuickMaterialStyle_QML_125,
    whereas QQmlMetaType::attachedPropertiesFunc only has attached
    property data for QQuickMaterialStyle (i.e. attached property types
    created from C++). We work around this by finding the first C++
    meta object, which works even for attached types created in QML.
*/
const QMetaObject *firstCppMetaObject(QQuickAttachedPropertyPropagator *propagator)
{
    return QQmlData::ensurePropertyCache(propagator)->firstCppMetaObject();
}

void QQuickAttachedPropertyPropagatorPrivate::itemWindowChanged(QQuickWindow *window)
{
    Q_Q(QQuickAttachedPropertyPropagator);
    QQuickAttachedPropertyPropagator *attachedParent = nullptr;
    qCDebug(lcAttached).noquote() << "window of" << q << "changed to" << window;

    attachedParent = findAttachedParent(firstCppMetaObject(q), q->parent());
    if (!attachedParent)
        attachedParent = attachedObject(firstCppMetaObject(q), window);
    setAttachedParent(attachedParent);
}

void QQuickAttachedPropertyPropagatorPrivate::transientParentWindowChanged(QWindow *newTransientParent)
{
    Q_Q(QQuickAttachedPropertyPropagator);
    QQuickAttachedPropertyPropagator *attachedParent = nullptr;
    qCDebug(lcAttached).noquote() << "transient parent window of" << q << "changed to" << newTransientParent;
    attachedParent = findAttachedParent(firstCppMetaObject(q), q->parent());
    if (!attachedParent)
        attachedParent = attachedObject(firstCppMetaObject(q), newTransientParent);
    setAttachedParent(attachedParent);
}

void QQuickAttachedPropertyPropagatorPrivate::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    Q_Q(QQuickAttachedPropertyPropagator);
    Q_UNUSED(item);
    Q_UNUSED(parent);
    setAttachedParent(findAttachedParent(firstCppMetaObject(q), q->parent()));
}

/*!
    Constructs a QQuickAttachedPropertyPropagator with the given \a parent.

    The \c parent will be used to find this object's
    \l {attachedParent()}{attached parent}.

    Derived classes should call \l initialize() in their constructor.
*/
QQuickAttachedPropertyPropagator::QQuickAttachedPropertyPropagator(QObject *parent)
    : QObject(*(new QQuickAttachedPropertyPropagatorPrivate), parent)
{
    Q_D(QQuickAttachedPropertyPropagator);
    d->attachTo(parent);
}

/*!
    Destroys the QQuickAttachedPropertyPropagator.
*/
QQuickAttachedPropertyPropagator::~QQuickAttachedPropertyPropagator()
{
    Q_D(QQuickAttachedPropertyPropagator);
    d->detachFrom(parent());
    d->setAttachedParent(nullptr);
}

/*!
    This function returns the attached children of this attached object.

    The attached children are used when propagating property values:

    \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
    \skipto MyStyle::propagateTheme
    \printuntil }
    \printuntil }
*/
QList<QQuickAttachedPropertyPropagator *> QQuickAttachedPropertyPropagator::attachedChildren() const
{
    Q_D(const QQuickAttachedPropertyPropagator);
    return d->attachedChildren;
}

/*!
    This function returns the attached parent of this attached object.

    The attached parent is used when inheriting property values:

    \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
    \skipto MyStyle::resetTheme
    \printuntil }
*/
QQuickAttachedPropertyPropagator *QQuickAttachedPropertyPropagator::attachedParent() const
{
    Q_D(const QQuickAttachedPropertyPropagator);
    return d->attachedParent;
}

/*!
    Finds and sets the attached parent for this attached object, and then does
    the same for its children. This must be called upon construction of the
    attached object in order for propagation to work.

    It can be useful to read global/default values before calling this
    function. For example, before calling \c initialize(), the
    \l {Imagine Style}{Imagine} style checks a static "globalsInitialized" flag
    to see if it should read default values from \l QSettings. The values from
    that file form the basis for any attached property values that have not
    been explicitly set.

    \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
    \skipto MyStyle::MyStyle
    \printuntil }
*/
void QQuickAttachedPropertyPropagator::initialize()
{
    Q_D(QQuickAttachedPropertyPropagator);
    qCDebug(lcAttached) << "initialize called for" << parent() << "- looking for attached parent...";
    QQuickAttachedPropertyPropagator *attachedParent = findAttachedParent(metaObject(), parent());
    if (attachedParent)
        d->setAttachedParent(attachedParent);

    const QList<QQuickAttachedPropertyPropagator *> attachedChildren = findAttachedChildren(metaObject(), parent());
    qCDebug(lcAttached) << "- found" << attachedChildren.size() << "attached children:";
    for (QQuickAttachedPropertyPropagator *child : attachedChildren) {
        qCDebug(lcAttached) << "  -" << child->parent();
        QQuickAttachedPropertyPropagatorPrivate::get(child)->setAttachedParent(this);
    }

    qCDebug(lcAttached) << "... finished initializing";
}

/*!
    This function is called whenever the attached parent of this
    QQuickAttachedPropertyPropagator changes from \a oldParent to \a newParent.

    Subclasses should reimplement this function to inherit attached properties
    from \c newParent.

    \quotefromfile ../../examples/quickcontrols/attachedstyleproperties/MyStyle/mystyle.cpp
    \skipto attachedParentChange
    \printuntil }
    \printuntil }
*/
void QQuickAttachedPropertyPropagator::attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent)
{
    Q_UNUSED(newParent);
    Q_UNUSED(oldParent);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QQuickAttachedPropertyPropagator *propagator)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote();
    if (!propagator) {
        debug << "QQuickAttachedPropertyPropagator(nullptr)";
        return debug;
    }

    // Cast to QObject to avoid recursion.
    debug << static_cast<const QObject *>(propagator) << " (which is attached to " << propagator->parent() << ')';
    return debug;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickattachedpropertypropagator.cpp"
