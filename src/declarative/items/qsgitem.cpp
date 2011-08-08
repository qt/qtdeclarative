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

#include "qsgitem.h"

#include "qsgcanvas.h"
#include <QtDeclarative/qjsengine.h>
#include "qsgcanvas_p.h"
#include "qsgevent.h"

#include "qsgevents_p_p.h"

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qgraphicstransform.h>
#include <QtGui/qpen.h>
#include <QtGui/qinputcontext.h>
#include <QtCore/qdebug.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qnumeric.h>

#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativestategroup_p.h>
#include <private/qdeclarativeopenmetaobject_p.h>
#include <private/qdeclarativestate_p.h>
#include <private/qlistmodelinterface_p.h>
#include <private/qsgitem_p.h>

#include <float.h>

// XXX todo Readd parentNotifier for faster parent bindings
// XXX todo Check that elements that create items handle memory correctly after visual ownership change

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Transform QSGTransform
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Transform elements provide a way of building advanced transformations on Items.

    The Transform element is a base type which cannot be instantiated directly.
    The following concrete Transform types are available:

    \list
    \o \l Rotation
    \o \l Scale
    \o \l Translate
    \endlist

    The Transform elements let you create and control advanced transformations that can be configured
    independently using specialized properties.

    You can assign any number of Transform elements to an \l Item. Each Transform is applied in order,
    one at a time.
*/

/*!
    \qmlclass Translate QSGTranslate
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Translate object provides a way to move an Item without changing its x or y properties.

    The Translate object provides independent control over position in addition to the Item's x and y properties.

    The following example moves the Y axis of the \l Rectangle elements while still allowing the \l Row element
    to lay the items out as if they had not been transformed:
    \qml
    import QtQuick 1.0

    Row {
        Rectangle {
            width: 100; height: 100
            color: "blue"
            transform: Translate { y: 20 }
        }
        Rectangle {
            width: 100; height: 100
            color: "red"
            transform: Translate { y: -20 }
        }
    }
    \endqml

    \image translate.png
*/

/*!
    \qmlproperty real QtQuick2::Translate::x

    The translation along the X axis.
*/

/*!
    \qmlproperty real QtQuick2::Translate::y

    The translation along the Y axis.
*/

/*!
    \qmlclass Scale QSGScale
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Scale element provides a way to scale an Item.

    The Scale element gives more control over scaling than using \l Item's \l{Item::scale}{scale} property. Specifically,
    it allows a different scale for the x and y axes, and allows the scale to be relative to an
    arbitrary point.

    The following example scales the X axis of the Rectangle, relative to its interior point 25, 25:
    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Scale { origin.x: 25; origin.y: 25; xScale: 3}
    }
    \endqml

    \sa Rotation, Translate
*/

/*!
    \qmlproperty real QtQuick2::Scale::origin.x
    \qmlproperty real QtQuick2::Scale::origin.y

    The point that the item is scaled from (i.e., the point that stays fixed relative to the parent as
    the rest of the item grows). By default the origin is 0, 0.
*/

/*!
    \qmlproperty real QtQuick2::Scale::xScale

    The scaling factor for the X axis.
*/

/*!
    \qmlproperty real QtQuick2::Scale::yScale

    The scaling factor for the Y axis.
*/

/*!
    \qmlclass Rotation QSGRotation
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Rotation object provides a way to rotate an Item.

    The Rotation object gives more control over rotation than using \l Item's \l{Item::rotation}{rotation} property.
    Specifically, it allows (z axis) rotation to be relative to an arbitrary point.

    The following example rotates a Rectangle around its interior point 25, 25:
    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Rotation { origin.x: 25; origin.y: 25; angle: 45}
    }
    \endqml

    Rotation also provides a way to specify 3D-like rotations for Items. For these types of
    rotations you must specify the axis to rotate around in addition to the origin point.

    The following example shows various 3D-like rotations applied to an \l Image.
    \snippet doc/src/snippets/declarative/rotation.qml 0

    \image axisrotation.png

    \sa {declarative/ui-components/dialcontrol}{Dial Control example}, {declarative/toys/clocks}{Clocks example}
*/

/*!
    \qmlproperty real QtQuick2::Rotation::origin.x
    \qmlproperty real QtQuick2::Rotation::origin.y

    The origin point of the rotation (i.e., the point that stays fixed relative to the parent as
    the rest of the item rotates). By default the origin is 0, 0.
*/

/*!
    \qmlproperty real QtQuick2::Rotation::axis.x
    \qmlproperty real QtQuick2::Rotation::axis.y
    \qmlproperty real QtQuick2::Rotation::axis.z

    The axis to rotate around. For simple (2D) rotation around a point, you do not need to specify an axis,
    as the default axis is the z axis (\c{ axis { x: 0; y: 0; z: 1 } }).

    For a typical 3D-like rotation you will usually specify both the origin and the axis.

    \image 3d-rotation-axis.png
*/

/*!
    \qmlproperty real QtQuick2::Rotation::angle

    The angle to rotate, in degrees clockwise.
*/

QSGTransformPrivate::QSGTransformPrivate()
{
}

QSGTransform::QSGTransform(QObject *parent)
: QObject(*(new QSGTransformPrivate), parent)
{
}

QSGTransform::QSGTransform(QSGTransformPrivate &dd, QObject *parent)
: QObject(dd, parent)
{
}

QSGTransform::~QSGTransform()
{
    Q_D(QSGTransform);
    for (int ii = 0; ii < d->items.count(); ++ii) {
        QSGItemPrivate *p = QSGItemPrivate::get(d->items.at(ii));
        p->transforms.removeOne(this);
        p->dirty(QSGItemPrivate::Transform);
    }
}

void QSGTransform::update()
{
    Q_D(QSGTransform);
    for (int ii = 0; ii < d->items.count(); ++ii) {
        QSGItemPrivate *p = QSGItemPrivate::get(d->items.at(ii));
        p->dirty(QSGItemPrivate::Transform);
    }
}

QSGContents::QSGContents(QSGItem *item)
: m_item(item), m_x(0), m_y(0), m_width(0), m_height(0)
{
    //### optimize
    connect(this, SIGNAL(rectChanged(QRectF)), m_item, SIGNAL(childrenRectChanged(QRectF)));
}

QSGContents::~QSGContents()
{
    QList<QSGItem *> children = m_item->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QSGItem *child = children.at(i);
        QSGItemPrivate::get(child)->removeItemChangeListener(this, QSGItemPrivate::Geometry | QSGItemPrivate::Destroyed);
    }
}

QRectF QSGContents::rectF() const
{
    return QRectF(m_x, m_y, m_width, m_height);
}

void QSGContents::calcHeight(QSGItem *changed)
{
    qreal oldy = m_y;
    qreal oldheight = m_height;

    if (changed) {
        qreal top = oldy;
        qreal bottom = oldy + oldheight;
        qreal y = changed->y();
        if (y + changed->height() > bottom)
            bottom = y + changed->height();
        if (y < top)
            top = y;
        m_y = top;
        m_height = bottom - top;
    } else {
        qreal top = FLT_MAX;
        qreal bottom = 0;
        QList<QSGItem *> children = m_item->childItems();
        for (int i = 0; i < children.count(); ++i) {
            QSGItem *child = children.at(i);
            qreal y = child->y();
            if (y + child->height() > bottom)
                bottom = y + child->height();
            if (y < top)
                top = y;
        }
        if (!children.isEmpty())
            m_y = top;
        m_height = qMax(bottom - top, qreal(0.0));
    }

    if (m_height != oldheight || m_y != oldy)
        emit rectChanged(rectF());
}

void QSGContents::calcWidth(QSGItem *changed)
{
    qreal oldx = m_x;
    qreal oldwidth = m_width;

    if (changed) {
        qreal left = oldx;
        qreal right = oldx + oldwidth;
        qreal x = changed->x();
        if (x + changed->width() > right)
            right = x + changed->width();
        if (x < left)
            left = x;
        m_x = left;
        m_width = right - left;
    } else {
        qreal left = FLT_MAX;
        qreal right = 0;
        QList<QSGItem *> children = m_item->childItems();
        for (int i = 0; i < children.count(); ++i) {
            QSGItem *child = children.at(i);
            qreal x = child->x();
            if (x + child->width() > right)
                right = x + child->width();
            if (x < left)
                left = x;
        }
        if (!children.isEmpty())
            m_x = left;
        m_width = qMax(right - left, qreal(0.0));
    }

    if (m_width != oldwidth || m_x != oldx)
        emit rectChanged(rectF());
}

void QSGContents::complete()
{
    QList<QSGItem *> children = m_item->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QSGItem *child = children.at(i);
        QSGItemPrivate::get(child)->addItemChangeListener(this, QSGItemPrivate::Geometry | QSGItemPrivate::Destroyed);
        //###what about changes to visibility?
    }

    calcGeometry();
}

void QSGContents::itemGeometryChanged(QSGItem *changed, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(changed)
    //### we can only pass changed if the left edge has moved left, or the right edge has moved right
    if (newGeometry.width() != oldGeometry.width() || newGeometry.x() != oldGeometry.x())
        calcWidth(/*changed*/);
    if (newGeometry.height() != oldGeometry.height() || newGeometry.y() != oldGeometry.y())
        calcHeight(/*changed*/);
}

void QSGContents::itemDestroyed(QSGItem *item)
{
    if (item)
        QSGItemPrivate::get(item)->removeItemChangeListener(this, QSGItemPrivate::Geometry | QSGItemPrivate::Destroyed);
    calcGeometry();
}

void QSGContents::childRemoved(QSGItem *item)
{
    if (item)
        QSGItemPrivate::get(item)->removeItemChangeListener(this, QSGItemPrivate::Geometry | QSGItemPrivate::Destroyed);
    calcGeometry();
}

void QSGContents::childAdded(QSGItem *item)
{
    if (item)
        QSGItemPrivate::get(item)->addItemChangeListener(this, QSGItemPrivate::Geometry | QSGItemPrivate::Destroyed);
    calcWidth(item);
    calcHeight(item);
}

QSGItemKeyFilter::QSGItemKeyFilter(QSGItem *item)
: m_processPost(false), m_next(0)
{
    QSGItemPrivate *p = item?QSGItemPrivate::get(item):0;
    if (p) {
        m_next = p->keyHandler;
        p->keyHandler = this;
    }
}

QSGItemKeyFilter::~QSGItemKeyFilter()
{
}

void QSGItemKeyFilter::keyPressed(QKeyEvent *event, bool post)
{
    if (m_next) m_next->keyPressed(event, post);
}

void QSGItemKeyFilter::keyReleased(QKeyEvent *event, bool post)
{
    if (m_next) m_next->keyReleased(event, post);
}

void QSGItemKeyFilter::inputMethodEvent(QInputMethodEvent *event, bool post)
{
    if (m_next)
        m_next->inputMethodEvent(event, post);
    else
        event->ignore();
}

QVariant QSGItemKeyFilter::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (m_next) return m_next->inputMethodQuery(query);
    return QVariant();
}

void QSGItemKeyFilter::componentComplete()
{
    if (m_next) m_next->componentComplete();
}
/*!
    \qmlclass KeyNavigation QSGKeyNavigationAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-interaction-elements
    \brief The KeyNavigation attached property supports key navigation by arrow keys.

    Key-based user interfaces commonly allow the use of arrow keys to navigate between
    focusable items.  The KeyNavigation attached property enables this behavior by providing a
    convenient way to specify the item that should gain focus when an arrow or tab key is pressed.

    The following example provides key navigation for a 2x2 grid of items:

    \snippet doc/src/snippets/declarative/keynavigation.qml 0

    The top-left item initially receives focus by setting \l {Item::}{focus} to
    \c true. When an arrow key is pressed, the focus will move to the
    appropriate item, as defined by the value that has been set for
    the KeyNavigation \l left, \l right, \l up or \l down properties.

    Note that if a KeyNavigation attached property receives the key press and release
    events for a requested arrow or tab key, the event is accepted and does not
    propagate any further.

    By default, KeyNavigation receives key events after the item to which it is attached.
    If the item accepts the key event, the KeyNavigation attached property will not
    receive an event for that key.  Setting the \l priority property to
    \c KeyNavigation.BeforeItem allows the event to be used for key navigation
    before the item, rather than after.

    If item to which the focus is switching is not enabled or visible, an attempt will
    be made to skip this item and focus on the next. This is possible if there are
    a chain of items with the same KeyNavigation handler. If multiple items in a row are not enabled
    or visible, they will also be skipped.

    KeyNavigation will implicitly set the other direction to return focus to this item. So if you set
    \l left to another item, \l right will be set on that item's KeyNavigation to set focus back to this
    item. However, if that item's KeyNavigation has had right explicitly set then no change will occur.
    This means that the above example could have been written, with the same behaviour, without specifing
    KeyNavigation.right or KeyNavigation.down for any of the items.

    \sa {Keys}{Keys attached property}
*/

/*!
    \qmlproperty Item QtQuick2::KeyNavigation::left
    \qmlproperty Item QtQuick2::KeyNavigation::right
    \qmlproperty Item QtQuick2::KeyNavigation::up
    \qmlproperty Item QtQuick2::KeyNavigation::down
    \qmlproperty Item QtQuick2::KeyNavigation::tab
    \qmlproperty Item QtQuick2::KeyNavigation::backtab

    These properties hold the item to assign focus to
    when the left, right, up or down cursor keys, or the
    tab key are pressed.
*/

/*!
    \qmlproperty Item QtQuick2::KeyNavigation::tab
    \qmlproperty Item QtQuick2::KeyNavigation::backtab

    These properties hold the item to assign focus to
    when the Tab key or Shift+Tab key combination (Backtab) are pressed.
*/

QSGKeyNavigationAttached::QSGKeyNavigationAttached(QObject *parent)
: QObject(*(new QSGKeyNavigationAttachedPrivate), parent),
  QSGItemKeyFilter(qobject_cast<QSGItem*>(parent))
{
    m_processPost = true;
}

QSGKeyNavigationAttached *
QSGKeyNavigationAttached::qmlAttachedProperties(QObject *obj)
{
    return new QSGKeyNavigationAttached(obj);
}

QSGItem *QSGKeyNavigationAttached::left() const
{
    Q_D(const QSGKeyNavigationAttached);
    return d->left;
}

void QSGKeyNavigationAttached::setLeft(QSGItem *i)
{
    Q_D(QSGKeyNavigationAttached);
    if (d->left == i)
        return;
    d->left = i;
    d->leftSet = true;
    QSGKeyNavigationAttached* other =
            qobject_cast<QSGKeyNavigationAttached*>(qmlAttachedPropertiesObject<QSGKeyNavigationAttached>(i));
    if (other && !other->d_func()->rightSet){
        other->d_func()->right = qobject_cast<QSGItem*>(parent());
        emit other->rightChanged();
    }
    emit leftChanged();
}

QSGItem *QSGKeyNavigationAttached::right() const
{
    Q_D(const QSGKeyNavigationAttached);
    return d->right;
}

void QSGKeyNavigationAttached::setRight(QSGItem *i)
{
    Q_D(QSGKeyNavigationAttached);
    if (d->right == i)
        return;
    d->right = i;
    d->rightSet = true;
    QSGKeyNavigationAttached* other =
            qobject_cast<QSGKeyNavigationAttached*>(qmlAttachedPropertiesObject<QSGKeyNavigationAttached>(i));
    if (other && !other->d_func()->leftSet){
        other->d_func()->left = qobject_cast<QSGItem*>(parent());
        emit other->leftChanged();
    }
    emit rightChanged();
}

QSGItem *QSGKeyNavigationAttached::up() const
{
    Q_D(const QSGKeyNavigationAttached);
    return d->up;
}

void QSGKeyNavigationAttached::setUp(QSGItem *i)
{
    Q_D(QSGKeyNavigationAttached);
    if (d->up == i)
        return;
    d->up = i;
    d->upSet = true;
    QSGKeyNavigationAttached* other =
            qobject_cast<QSGKeyNavigationAttached*>(qmlAttachedPropertiesObject<QSGKeyNavigationAttached>(i));
    if (other && !other->d_func()->downSet){
        other->d_func()->down = qobject_cast<QSGItem*>(parent());
        emit other->downChanged();
    }
    emit upChanged();
}

QSGItem *QSGKeyNavigationAttached::down() const
{
    Q_D(const QSGKeyNavigationAttached);
    return d->down;
}

void QSGKeyNavigationAttached::setDown(QSGItem *i)
{
    Q_D(QSGKeyNavigationAttached);
    if (d->down == i)
        return;
    d->down = i;
    d->downSet = true;
    QSGKeyNavigationAttached* other =
            qobject_cast<QSGKeyNavigationAttached*>(qmlAttachedPropertiesObject<QSGKeyNavigationAttached>(i));
    if(other && !other->d_func()->upSet){
        other->d_func()->up = qobject_cast<QSGItem*>(parent());
        emit other->upChanged();
    }
    emit downChanged();
}

QSGItem *QSGKeyNavigationAttached::tab() const
{
    Q_D(const QSGKeyNavigationAttached);
    return d->tab;
}

void QSGKeyNavigationAttached::setTab(QSGItem *i)
{
    Q_D(QSGKeyNavigationAttached);
    if (d->tab == i)
        return;
    d->tab = i;
    d->tabSet = true;
    QSGKeyNavigationAttached* other =
            qobject_cast<QSGKeyNavigationAttached*>(qmlAttachedPropertiesObject<QSGKeyNavigationAttached>(i));
    if(other && !other->d_func()->backtabSet){
        other->d_func()->backtab = qobject_cast<QSGItem*>(parent());
        emit other->backtabChanged();
    }
    emit tabChanged();
}

QSGItem *QSGKeyNavigationAttached::backtab() const
{
    Q_D(const QSGKeyNavigationAttached);
    return d->backtab;
}

void QSGKeyNavigationAttached::setBacktab(QSGItem *i)
{
    Q_D(QSGKeyNavigationAttached);
    if (d->backtab == i)
        return;
    d->backtab = i;
    d->backtabSet = true;
    QSGKeyNavigationAttached* other =
            qobject_cast<QSGKeyNavigationAttached*>(qmlAttachedPropertiesObject<QSGKeyNavigationAttached>(i));
    if(other && !other->d_func()->tabSet){
        other->d_func()->tab = qobject_cast<QSGItem*>(parent());
        emit other->tabChanged();
    }
    emit backtabChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::KeyNavigation::priority

    This property determines whether the keys are processed before
    or after the attached item's own key handling.

    \list
    \o KeyNavigation.BeforeItem - process the key events before normal
    item key processing.  If the event is used for key navigation, it will be accepted and will not
    be passed on to the item.
    \o KeyNavigation.AfterItem (default) - process the key events after normal item key
    handling.  If the item accepts the key event it will not be
    handled by the KeyNavigation attached property handler.
    \endlist
*/
QSGKeyNavigationAttached::Priority QSGKeyNavigationAttached::priority() const
{
    return m_processPost ? AfterItem : BeforeItem;
}

void QSGKeyNavigationAttached::setPriority(Priority order)
{
    bool processPost = order == AfterItem;
    if (processPost != m_processPost) {
        m_processPost = processPost;
        emit priorityChanged();
    }
}

void QSGKeyNavigationAttached::keyPressed(QKeyEvent *event, bool post)
{
    Q_D(QSGKeyNavigationAttached);
    event->ignore();

    if (post != m_processPost) {
        QSGItemKeyFilter::keyPressed(event, post);
        return;
    }

    bool mirror = false;
    switch(event->key()) {
    case Qt::Key_Left: {
        if (QSGItem *parentItem = qobject_cast<QSGItem*>(parent()))
            mirror = QSGItemPrivate::get(parentItem)->effectiveLayoutMirror;
        QSGItem* leftItem = mirror ? d->right : d->left;
        if (leftItem) {
            setFocusNavigation(leftItem, mirror ? "right" : "left");
            event->accept();
        }
        break;
    }
    case Qt::Key_Right: {
        if (QSGItem *parentItem = qobject_cast<QSGItem*>(parent()))
            mirror = QSGItemPrivate::get(parentItem)->effectiveLayoutMirror;
        QSGItem* rightItem = mirror ? d->left : d->right;
        if (rightItem) {
            setFocusNavigation(rightItem, mirror ? "left" : "right");
            event->accept();
        }
        break;
    }
    case Qt::Key_Up:
        if (d->up) {
            setFocusNavigation(d->up, "up");
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            setFocusNavigation(d->down, "down");
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            setFocusNavigation(d->tab, "tab");
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            setFocusNavigation(d->backtab, "backtab");
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QSGItemKeyFilter::keyPressed(event, post);
}

void QSGKeyNavigationAttached::keyReleased(QKeyEvent *event, bool post)
{
    Q_D(QSGKeyNavigationAttached);
    event->ignore();

    if (post != m_processPost) {
        QSGItemKeyFilter::keyReleased(event, post);
        return;
    }

    bool mirror = false;
    switch(event->key()) {
    case Qt::Key_Left:
        if (QSGItem *parentItem = qobject_cast<QSGItem*>(parent()))
            mirror = QSGItemPrivate::get(parentItem)->effectiveLayoutMirror;
        if (mirror ? d->right : d->left)
            event->accept();
        break;
    case Qt::Key_Right:
        if (QSGItem *parentItem = qobject_cast<QSGItem*>(parent()))
            mirror = QSGItemPrivate::get(parentItem)->effectiveLayoutMirror;
        if (mirror ? d->left : d->right)
            event->accept();
        break;
    case Qt::Key_Up:
        if (d->up) {
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QSGItemKeyFilter::keyReleased(event, post);
}

void QSGKeyNavigationAttached::setFocusNavigation(QSGItem *currentItem, const char *dir)
{
    QSGItem *initialItem = currentItem;
    bool isNextItem = false;
    do {
        isNextItem = false;
        if (currentItem->isVisible() && currentItem->isEnabled()) {
            currentItem->setFocus(true);
        } else {
            QObject *attached =
                qmlAttachedPropertiesObject<QSGKeyNavigationAttached>(currentItem, false);
            if (attached) {
                QSGItem *tempItem = qvariant_cast<QSGItem*>(attached->property(dir));
                if (tempItem) {
                    currentItem = tempItem;
                    isNextItem = true;
                }
            }
        }
    }
    while (currentItem != initialItem && isNextItem);
}

const QSGKeysAttached::SigMap QSGKeysAttached::sigMap[] = {
    { Qt::Key_Left, "leftPressed" },
    { Qt::Key_Right, "rightPressed" },
    { Qt::Key_Up, "upPressed" },
    { Qt::Key_Down, "downPressed" },
    { Qt::Key_Tab, "tabPressed" },
    { Qt::Key_Backtab, "backtabPressed" },
    { Qt::Key_Asterisk, "asteriskPressed" },
    { Qt::Key_NumberSign, "numberSignPressed" },
    { Qt::Key_Escape, "escapePressed" },
    { Qt::Key_Return, "returnPressed" },
    { Qt::Key_Enter, "enterPressed" },
    { Qt::Key_Delete, "deletePressed" },
    { Qt::Key_Space, "spacePressed" },
    { Qt::Key_Back, "backPressed" },
    { Qt::Key_Cancel, "cancelPressed" },
    { Qt::Key_Select, "selectPressed" },
    { Qt::Key_Yes, "yesPressed" },
    { Qt::Key_No, "noPressed" },
    { Qt::Key_Context1, "context1Pressed" },
    { Qt::Key_Context2, "context2Pressed" },
    { Qt::Key_Context3, "context3Pressed" },
    { Qt::Key_Context4, "context4Pressed" },
    { Qt::Key_Call, "callPressed" },
    { Qt::Key_Hangup, "hangupPressed" },
    { Qt::Key_Flip, "flipPressed" },
    { Qt::Key_Menu, "menuPressed" },
    { Qt::Key_VolumeUp, "volumeUpPressed" },
    { Qt::Key_VolumeDown, "volumeDownPressed" },
    { 0, 0 }
};

bool QSGKeysAttachedPrivate::isConnected(const char *signalName)
{
    return isSignalConnected(signalIndex(signalName));
}

/*!
    \qmlclass Keys QSGKeysAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-interaction-elements
    \brief The Keys attached property provides key handling to Items.

    All visual primitives support key handling via the Keys
    attached property.  Keys can be handled via the onPressed
    and onReleased signal properties.

    The signal properties have a \l KeyEvent parameter, named
    \e event which contains details of the event.  If a key is
    handled \e event.accepted should be set to true to prevent the
    event from propagating up the item hierarchy.

    \section1 Example Usage

    The following example shows how the general onPressed handler can
    be used to test for a certain key; in this case, the left cursor
    key:

    \snippet doc/src/snippets/declarative/keys/keys-pressed.qml key item

    Some keys may alternatively be handled via specific signal properties,
    for example \e onSelectPressed.  These handlers automatically set
    \e event.accepted to true.

    \snippet doc/src/snippets/declarative/keys/keys-handler.qml key item

    See \l{Qt::Key}{Qt.Key} for the list of keyboard codes.

    \section1 Key Handling Priorities

    The Keys attached property can be configured to handle key events
    before or after the item it is attached to. This makes it possible
    to intercept events in order to override an item's default behavior,
    or act as a fallback for keys not handled by the item.

    If \l priority is Keys.BeforeItem (default) the order of key event processing is:

    \list 1
    \o Items specified in \c forwardTo
    \o specific key handlers, e.g. onReturnPressed
    \o onKeyPress, onKeyRelease handlers
    \o Item specific key handling, e.g. TextInput key handling
    \o parent item
    \endlist

    If priority is Keys.AfterItem the order of key event processing is:

    \list 1
    \o Item specific key handling, e.g. TextInput key handling
    \o Items specified in \c forwardTo
    \o specific key handlers, e.g. onReturnPressed
    \o onKeyPress, onKeyRelease handlers
    \o parent item
    \endlist

    If the event is accepted during any of the above steps, key
    propagation stops.

    \sa KeyEvent, {KeyNavigation}{KeyNavigation attached property}
*/

/*!
    \qmlproperty bool QtQuick2::Keys::enabled

    This flags enables key handling if true (default); otherwise
    no key handlers will be called.
*/

/*!
    \qmlproperty enumeration QtQuick2::Keys::priority

    This property determines whether the keys are processed before
    or after the attached item's own key handling.

    \list
    \o Keys.BeforeItem (default) - process the key events before normal
    item key processing.  If the event is accepted it will not
    be passed on to the item.
    \o Keys.AfterItem - process the key events after normal item key
    handling.  If the item accepts the key event it will not be
    handled by the Keys attached property handler.
    \endlist
*/

/*!
    \qmlproperty list<Object> QtQuick2::Keys::forwardTo

    This property provides a way to forward key presses, key releases, and keyboard input
    coming from input methods to other items. This can be useful when you want
    one item to handle some keys (e.g. the up and down arrow keys), and another item to
    handle other keys (e.g. the left and right arrow keys).  Once an item that has been
    forwarded keys accepts the event it is no longer forwarded to items later in the
    list.

    This example forwards key events to two lists:
    \qml
    Item {
        ListView {
            id: list1
            // ...
        }
        ListView {
            id: list2
            // ...
        }
        Keys.forwardTo: [list1, list2]
        focus: true
    }
    \endqml
*/

/*!
    \qmlsignal QtQuick2::Keys::onPressed(KeyEvent event)

    This handler is called when a key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onReleased(KeyEvent event)

    This handler is called when a key has been released. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit0Pressed(KeyEvent event)

    This handler is called when the digit '0' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit1Pressed(KeyEvent event)

    This handler is called when the digit '1' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit2Pressed(KeyEvent event)

    This handler is called when the digit '2' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit3Pressed(KeyEvent event)

    This handler is called when the digit '3' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit4Pressed(KeyEvent event)

    This handler is called when the digit '4' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit5Pressed(KeyEvent event)

    This handler is called when the digit '5' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit6Pressed(KeyEvent event)

    This handler is called when the digit '6' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit7Pressed(KeyEvent event)

    This handler is called when the digit '7' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit8Pressed(KeyEvent event)

    This handler is called when the digit '8' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit9Pressed(KeyEvent event)

    This handler is called when the digit '9' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onLeftPressed(KeyEvent event)

    This handler is called when the Left arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onRightPressed(KeyEvent event)

    This handler is called when the Right arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onUpPressed(KeyEvent event)

    This handler is called when the Up arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDownPressed(KeyEvent event)

    This handler is called when the Down arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onTabPressed(KeyEvent event)

    This handler is called when the Tab key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onBacktabPressed(KeyEvent event)

    This handler is called when the Shift+Tab key combination (Backtab) has
    been pressed. The \a event parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onAsteriskPressed(KeyEvent event)

    This handler is called when the Asterisk '*' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onEscapePressed(KeyEvent event)

    This handler is called when the Escape key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onReturnPressed(KeyEvent event)

    This handler is called when the Return key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onEnterPressed(KeyEvent event)

    This handler is called when the Enter key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDeletePressed(KeyEvent event)

    This handler is called when the Delete key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onSpacePressed(KeyEvent event)

    This handler is called when the Space key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onBackPressed(KeyEvent event)

    This handler is called when the Back key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onCancelPressed(KeyEvent event)

    This handler is called when the Cancel key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onSelectPressed(KeyEvent event)

    This handler is called when the Select key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onYesPressed(KeyEvent event)

    This handler is called when the Yes key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onNoPressed(KeyEvent event)

    This handler is called when the No key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext1Pressed(KeyEvent event)

    This handler is called when the Context1 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext2Pressed(KeyEvent event)

    This handler is called when the Context2 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext3Pressed(KeyEvent event)

    This handler is called when the Context3 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext4Pressed(KeyEvent event)

    This handler is called when the Context4 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onCallPressed(KeyEvent event)

    This handler is called when the Call key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onHangupPressed(KeyEvent event)

    This handler is called when the Hangup key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onFlipPressed(KeyEvent event)

    This handler is called when the Flip key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onMenuPressed(KeyEvent event)

    This handler is called when the Menu key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onVolumeUpPressed(KeyEvent event)

    This handler is called when the VolumeUp key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onVolumeDownPressed(KeyEvent event)

    This handler is called when the VolumeDown key has been pressed. The \a event
    parameter provides information about the event.
*/

QSGKeysAttached::QSGKeysAttached(QObject *parent)
: QObject(*(new QSGKeysAttachedPrivate), parent),
  QSGItemKeyFilter(qobject_cast<QSGItem*>(parent))
{
    Q_D(QSGKeysAttached);
    m_processPost = false;
    d->item = qobject_cast<QSGItem*>(parent);
}

QSGKeysAttached::~QSGKeysAttached()
{
}

QSGKeysAttached::Priority QSGKeysAttached::priority() const
{
    return m_processPost ? AfterItem : BeforeItem;
}

void QSGKeysAttached::setPriority(Priority order)
{
    bool processPost = order == AfterItem;
    if (processPost != m_processPost) {
        m_processPost = processPost;
        emit priorityChanged();
    }
}

void QSGKeysAttached::componentComplete()
{
    Q_D(QSGKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QSGItem *targetItem = d->targets.at(ii);
            if (targetItem && (targetItem->flags() & QSGItem::ItemAcceptsInputMethod)) {
                d->item->setFlag(QSGItem::ItemAcceptsInputMethod);
                break;
            }
        }
    }
}

void QSGKeysAttached::keyPressed(QKeyEvent *event, bool post)
{
    Q_D(QSGKeysAttached);
    if (post != m_processPost || !d->enabled || d->inPress) {
        event->ignore();
        QSGItemKeyFilter::keyPressed(event, post);
        return;
    }

    // first process forwards
    if (d->item && d->item->canvas()) {
        d->inPress = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QSGItem *i = d->targets.at(ii);
            if (i && i->isVisible()) {
                d->item->canvas()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inPress = false;
                    return;
                }
            }
        }
        d->inPress = false;
    }

    QSGKeyEvent ke(*event);
    QByteArray keySignal = keyToSignal(event->key());
    if (!keySignal.isEmpty()) {
        keySignal += "(QSGKeyEvent*)";
        if (d->isConnected(keySignal)) {
            // If we specifically handle a key then default to accepted
            ke.setAccepted(true);
            int idx = QSGKeysAttached::staticMetaObject.indexOfSignal(keySignal);
            metaObject()->method(idx).invoke(this, Qt::DirectConnection, Q_ARG(QSGKeyEvent*, &ke));
        }
    }
    if (!ke.isAccepted())
        emit pressed(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QSGItemKeyFilter::keyPressed(event, post);
}

void QSGKeysAttached::keyReleased(QKeyEvent *event, bool post)
{
    Q_D(QSGKeysAttached);
    if (post != m_processPost || !d->enabled || d->inRelease) {
        event->ignore();
        QSGItemKeyFilter::keyReleased(event, post);
        return;
    }

    if (d->item && d->item->canvas()) {
        d->inRelease = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QSGItem *i = d->targets.at(ii);
            if (i && i->isVisible()) {
                d->item->canvas()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inRelease = false;
                    return;
                }
            }
        }
        d->inRelease = false;
    }

    QSGKeyEvent ke(*event);
    emit released(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QSGItemKeyFilter::keyReleased(event, post);
}

void QSGKeysAttached::inputMethodEvent(QInputMethodEvent *event, bool post)
{
    Q_D(QSGKeysAttached);
    if (post == m_processPost && d->item && !d->inIM && d->item->canvas()) {
        d->inIM = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QSGItem *i = d->targets.at(ii);
            if (i && i->isVisible() && (i->flags() & QSGItem::ItemAcceptsInputMethod)) {
                d->item->canvas()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->imeItem = i;
                    d->inIM = false;
                    return;
                }
            }
        }
        d->inIM = false;
    }
    QSGItemKeyFilter::inputMethodEvent(event, post);
}

QVariant QSGKeysAttached::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QSGKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QSGItem *i = d->targets.at(ii);
            if (i && i->isVisible() && (i->flags() & QSGItem::ItemAcceptsInputMethod) && i == d->imeItem) {
                //### how robust is i == d->imeItem check?
                QVariant v = i->inputMethodQuery(query);
                if (v.userType() == QVariant::RectF)
                    v = d->item->mapRectFromItem(i, v.toRectF());  //### cost?
                return v;
            }
        }
    }
    return QSGItemKeyFilter::inputMethodQuery(query);
}

QSGKeysAttached *QSGKeysAttached::qmlAttachedProperties(QObject *obj)
{
    return new QSGKeysAttached(obj);
}

/*!
    \qmlclass LayoutMirroring QSGLayoutMirroringAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-utility-elements
    \brief The LayoutMirroring attached property is used to mirror layout behavior.

    The LayoutMirroring attached property is used to horizontally mirror \l {anchor-layout}{Item anchors},
    \l{Using QML Positioner and Repeater Items}{positioner} elements (such as \l Row and \l Grid)
    and views (such as \l GridView and horizontal \l ListView). Mirroring is a visual change: left
    anchors become right anchors, and positioner elements like \l Grid and \l Row reverse the
    horizontal layout of child items.

    Mirroring is enabled for an item by setting the \l enabled property to true. By default, this
    only affects the item itself; setting the \l childrenInherit property to true propagates the mirroring
    behavior to all child elements as well. If the \c LayoutMirroring attached property has not been defined
    for an item, mirroring is not enabled.

    The following example shows mirroring in action. The \l Row below is specified as being anchored
    to the left of its parent. However, since mirroring has been enabled, the anchor is horizontally
    reversed and it is now anchored to the right. Also, since items in a \l Row are positioned
    from left to right by default, they are now positioned from right to left instead, as demonstrated
    by the numbering and opacity of the items:

    \snippet doc/src/snippets/declarative/layoutmirroring.qml 0

    \image layoutmirroring.png

    Layout mirroring is useful when it is necessary to support both left-to-right and right-to-left
    layout versions of an application to target different language areas. The \l childrenInherit
    property allows layout mirroring to be applied without manually setting layout configurations
    for every item in an application. Keep in mind, however, that mirroring does not affect any
    positioning that is defined by the \l Item \l {Item::}{x} coordinate value, so even with
    mirroring enabled, it will often be necessary to apply some layout fixes to support the
    desired layout direction. Also, it may be necessary to disable the mirroring of individual
    child items (by setting \l {enabled}{LayoutMirroring.enabled} to false for such items) if
    mirroring is not the desired behavior, or if the child item already implements mirroring in
    some custom way.

    See \l {QML Right-to-left User Interfaces} for further details on using \c LayoutMirroring and
    other related features to implement right-to-left support for an application.
*/

/*!
    \qmlproperty bool QtQuick2::LayoutMirroring::enabled

    This property holds whether the item's layout is mirrored horizontally. Setting this to true
    horizontally reverses \l {anchor-layout}{anchor} settings such that left anchors become right,
    and right anchors become left. For \l{Using QML Positioner and Repeater Items}{positioner} elements
    (such as \l Row and \l Grid) and view elements (such as \l {GridView}{GridView} and \l {ListView}{ListView})
    this also mirrors the horizontal layout direction of the item.

    The default value is false.
*/

/*!
    \qmlproperty bool QtQuick2::LayoutMirroring::childrenInherit

    This property holds whether the \l {enabled}{LayoutMirroring.enabled} value for this item
    is inherited by its children.

    The default value is false.
*/


QSGLayoutMirroringAttached::QSGLayoutMirroringAttached(QObject *parent) : QObject(parent), itemPrivate(0)
{
    if (QSGItem *item = qobject_cast<QSGItem*>(parent)) {
        itemPrivate = QSGItemPrivate::get(item);
        itemPrivate->attachedLayoutDirection = this;
    } else
        qmlInfo(parent) << tr("LayoutDirection attached property only works with Items");
}

QSGLayoutMirroringAttached * QSGLayoutMirroringAttached::qmlAttachedProperties(QObject *object)
{
    return new QSGLayoutMirroringAttached(object);
}

bool QSGLayoutMirroringAttached::enabled() const
{
    return itemPrivate ? itemPrivate->effectiveLayoutMirror : false;
}

void QSGLayoutMirroringAttached::setEnabled(bool enabled)
{
    if (!itemPrivate)
        return;

    itemPrivate->isMirrorImplicit = false;
    if (enabled != itemPrivate->effectiveLayoutMirror) {
        itemPrivate->setLayoutMirror(enabled);
        if (itemPrivate->inheritMirrorFromItem)
             itemPrivate->resolveLayoutMirror();
    }
}

void QSGLayoutMirroringAttached::resetEnabled()
{
    if (itemPrivate && !itemPrivate->isMirrorImplicit) {
        itemPrivate->isMirrorImplicit = true;
        itemPrivate->resolveLayoutMirror();
    }
}

bool QSGLayoutMirroringAttached::childrenInherit() const
{
    return itemPrivate ? itemPrivate->inheritMirrorFromItem : false;
}

void QSGLayoutMirroringAttached::setChildrenInherit(bool childrenInherit) {
    if (itemPrivate && childrenInherit != itemPrivate->inheritMirrorFromItem) {
        itemPrivate->inheritMirrorFromItem = childrenInherit;
        itemPrivate->resolveLayoutMirror();
        childrenInheritChanged();
    }
}

void QSGItemPrivate::resolveLayoutMirror()
{
    Q_Q(QSGItem);
    if (QSGItem *parentItem = q->parentItem()) {
        QSGItemPrivate *parentPrivate = QSGItemPrivate::get(parentItem);
        setImplicitLayoutMirror(parentPrivate->inheritedLayoutMirror, parentPrivate->inheritMirrorFromParent);
    } else {
        setImplicitLayoutMirror(isMirrorImplicit ? false : effectiveLayoutMirror, inheritMirrorFromItem);
    }
}

void QSGItemPrivate::setImplicitLayoutMirror(bool mirror, bool inherit)
{
    inherit = inherit || inheritMirrorFromItem;
    if (!isMirrorImplicit && inheritMirrorFromItem)
        mirror = effectiveLayoutMirror;
    if (mirror == inheritedLayoutMirror && inherit == inheritMirrorFromParent)
        return;

    inheritMirrorFromParent = inherit;
    inheritedLayoutMirror = inheritMirrorFromParent ? mirror : false;

    if (isMirrorImplicit)
        setLayoutMirror(inherit ? inheritedLayoutMirror : false);
    for (int i = 0; i < childItems.count(); ++i) {
        if (QSGItem *child = qobject_cast<QSGItem *>(childItems.at(i))) {
            QSGItemPrivate *childPrivate = QSGItemPrivate::get(child);
            childPrivate->setImplicitLayoutMirror(inheritedLayoutMirror, inheritMirrorFromParent);
        }
    }
}

void QSGItemPrivate::setLayoutMirror(bool mirror)
{
    if (mirror != effectiveLayoutMirror) {
        effectiveLayoutMirror = mirror;
        if (_anchors) {
            QSGAnchorsPrivate *anchor_d = QSGAnchorsPrivate::get(_anchors);
            anchor_d->fillChanged();
            anchor_d->centerInChanged();
            anchor_d->updateHorizontalAnchors();
            emit _anchors->mirroredChanged();
        }
        mirrorChange();
        if (attachedLayoutDirection) {
            emit attachedLayoutDirection->enabledChanged();
        }
    }
}

/*!
    \class QSGItem
    \brief The QSGItem class provides the most basic of all visual items in QML.

    All visual items in Qt Declarative inherit from QSGItem.  Although QSGItem
    has no visual appearance, it defines all the properties that are
    common across visual items - such as the x and y position, the
    width and height, \l {anchor-layout}{anchoring} and key handling.

    You can subclass QSGItem to provide your own custom visual item that inherits
    these features. Note that, because it does not draw anything, QSGItem sets the
    QGraphicsItem::ItemHasNoContents flag. If you subclass QSGItem to create a visual
    item, you will need to unset this flag.

*/

/*!
    \qmlclass Item QSGItem
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements
    \brief The Item is the most basic of all visual items in QML.

    All visual items in Qt Declarative inherit from Item.  Although Item
    has no visual appearance, it defines all the properties that are
    common across visual items - such as the x and y position, the
    width and height, \l {anchor-layout}{anchoring} and key handling.

    Item is also useful for grouping items together.

    \qml
    Item {
        Image {
            source: "tile.png"
        }
        Image {
            x: 80
            width: 100
            height: 100
            source: "tile.png"
        }
        Image {
            x: 190
            width: 100
            height: 100
            fillMode: Image.Tile
            source: "tile.png"
        }
    }
    \endqml


    \section1 Key Handling

    Key handling is available to all Item-based visual elements via the \l {Keys}{Keys}
    attached property.  The \e Keys attached property provides basic handlers such
    as \l {Keys::onPressed}{onPressed} and \l {Keys::onReleased}{onReleased},
    as well as handlers for specific keys, such as
    \l {Keys::onCancelPressed}{onCancelPressed}.  The example below
    assigns \l {qmlfocus}{focus} to the item and handles
    the Left key via the general \e onPressed handler and the Select key via the
    onSelectPressed handler:

    \qml
    Item {
        focus: true
        Keys.onPressed: {
            if (event.key == Qt.Key_Left) {
                console.log("move left");
                event.accepted = true;
            }
        }
        Keys.onSelectPressed: console.log("Selected");
    }
    \endqml

    See the \l {Keys}{Keys} attached property for detailed documentation.

    \section1 Layout Mirroring

    Item layouts can be mirrored using the \l {LayoutMirroring}{LayoutMirroring} attached property.

*/

/*!
    \fn void QSGItem::childrenRectChanged(const QRectF &)
    \internal
*/

/*!
    \fn void QSGItem::baselineOffsetChanged(qreal)
    \internal
*/

/*!
    \fn void QSGItem::stateChanged(const QString &state)
    \internal
*/

/*!
    \fn void QSGItem::parentChanged(QSGItem *)
    \internal
*/

/*!
    \fn void QSGItem::smoothChanged(bool)
    \internal
*/

/*!
    \fn void QSGItem::clipChanged(bool)
    \internal
*/

/*! \fn void QSGItem::transformOriginChanged(TransformOrigin)
  \internal
*/

/*!
    \fn void QSGItem::focusChanged(bool)
    \internal
*/

/*!
    \fn void QSGItem::activeFocusChanged(bool)
    \internal
*/
/*!
    \fn QSGItem::QSGItem(QSGItem *parent)

    Constructs a QSGItem with the given \a parent.
*/
QSGItem::QSGItem(QSGItem* parent)
: QObject(*(new QSGItemPrivate), parent)
{
    Q_D(QSGItem);
    d->init(parent);
}

/*! \internal
*/
QSGItem::QSGItem(QSGItemPrivate &dd, QSGItem *parent)
: QObject(dd, parent)
{
    Q_D(QSGItem);
    d->init(parent);
}

/*!
    Destroys the QSGItem.
*/
QSGItem::~QSGItem()
{
    Q_D(QSGItem);

    // XXX todo - optimize
    setParentItem(0);
    while (!d->childItems.isEmpty())
        d->childItems.first()->setParentItem(0);

    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        QSGAnchorsPrivate *anchor = d->changeListeners.at(ii).listener->anchorPrivate();
        if (anchor)
            anchor->clearItem(this);
    }

    // XXX todo - the original checks if the parent is being destroyed
    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        QSGAnchorsPrivate *anchor = d->changeListeners.at(ii).listener->anchorPrivate();
        if (anchor && anchor->item && anchor->item->parent() != this) //child will be deleted anyway
            anchor->updateOnComplete();
    }

    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QSGItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QSGItemPrivate::Destroyed)
            change.listener->itemDestroyed(this);
    }
    d->changeListeners.clear();
    delete d->_anchorLines; d->_anchorLines = 0;
    delete d->_anchors; d->_anchors = 0;
    delete d->_stateGroup; d->_stateGroup = 0;
    delete d->_contents; d->_contents = 0;
}

/*!
    \qmlproperty enumeration QtQuick2::Item::transformOrigin
    This property holds the origin point around which scale and rotation transform.

    Nine transform origins are available, as shown in the image below.

    \image declarative-transformorigin.png

    This example rotates an image around its bottom-right corner.
    \qml
    Image {
        source: "myimage.png"
        transformOrigin: Item.BottomRight
        rotation: 45
    }
    \endqml

    The default transform origin is \c Item.Center.

    To set an arbitrary transform origin point use the \l Scale or \l Rotation
    transform elements.
*/

/*!
    \qmlproperty Item QtQuick2::Item::parent
    This property holds the parent of the item.
*/

/*!
    \property QSGItem::parent
    This property holds the parent of the item.
*/
void QSGItem::setParentItem(QSGItem *parentItem)
{
    Q_D(QSGItem);
    if (parentItem == d->parentItem)
        return;

    d->removeFromDirtyList();

    QSGItem *oldParentItem = d->parentItem;
    QSGItem *scopeFocusedItem = 0;

    if (oldParentItem) {
        QSGItemPrivate *op = QSGItemPrivate::get(oldParentItem);

        QSGItem *scopeItem = 0;

        if (d->canvas && hasFocus()) {
            scopeItem = oldParentItem;
            while (!scopeItem->isFocusScope()) scopeItem = scopeItem->parentItem();
            scopeFocusedItem = this;
        } else if (d->canvas && !isFocusScope() && d->subFocusItem) {
            scopeItem = oldParentItem;
            while (!scopeItem->isFocusScope()) scopeItem = scopeItem->parentItem();
            scopeFocusedItem = d->subFocusItem;
        }

        if (scopeFocusedItem)
            QSGCanvasPrivate::get(d->canvas)->clearFocusInScope(scopeItem, scopeFocusedItem,
                                                                QSGCanvasPrivate::DontChangeFocusProperty);

        op->removeChild(this);
    }

    d->parentItem = parentItem;

    QSGCanvas *parentCanvas = parentItem?QSGItemPrivate::get(parentItem)->canvas:0;
    if (d->canvas != parentCanvas) {
        if (d->canvas && d->itemNodeInstance)
            QSGCanvasPrivate::get(d->canvas)->cleanup(d->itemNodeInstance);

        QSGItemPrivate::InitializationState initState;
        initState.clear();
        d->initCanvas(&initState, parentCanvas);
    }

    d->dirty(QSGItemPrivate::ParentChanged);

    if (d->parentItem)
        QSGItemPrivate::get(d->parentItem)->addChild(this);

    d->setEffectiveVisibleRecur(d->calcEffectiveVisible());
    d->setEffectiveEnableRecur(d->calcEffectiveEnable());

    if (scopeFocusedItem && d->parentItem && d->canvas) {
        // We need to test whether this item becomes scope focused
        QSGItem *scopeItem = 0;
        scopeItem = d->parentItem;
        while (!scopeItem->isFocusScope()) scopeItem = scopeItem->parentItem();

        if (scopeItem->scopedFocusItem()) {
            QSGItemPrivate::get(scopeFocusedItem)->focus = false;
            emit scopeFocusedItem->focusChanged(false);
        } else {
            QSGCanvasPrivate::get(d->canvas)->setFocusInScope(scopeItem, scopeFocusedItem,
                                                              QSGCanvasPrivate::DontChangeFocusProperty);
        }
    }

    d->resolveLayoutMirror();

    d->itemChange(ItemParentHasChanged, d->parentItem);

    emit parentChanged(d->parentItem);
}

void QSGItem::stackBefore(const QSGItem *sibling)
{
    Q_D(QSGItem);
    if (!sibling || sibling == this || !d->parentItem || d->parentItem != QSGItemPrivate::get(sibling)->parentItem) {
        qWarning("QSGItem::stackBefore: Cannot stack before %p, which must be a sibling", sibling);
        return;
    }

    QSGItemPrivate *parentPrivate = QSGItemPrivate::get(d->parentItem);

    int myIndex = parentPrivate->childItems.indexOf(this);
    int siblingIndex = parentPrivate->childItems.indexOf(const_cast<QSGItem *>(sibling));

    Q_ASSERT(myIndex != -1 && siblingIndex != -1);

    if (myIndex == siblingIndex - 1)
        return;

    parentPrivate->childItems.removeAt(myIndex);

    if (myIndex < siblingIndex) --siblingIndex;

    parentPrivate->childItems.insert(siblingIndex, this);

    parentPrivate->dirty(QSGItemPrivate::ChildrenStackingChanged);

    for (int ii = qMin(siblingIndex, myIndex); ii < parentPrivate->childItems.count(); ++ii)
        QSGItemPrivate::get(parentPrivate->childItems.at(ii))->siblingOrderChanged();
}

void QSGItem::stackAfter(const QSGItem *sibling)
{
    Q_D(QSGItem);
    if (!sibling || sibling == this || !d->parentItem || d->parentItem != QSGItemPrivate::get(sibling)->parentItem) {
        qWarning("QSGItem::stackAfter: Cannot stack after %p, which must be a sibling", sibling);
        return;
    }

    QSGItemPrivate *parentPrivate = QSGItemPrivate::get(d->parentItem);

    int myIndex = parentPrivate->childItems.indexOf(this);
    int siblingIndex = parentPrivate->childItems.indexOf(const_cast<QSGItem *>(sibling));

    Q_ASSERT(myIndex != -1 && siblingIndex != -1);

    if (myIndex == siblingIndex + 1)
        return;

    parentPrivate->childItems.removeAt(myIndex);

    if (myIndex < siblingIndex) --siblingIndex;

    parentPrivate->childItems.insert(siblingIndex + 1, this);

    parentPrivate->dirty(QSGItemPrivate::ChildrenStackingChanged);

    for (int ii = qMin(myIndex, siblingIndex + 1); ii < parentPrivate->childItems.count(); ++ii)
        QSGItemPrivate::get(parentPrivate->childItems.at(ii))->siblingOrderChanged();
}

/*!
    Returns the QSGItem parent of this item.
*/
QSGItem *QSGItem::parentItem() const
{
    Q_D(const QSGItem);
    return d->parentItem;
}

QSGEngine *QSGItem::sceneGraphEngine() const
{
    return canvas()->sceneGraphEngine();
}

QSGCanvas *QSGItem::canvas() const
{
    Q_D(const QSGItem);
    return d->canvas;
}

static bool itemZOrder_sort(QSGItem *lhs, QSGItem *rhs)
{
    return lhs->z() < rhs->z();
}

QList<QSGItem *> QSGItemPrivate::paintOrderChildItems() const
{
    // XXX todo - optimize, don't sort and return items that are
    // ignored anyway, like invisible or disabled items.
    QList<QSGItem *> items = childItems;
    qStableSort(items.begin(), items.end(), itemZOrder_sort);
    return items;
}

void QSGItemPrivate::addChild(QSGItem *child)
{
    Q_Q(QSGItem);

    Q_ASSERT(!childItems.contains(child));

    childItems.append(child);

    dirty(QSGItemPrivate::ChildrenChanged);

    itemChange(QSGItem::ItemChildAddedChange, child);

    emit q->childrenChanged();
}

void QSGItemPrivate::removeChild(QSGItem *child)
{
    Q_Q(QSGItem);

    Q_ASSERT(child);
    Q_ASSERT(childItems.contains(child));
    childItems.removeOne(child);
    Q_ASSERT(!childItems.contains(child));

    dirty(QSGItemPrivate::ChildrenChanged);

    itemChange(QSGItem::ItemChildRemovedChange, child);

    emit q->childrenChanged();
}

void QSGItemPrivate::InitializationState::clear()
{
    focusScope = 0;
}

void QSGItemPrivate::InitializationState::clear(QSGItem *fs)
{
    focusScope = fs;
}

QSGItem *QSGItemPrivate::InitializationState::getFocusScope(QSGItem *item)
{
    if (!focusScope) {
        QSGItem *fs = item->parentItem();
        while (!fs->isFocusScope())
            fs = fs->parentItem();
        focusScope = fs;
    }
    return focusScope;
}

void QSGItemPrivate::initCanvas(InitializationState *state, QSGCanvas *c)
{
    Q_Q(QSGItem);

    if (canvas) {
        removeFromDirtyList();
        QSGCanvasPrivate *c = QSGCanvasPrivate::get(canvas);
        if (polishScheduled)
            c->itemsToPolish.remove(q);
        if (c->mouseGrabberItem == q)
            c->mouseGrabberItem = 0;
        if ( hoverEnabled )
            c->hoverItems.removeAll(q);
    }

    canvas = c;

    if (canvas && polishScheduled)
        QSGCanvasPrivate::get(canvas)->itemsToPolish.insert(q);

    if (canvas && hoverEnabled && !canvas->hasMouseTracking())
        canvas->setMouseTracking(true);

    // XXX todo - why aren't these added to the destroy list?
    itemNodeInstance = 0;
    opacityNode = 0;
    clipNode = 0;
    rootNode = 0;
    groupNode = 0;
    paintNode = 0;
    beforePaintNode = 0;

    InitializationState _dummy;
    InitializationState *childState = state;

    if (c && q->isFocusScope()) {
        _dummy.clear(q);
        childState = &_dummy;
    }

    for (int ii = 0; ii < childItems.count(); ++ii) {
        QSGItem *child = childItems.at(ii);
        QSGItemPrivate::get(child)->initCanvas(childState, c);
    }

    if (c && focus) {
        // Fixup
        if (state->getFocusScope(q)->scopedFocusItem()) {
            focus = false;
            emit q->focusChanged(false);
        } else {
            QSGCanvasPrivate::get(canvas)->setFocusInScope(state->getFocusScope(q), q);
        }
    }

    dirty(Canvas);

    itemChange(QSGItem::ItemSceneChange, c);
}

/*!
Returns a transform that maps points from canvas space into item space.
*/
QTransform QSGItemPrivate::canvasToItemTransform() const
{
    // XXX todo - optimize
    return itemToCanvasTransform().inverted();
}

/*!
Returns a transform that maps points from item space into canvas space.
*/
QTransform QSGItemPrivate::itemToCanvasTransform() const
{
    // XXX todo
    QTransform rv = parentItem?QSGItemPrivate::get(parentItem)->itemToCanvasTransform():QTransform();
    itemToParentTransform(rv);
    return rv;
}

/*!
Motifies \a t with this items local transform relative to its parent.
*/
void QSGItemPrivate::itemToParentTransform(QTransform &t) const
{
    if (x || y)
        t.translate(x, y);

    if (!transforms.isEmpty()) {
        QMatrix4x4 m(t);
        for (int ii = transforms.count() - 1; ii >= 0; --ii)
            transforms.at(ii)->applyTo(&m);
        t = m.toTransform();
    }

    if (scale != 1. || rotation != 0.) {
        QPointF tp = computeTransformOrigin();
        t.translate(tp.x(), tp.y());
        t.scale(scale, scale);
        t.rotate(rotation);
        t.translate(-tp.x(), -tp.y());
    }
}


/*!
    \qmlproperty real QtQuick2::Item::childrenRect.x
    \qmlproperty real QtQuick2::Item::childrenRect.y
    \qmlproperty real QtQuick2::Item::childrenRect.width
    \qmlproperty real QtQuick2::Item::childrenRect.height

    The childrenRect properties allow an item access to the geometry of its
    children. This property is useful if you have an item that needs to be
    sized to fit its children.
*/


/*!
    \qmlproperty list<Item> QtQuick2::Item::children
    \qmlproperty list<Object> QtQuick2::Item::resources

    The children property contains the list of visual children of this item.
    The resources property contains non-visual resources that you want to
    reference by name.

    Generally you can rely on Item's default property to handle all this for
    you, but it can come in handy in some cases.

    \qml
    Item {
        children: [
            Text {},
            Rectangle {}
        ]
        resources: [
            Component {
                id: myComponent
                Text {}
            }
        ]
    }
    \endqml
*/

/*!
    Returns true if construction of the QML component is complete; otherwise
    returns false.

    It is often desirable to delay some processing until the component is
    completed.

    \sa componentComplete()
*/
bool QSGItem::isComponentComplete() const
{
    Q_D(const QSGItem);
    return d->componentComplete;
}

QSGItemPrivate::QSGItemPrivate()
: _anchors(0), _contents(0), baselineOffset(0), _anchorLines(0), _stateGroup(0), origin(QSGItem::Center),

  flags(0), widthValid(false), heightValid(false), componentComplete(true),
  keepMouse(false), hoverEnabled(false), smooth(false), focus(false), activeFocus(false), notifiedFocus(false),
  notifiedActiveFocus(false), filtersChildMouseEvents(false), explicitVisible(true),
  effectiveVisible(true), explicitEnable(true), effectiveEnable(true), polishScheduled(false),
  inheritedLayoutMirror(false), effectiveLayoutMirror(false), isMirrorImplicit(true),
  inheritMirrorFromParent(false), inheritMirrorFromItem(false), childrenDoNotOverlap(false),

  canvas(0), parentItem(0),

  subFocusItem(0),

  x(0), y(0), width(0), height(0), implicitWidth(0), implicitHeight(0),
  z(0), scale(1), rotation(0), opacity(1),

  attachedLayoutDirection(0), acceptedMouseButtons(0),
  imHints(Qt::ImhNone),

  keyHandler(0),

  dirtyAttributes(0), nextDirtyItem(0), prevDirtyItem(0),

  itemNodeInstance(0), opacityNode(0), clipNode(0), rootNode(0), groupNode(0), paintNode(0)
  , beforePaintNode(0), effectRefCount(0), hideRefCount(0)
{
}

void QSGItemPrivate::init(QSGItem *parent)
{
    Q_Q(QSGItem);
    baselineOffset.invalidate();

    if (parent) {
        q->setParentItem(parent);
        QSGItemPrivate *parentPrivate = QSGItemPrivate::get(parent);
        setImplicitLayoutMirror(parentPrivate->inheritedLayoutMirror, parentPrivate->inheritMirrorFromParent);
    }
}

void QSGItemPrivate::data_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    if (!o)
        return;

    QSGItem *that = static_cast<QSGItem *>(prop->object);

    // This test is measurably (albeit only slightly) faster than qobject_cast<>()
    const QMetaObject *mo = o->metaObject();
    while (mo && mo != &QSGItem::staticMetaObject) {
        if (mo == &QGraphicsObject::staticMetaObject)
            qWarning("Cannot add a QtQuick 1.0 item (%s) into a QtQuick 2.0 scene!", o->metaObject()->className());
        mo = mo->d.superdata;
    }

    if (mo) {
        QSGItem *item = static_cast<QSGItem *>(o);
        item->setParentItem(that);
    } else {
        // XXX todo - do we really want this behavior?
        o->setParent(that);
    }
}

/*!
    \qmlproperty list<Object> QtQuick2::Item::data
    \default

    The data property allows you to freely mix visual children and resources
    in an item.  If you assign a visual item to the data list it becomes
    a child and if you assign any other object type, it is added as a resource.

    So you can write:
    \qml
    Item {
        Text {}
        Rectangle {}
        Timer {}
    }
    \endqml

    instead of:
    \qml
    Item {
        children: [
            Text {},
            Rectangle {}
        ]
        resources: [
            Timer {}
        ]
    }
    \endqml

    data is a behind-the-scenes property: you should never need to explicitly
    specify it.
 */

int QSGItemPrivate::data_count(QDeclarativeListProperty<QObject> *prop)
{
    Q_UNUSED(prop);
    // XXX todo
    return 0;
}

QObject *QSGItemPrivate::data_at(QDeclarativeListProperty<QObject> *prop, int i)
{
    Q_UNUSED(prop);
    Q_UNUSED(i);
    // XXX todo
    return 0;
}

void QSGItemPrivate::data_clear(QDeclarativeListProperty<QObject> *prop)
{
    Q_UNUSED(prop);
    // XXX todo
}

QObject *QSGItemPrivate::resources_at(QDeclarativeListProperty<QObject> *prop, int index)
{
    const QObjectList children = prop->object->children();
    if (index < children.count())
        return children.at(index);
    else
        return 0;
}

void QSGItemPrivate::resources_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    // XXX todo - do we really want this behavior?
    o->setParent(prop->object);
}

int QSGItemPrivate::resources_count(QDeclarativeListProperty<QObject> *prop)
{
    return prop->object->children().count();
}

void QSGItemPrivate::resources_clear(QDeclarativeListProperty<QObject> *prop)
{
    // XXX todo - do we really want this behavior?
    const QObjectList children = prop->object->children();
    for (int index = 0; index < children.count(); index++)
        children.at(index)->setParent(0);
}

QSGItem *QSGItemPrivate::children_at(QDeclarativeListProperty<QSGItem> *prop, int index)
{
    QSGItemPrivate *p = QSGItemPrivate::get(static_cast<QSGItem *>(prop->object));
    if (index >= p->childItems.count() || index < 0)
        return 0;
    else
        return p->childItems.at(index);
}

void QSGItemPrivate::children_append(QDeclarativeListProperty<QSGItem> *prop, QSGItem *o)
{
    if (!o)
        return;

    QSGItem *that = static_cast<QSGItem *>(prop->object);
    if (o->parentItem() == that)
        o->setParentItem(0);

    o->setParentItem(that);
}

int QSGItemPrivate::children_count(QDeclarativeListProperty<QSGItem> *prop)
{
    QSGItemPrivate *p = QSGItemPrivate::get(static_cast<QSGItem *>(prop->object));
    return p->childItems.count();
}

void QSGItemPrivate::children_clear(QDeclarativeListProperty<QSGItem> *prop)
{
    QSGItem *that = static_cast<QSGItem *>(prop->object);
    QSGItemPrivate *p = QSGItemPrivate::get(that);
    while (!p->childItems.isEmpty())
        p->childItems.at(0)->setParentItem(0);
}

int QSGItemPrivate::transform_count(QDeclarativeListProperty<QSGTransform> *prop)
{
    QSGItem *that = static_cast<QSGItem *>(prop->object);
    return QSGItemPrivate::get(that)->transforms.count();
}

void QSGTransform::appendToItem(QSGItem *item)
{
    Q_D(QSGTransform);
    if (!item)
        return;

    QSGItemPrivate *p = QSGItemPrivate::get(item);

    if (!d->items.isEmpty() && !p->transforms.isEmpty() && p->transforms.contains(this)) {
        p->transforms.removeOne(this);
        p->transforms.append(this);
    } else {
        p->transforms.append(this);
        d->items.append(item);
    }

    p->dirty(QSGItemPrivate::Transform);
}

void QSGTransform::prependToItem(QSGItem *item)
{
    Q_D(QSGTransform);
    if (!item)
        return;

    QSGItemPrivate *p = QSGItemPrivate::get(item);

    if (!d->items.isEmpty() && !p->transforms.isEmpty() && p->transforms.contains(this)) {
        p->transforms.removeOne(this);
        p->transforms.prepend(this);
    } else {
        p->transforms.prepend(this);
        d->items.append(item);
    }

    p->dirty(QSGItemPrivate::Transform);
}

void QSGItemPrivate::transform_append(QDeclarativeListProperty<QSGTransform> *prop, QSGTransform *transform)
{
    if (!transform)
        return;

    QSGItem *that = static_cast<QSGItem *>(prop->object);
    transform->appendToItem(that);
}

QSGTransform *QSGItemPrivate::transform_at(QDeclarativeListProperty<QSGTransform> *prop, int idx)
{
    QSGItem *that = static_cast<QSGItem *>(prop->object);
    QSGItemPrivate *p = QSGItemPrivate::get(that);

    if (idx < 0 || idx >= p->transforms.count())
        return 0;
    else
        return p->transforms.at(idx);
}

void QSGItemPrivate::transform_clear(QDeclarativeListProperty<QSGTransform> *prop)
{
    QSGItem *that = static_cast<QSGItem *>(prop->object);
    QSGItemPrivate *p = QSGItemPrivate::get(that);

    for (int ii = 0; ii < p->transforms.count(); ++ii) {
        QSGTransform *t = p->transforms.at(ii);
        QSGTransformPrivate *tp = QSGTransformPrivate::get(t);
        tp->items.removeOne(that);
    }

    p->transforms.clear();

    p->dirty(QSGItemPrivate::Transform);
}

/*!
    \property QSGItem::childrenRect
    \brief The geometry of an item's children.

    This property holds the (collective) position and size of the item's children.
*/

/*!
  \qmlproperty real QtQuick2::Item::x
  \qmlproperty real QtQuick2::Item::y
  \qmlproperty real QtQuick2::Item::width
  \qmlproperty real QtQuick2::Item::height

  Defines the item's position and size relative to its parent.

  \qml
  Item { x: 100; y: 100; width: 100; height: 100 }
  \endqml
 */

/*!
  \qmlproperty real QtQuick2::Item::z

  Sets the stacking order of sibling items.  By default the stacking order is 0.

  Items with a higher stacking value are drawn on top of siblings with a
  lower stacking order.  Items with the same stacking value are drawn
  bottom up in the order they appear.  Items with a negative stacking
  value are drawn under their parent's content.

  The following example shows the various effects of stacking order.

  \table
  \row
  \o \image declarative-item_stacking1.png
  \o Same \c z - later children above earlier children:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking2.png
  \o Higher \c z on top:
  \qml
  Item {
      Rectangle {
          z: 1
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking3.png
  \o Same \c z - children above parents:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking4.png
  \o Lower \c z below:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              z: -1
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \endtable
 */

/*!
    \qmlproperty bool QtQuick2::Item::visible

    This property holds whether the item is visible. By default this is true.

    Setting this property directly affects the \c visible value of child
    items. When set to \c false, the \c visible values of all child items also
    become \c false. When set to \c true, the \c visible values of child items
    are returned to \c true, unless they have explicitly been set to \c false.

    (Because of this flow-on behavior, using the \c visible property may not
    have the intended effect if a property binding should only respond to
    explicit property changes. In such cases it may be better to use the
    \l opacity property instead.)

    Setting this property to \c false automatically causes \l focus to be set
    to \c false, and this item will longer receive mouse and keyboard events.
    (In contrast, setting the \l opacity to 0 does not affect the \l focus
    property and the receiving of key events.)

    \note This property's value is only affected by changes to this property or
    the parent's \c visible property. It does not change, for example, if this
    item moves off-screen, or if the \l opacity changes to 0.
*/


/*!
  \qmlproperty AnchorLine QtQuick2::Item::anchors.top
  \qmlproperty AnchorLine QtQuick2::Item::anchors.bottom
  \qmlproperty AnchorLine QtQuick2::Item::anchors.left
  \qmlproperty AnchorLine QtQuick2::Item::anchors.right
  \qmlproperty AnchorLine QtQuick2::Item::anchors.horizontalCenter
  \qmlproperty AnchorLine QtQuick2::Item::anchors.verticalCenter
  \qmlproperty AnchorLine QtQuick2::Item::anchors.baseline

  \qmlproperty Item QtQuick2::Item::anchors.fill
  \qmlproperty Item QtQuick2::Item::anchors.centerIn

  \qmlproperty real QtQuick2::Item::anchors.margins
  \qmlproperty real QtQuick2::Item::anchors.topMargin
  \qmlproperty real QtQuick2::Item::anchors.bottomMargin
  \qmlproperty real QtQuick2::Item::anchors.leftMargin
  \qmlproperty real QtQuick2::Item::anchors.rightMargin
  \qmlproperty real QtQuick2::Item::anchors.horizontalCenterOffset
  \qmlproperty real QtQuick2::Item::anchors.verticalCenterOffset
  \qmlproperty real QtQuick2::Item::anchors.baselineOffset

  \qmlproperty bool QtQuick2::Item::anchors.mirrored

  Anchors provide a way to position an item by specifying its
  relationship with other items.

  Margins apply to top, bottom, left, right, and fill anchors.
  The \c anchors.margins property can be used to set all of the various margins at once, to the same value.
  Note that margins are anchor-specific and are not applied if an item does not
  use anchors.

  Offsets apply for horizontal center, vertical center, and baseline anchors.

  \table
  \row
  \o \image declarative-anchors_example.png
  \o Text anchored to Image, horizontally centered and vertically below, with a margin.
  \qml
  Item {
      Image {
          id: pic
          // ...
      }
      Text {
          id: label
          anchors.horizontalCenter: pic.horizontalCenter
          anchors.top: pic.bottom
          anchors.topMargin: 5
          // ...
      }
  }
  \endqml
  \row
  \o \image declarative-anchors_example2.png
  \o
  Left of Text anchored to right of Image, with a margin. The y
  property of both defaults to 0.

  \qml
  Item {
      Image {
          id: pic
          // ...
      }
      Text {
          id: label
          anchors.left: pic.right
          anchors.leftMargin: 5
          // ...
      }
  }
  \endqml
  \endtable

  \c anchors.fill provides a convenient way for one item to have the
  same geometry as another item, and is equivalent to connecting all
  four directional anchors.

  To clear an anchor value, set it to \c undefined.

  \c anchors.mirrored returns true it the layout has been \l {LayoutMirroring}{mirrored}.

  \note You can only anchor an item to siblings or a parent.

  For more information see \l {anchor-layout}{Anchor Layouts}.
*/

/*!
  \property QSGItem::baselineOffset
  \brief The position of the item's baseline in local coordinates.

  The baseline of a \l Text item is the imaginary line on which the text
  sits. Controls containing text usually set their baseline to the
  baseline of their text.

  For non-text items, a default baseline offset of 0 is used.
*/
QSGAnchors *QSGItemPrivate::anchors() const
{
    if (!_anchors) {
        Q_Q(const QSGItem);
        _anchors = new QSGAnchors(const_cast<QSGItem *>(q));
        if (!componentComplete)
            _anchors->classBegin();
    }
    return _anchors;
}

QSGItemPrivate::AnchorLines *QSGItemPrivate::anchorLines() const
{
    Q_Q(const QSGItem);
    if (!_anchorLines) _anchorLines =
        new AnchorLines(const_cast<QSGItem *>(q));
    return _anchorLines;
}

void QSGItemPrivate::siblingOrderChanged()
{
    Q_Q(QSGItem);
    for(int ii = 0; ii < changeListeners.count(); ++ii) {
        const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
        if (change.types & QSGItemPrivate::SiblingOrder) {
            change.listener->itemSiblingOrderChanged(q);
        }
    }
}

QDeclarativeListProperty<QObject> QSGItemPrivate::data()
{
    return QDeclarativeListProperty<QObject>(q_func(), 0, QSGItemPrivate::data_append,
                                             QSGItemPrivate::data_count,
                                             QSGItemPrivate::data_at,
                                             QSGItemPrivate::data_clear);
}

QRectF QSGItem::childrenRect()
{
    Q_D(QSGItem);
    if (!d->_contents) {
        d->_contents = new QSGContents(this);
        if (d->componentComplete)
            d->_contents->complete();
    }
    return d->_contents->rectF();
}

QList<QSGItem *> QSGItem::childItems() const
{
    Q_D(const QSGItem);
    return d->childItems;
}

bool QSGItem::clip() const
{
    return flags() & ItemClipsChildrenToShape;
}

void QSGItem::setClip(bool c)
{
    if (clip() == c)
        return;

    setFlag(ItemClipsChildrenToShape, c);

    emit clipChanged(c);
}


/*!
  This function is called to handle this item's changes in
  geometry from \a oldGeometry to \a newGeometry. If the two
  geometries are the same, it doesn't do anything.
 */
void QSGItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QSGItem);

    if (d->_anchors)
        QSGAnchorsPrivate::get(d->_anchors)->updateMe();

    for(int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QSGItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QSGItemPrivate::Geometry)
            change.listener->itemGeometryChanged(this, newGeometry, oldGeometry);
    }

    if (newGeometry.x() != oldGeometry.x())
        emit xChanged();
    if (newGeometry.y() != oldGeometry.y())
        emit yChanged();
    if (newGeometry.width() != oldGeometry.width())
        emit widthChanged();
    if (newGeometry.height() != oldGeometry.height())
        emit heightChanged();
}

/*!
    Called by the rendering thread when it is time to sync the state of the QML objects with the
    scene graph objects. The function should return the root of the scene graph subtree for
    this item. \a oldNode is the node that was returned the last time the function was called.

    The main thread is blocked while this function is executed so it is safe to read
    values from the QSGItem instance and other objects in the main thread.

    \warning This is the only function in which it is allowed to make use of scene graph
    objects from the main thread. Use of scene graph objects outside this function will
    result in race conditions and potential crashes.
 */

QSGNode *QSGItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    delete oldNode;
    return 0;
}

QSGTransformNode *QSGItemPrivate::createTransformNode()
{
    return new QSGTransformNode;
}

void QSGItem::updatePolish()
{
}

void QSGItemPrivate::removeItemChangeListener(QSGItemChangeListener *listener, ChangeTypes types)
{
    ChangeListener change(listener, types);
    changeListeners.removeOne(change);
}

void QSGItem::keyPressEvent(QKeyEvent *event)
{
    event->ignore();
}

void QSGItem::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

void QSGItem::inputMethodEvent(QInputMethodEvent *event)
{
    event->ignore();
}

void QSGItem::focusInEvent(QFocusEvent *)
{
}

void QSGItem::focusOutEvent(QFocusEvent *)
{
}

void QSGItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

void QSGItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

void QSGItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

void QSGItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    mousePressEvent(event);
}

void QSGItem::mouseUngrabEvent()
{
    // XXX todo
}

void QSGItem::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}

void QSGItem::touchEvent(QTouchEvent *event)
{
    event->ignore();
}

void QSGItem::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
}

void QSGItem::hoverMoveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
}

void QSGItem::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
}

void QSGItem::dragMoveEvent(QSGDragEvent *event)
{
    event->setAccepted(false);
}

void QSGItem::dragEnterEvent(QSGDragEvent *event)
{
    event->setAccepted(false);
}

void QSGItem::dragExitEvent(QSGDragEvent *event)
{
    event->setAccepted(false);
}

void QSGItem::dragDropEvent(QSGDragEvent *event)
{
    event->setAccepted(false);
}

bool QSGItem::childMouseEventFilter(QSGItem *, QEvent *)
{
    return false;
}

void QSGItem::windowDeactivateEvent()
{
    foreach (QSGItem* item, childItems()) {
        item->windowDeactivateEvent();
    }
}

Qt::InputMethodHints QSGItem::inputMethodHints() const
{
    Q_D(const QSGItem);
    return d->imHints;
}

void QSGItem::setInputMethodHints(Qt::InputMethodHints hints)
{
    Q_D(QSGItem);
    d->imHints = hints;

    if (!d->canvas || d->canvas->activeFocusItem() != this)
        return;

    QSGCanvasPrivate::get(d->canvas)->updateInputMethodData();
#ifndef QT_NO_IM
    if (d->canvas->hasFocus())
        if (QInputContext *inputContext = d->canvas->inputContext())
            inputContext->update();
#endif
}

void QSGItem::updateMicroFocus()
{
#ifndef QT_NO_IM
    Q_D(QSGItem);
    if (d->canvas && d->canvas->hasFocus())
        if (QInputContext *inputContext = d->canvas->inputContext())
            inputContext->update();
#endif
}

QVariant QSGItem::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QSGItem);
    QVariant v;

    if (d->keyHandler)
        v = d->keyHandler->inputMethodQuery(query);

    return v;
}

QSGAnchorLine QSGItemPrivate::left() const
{
    return anchorLines()->left;
}

QSGAnchorLine QSGItemPrivate::right() const
{
    return anchorLines()->right;
}

QSGAnchorLine QSGItemPrivate::horizontalCenter() const
{
    return anchorLines()->hCenter;
}

QSGAnchorLine QSGItemPrivate::top() const
{
    return anchorLines()->top;
}

QSGAnchorLine QSGItemPrivate::bottom() const
{
    return anchorLines()->bottom;
}

QSGAnchorLine QSGItemPrivate::verticalCenter() const
{
    return anchorLines()->vCenter;
}

QSGAnchorLine QSGItemPrivate::baseline() const
{
    return anchorLines()->baseline;
}

qreal QSGItem::baselineOffset() const
{
    Q_D(const QSGItem);
    if (!d->baselineOffset.isValid()) {
        return 0.0;
    } else
        return d->baselineOffset;
}

void QSGItem::setBaselineOffset(qreal offset)
{
    Q_D(QSGItem);
    if (offset == d->baselineOffset)
        return;

    d->baselineOffset = offset;

    for(int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QSGItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QSGItemPrivate::Geometry) {
            QSGAnchorsPrivate *anchor = change.listener->anchorPrivate();
            if (anchor)
                anchor->updateVerticalAnchors();
        }
    }
    emit baselineOffsetChanged(offset);
}

void QSGItem::update()
{
    Q_D(QSGItem);
    Q_ASSERT(flags() & ItemHasContents);
    d->dirty(QSGItemPrivate::Content);
}

void QSGItem::polish()
{
    Q_D(QSGItem);
    if (!d->polishScheduled) {
        d->polishScheduled = true;
        if (d->canvas) {
            QSGCanvasPrivate *p = QSGCanvasPrivate::get(d->canvas);
            bool maybeupdate = p->itemsToPolish.isEmpty();
            p->itemsToPolish.insert(this);
            if (maybeupdate) d->canvas->maybeUpdate();
        }
    }
}

void QSGItem::mapFromItem(QDeclarativeV8Function *args) const
{
    if (args->Length() != 0) {
        v8::Local<v8::Value> item = (*args)[0];
        QV8Engine *engine = args->engine();

        QSGItem *itemObj = 0;
        if (!item->IsNull())
            itemObj = qobject_cast<QSGItem*>(engine->toQObject(item));

        if (!itemObj && !item->IsNull()) {
            qmlInfo(this) << "mapFromItem() given argument \"" << engine->toString(item->ToString())
                          << "\" which is neither null nor an Item";
            return;
        }

        v8::Local<v8::Object> rv = v8::Object::New();
        args->returnValue(rv);

        qreal x = (args->Length() > 1)?(*args)[1]->NumberValue():0;
        qreal y = (args->Length() > 2)?(*args)[2]->NumberValue():0;

        QPointF p = mapFromItem(itemObj, QPointF(x, y));

        rv->Set(v8::String::New("x"), v8::Number::New(p.x()));
        rv->Set(v8::String::New("y"), v8::Number::New(p.y()));
    }
}

QTransform QSGItem::itemTransform(QSGItem *other, bool *ok) const
{
    Q_D(const QSGItem);

    // XXX todo - we need to be able to handle common parents better and detect
    // invalid cases
    if (ok) *ok = true;

    QTransform t = d->itemToCanvasTransform();
    if (other) t *= QSGItemPrivate::get(other)->canvasToItemTransform();

    return t;
}

void QSGItem::mapToItem(QDeclarativeV8Function *args) const
{
    if (args->Length() != 0) {
        v8::Local<v8::Value> item = (*args)[0];
        QV8Engine *engine = args->engine();

        QSGItem *itemObj = 0;
        if (!item->IsNull())
            itemObj = qobject_cast<QSGItem*>(engine->toQObject(item));

        if (!itemObj && !item->IsNull()) {
            qmlInfo(this) << "mapToItem() given argument \"" << engine->toString(item->ToString())
                          << "\" which is neither null nor an Item";
            return;
        }

        v8::Local<v8::Object> rv = v8::Object::New();
        args->returnValue(rv);

        qreal x = (args->Length() > 1)?(*args)[1]->NumberValue():0;
        qreal y = (args->Length() > 2)?(*args)[2]->NumberValue():0;

        QPointF p = mapToItem(itemObj, QPointF(x, y));

        rv->Set(v8::String::New("x"), v8::Number::New(p.x()));
        rv->Set(v8::String::New("y"), v8::Number::New(p.y()));
    }
}

void QSGItem::forceActiveFocus()
{
    setFocus(true);
    QSGItem *parent = parentItem();
    while (parent) {
        if (parent->flags() & QSGItem::ItemIsFocusScope) {
            parent->setFocus(true);
        }
        parent = parent->parentItem();
    }
}

QSGItem *QSGItem::childAt(qreal x, qreal y) const
{
    // XXX todo - should this include transform etc.?
    const QList<QSGItem *> children = childItems();
    for (int i = children.count()-1; i >= 0; --i) {
        QSGItem *child = children.at(i);
        if (child->isVisible() && child->x() <= x
                && child->x() + child->width() >= x
                && child->y() <= y
                && child->y() + child->height() >= y)
            return child;
    }
    return 0;
}

QDeclarativeListProperty<QObject> QSGItemPrivate::resources()
{
    return QDeclarativeListProperty<QObject>(q_func(), 0, QSGItemPrivate::resources_append,
                                             QSGItemPrivate::resources_count,
                                             QSGItemPrivate::resources_at,
                                             QSGItemPrivate::resources_clear);
}

QDeclarativeListProperty<QSGItem> QSGItemPrivate::children()
{
    return QDeclarativeListProperty<QSGItem>(q_func(), 0, QSGItemPrivate::children_append,
                                             QSGItemPrivate::children_count,
                                             QSGItemPrivate::children_at,
                                             QSGItemPrivate::children_clear);

}

QDeclarativeListProperty<QDeclarativeState> QSGItemPrivate::states()
{
    return _states()->statesProperty();
}

QDeclarativeListProperty<QDeclarativeTransition> QSGItemPrivate::transitions()
{
    return _states()->transitionsProperty();
}

QString QSGItemPrivate::state() const
{
    if (!_stateGroup)
        return QString();
    else
        return _stateGroup->state();
}

void QSGItemPrivate::setState(const QString &state)
{
    _states()->setState(state);
}

QDeclarativeListProperty<QSGTransform> QSGItem::transform()
{
    Q_D(QSGItem);
    return QDeclarativeListProperty<QSGTransform>(this, 0, d->transform_append, d->transform_count,
                                                  d->transform_at, d->transform_clear);
}

void QSGItem::classBegin()
{
    Q_D(QSGItem);
    d->componentComplete = false;
    if (d->_stateGroup)
        d->_stateGroup->classBegin();
    if (d->_anchors)
        d->_anchors->classBegin();
}

void QSGItem::componentComplete()
{
    Q_D(QSGItem);
    d->componentComplete = true;
    if (d->_stateGroup)
        d->_stateGroup->componentComplete();
    if (d->_anchors) {
        d->_anchors->componentComplete();
        QSGAnchorsPrivate::get(d->_anchors)->updateOnComplete();
    }
    if (d->keyHandler)
        d->keyHandler->componentComplete();
    if (d->_contents)
        d->_contents->complete();
}

QDeclarativeStateGroup *QSGItemPrivate::_states()
{
    Q_Q(QSGItem);
    if (!_stateGroup) {
        _stateGroup = new QDeclarativeStateGroup;
        if (!componentComplete)
            _stateGroup->classBegin();
        QObject::connect(_stateGroup, SIGNAL(stateChanged(QString)),
                         q, SIGNAL(stateChanged(QString)));
    }

    return _stateGroup;
}

QSGItemPrivate::AnchorLines::AnchorLines(QSGItem *q)
{
    left.item = q;
    left.anchorLine = QSGAnchorLine::Left;
    right.item = q;
    right.anchorLine = QSGAnchorLine::Right;
    hCenter.item = q;
    hCenter.anchorLine = QSGAnchorLine::HCenter;
    top.item = q;
    top.anchorLine = QSGAnchorLine::Top;
    bottom.item = q;
    bottom.anchorLine = QSGAnchorLine::Bottom;
    vCenter.item = q;
    vCenter.anchorLine = QSGAnchorLine::VCenter;
    baseline.item = q;
    baseline.anchorLine = QSGAnchorLine::Baseline;
}

QPointF QSGItemPrivate::computeTransformOrigin() const
{
    switch(origin) {
    default:
    case QSGItem::TopLeft:
        return QPointF(0, 0);
    case QSGItem::Top:
        return QPointF(width / 2., 0);
    case QSGItem::TopRight:
        return QPointF(width, 0);
    case QSGItem::Left:
        return QPointF(0, height / 2.);
    case QSGItem::Center:
        return QPointF(width / 2., height / 2.);
    case QSGItem::Right:
        return QPointF(width, height / 2.);
    case QSGItem::BottomLeft:
        return QPointF(0, height);
    case QSGItem::Bottom:
        return QPointF(width / 2., height);
    case QSGItem::BottomRight:
        return QPointF(width, height);
    }
}

void QSGItemPrivate::transformChanged()
{
}

void QSGItemPrivate::deliverKeyEvent(QKeyEvent *e)
{
    Q_Q(QSGItem);

    Q_ASSERT(e->isAccepted());
    if (keyHandler) {
        if (e->type() == QEvent::KeyPress)
            keyHandler->keyPressed(e, false);
        else
            keyHandler->keyReleased(e, false);

        if (e->isAccepted())
            return;
        else
            e->accept();
    }

    if (e->type() == QEvent::KeyPress)
        q->keyPressEvent(e);
    else
        q->keyReleaseEvent(e);

    if (e->isAccepted())
        return;

    if (keyHandler) {
        e->accept();

        if (e->type() == QEvent::KeyPress)
            keyHandler->keyPressed(e, true);
        else
            keyHandler->keyReleased(e, true);
    }
}

void QSGItemPrivate::deliverInputMethodEvent(QInputMethodEvent *e)
{
    Q_Q(QSGItem);

    Q_ASSERT(e->isAccepted());
    if (keyHandler) {
        keyHandler->inputMethodEvent(e, false);

        if (e->isAccepted())
            return;
        else
            e->accept();
    }

    q->inputMethodEvent(e);

    if (e->isAccepted())
        return;

    if (keyHandler) {
        e->accept();

        keyHandler->inputMethodEvent(e, true);
    }
}

void QSGItemPrivate::deliverFocusEvent(QFocusEvent *e)
{
    Q_Q(QSGItem);

    if (e->type() == QEvent::FocusIn) {
        q->focusInEvent(e);
    } else {
        q->focusOutEvent(e);
    }
}

void QSGItemPrivate::deliverMouseEvent(QGraphicsSceneMouseEvent *e)
{
    Q_Q(QSGItem);

    Q_ASSERT(e->isAccepted());

    switch(e->type()) {
    default:
        Q_ASSERT(!"Unknown event type");
    case QEvent::GraphicsSceneMouseMove:
        q->mouseMoveEvent(e);
        break;
    case QEvent::GraphicsSceneMousePress:
        q->mousePressEvent(e);
        break;
    case QEvent::GraphicsSceneMouseRelease:
        q->mouseReleaseEvent(e);
        break;
    case QEvent::GraphicsSceneMouseDoubleClick:
        q->mouseDoubleClickEvent(e);
        break;
    }
}

void QSGItemPrivate::deliverWheelEvent(QWheelEvent *e)
{
    Q_Q(QSGItem);
    q->wheelEvent(e);
}

void QSGItemPrivate::deliverTouchEvent(QTouchEvent *e)
{
    Q_Q(QSGItem);
    q->touchEvent(e);
}

void QSGItemPrivate::deliverHoverEvent(QHoverEvent *e)
{
    Q_Q(QSGItem);
    switch(e->type()) {
    default:
        Q_ASSERT(!"Unknown event type");
    case QEvent::HoverEnter:
        q->hoverEnterEvent(e);
        break;
    case QEvent::HoverLeave:
        q->hoverLeaveEvent(e);
        break;
    case QEvent::HoverMove:
        q->hoverMoveEvent(e);
        break;
    }
}

void QSGItemPrivate::deliverDragEvent(QSGDragEvent *e)
{
    Q_Q(QSGItem);
    switch (e->type()) {
    default:
        Q_ASSERT(!"Unknown event type");
    case QSGEvent::SGDragEnter:
        q->dragEnterEvent(e);
        break;
    case QSGEvent::SGDragExit:
        q->dragExitEvent(e);
        break;
    case QSGEvent::SGDragMove:
        q->dragMoveEvent(e);
        break;
    case QSGEvent::SGDragDrop:
        q->dragDropEvent(e);
        break;
    }
}

void QSGItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_UNUSED(change);
    Q_UNUSED(value);
}

/*! \internal */
// XXX todo - do we want/need this anymore?
// Note that it's now used for varying clip rect
QRectF QSGItem::boundingRect() const
{
    Q_D(const QSGItem);
    return QRectF(0, 0, d->width, d->height);
}

QSGItem::TransformOrigin QSGItem::transformOrigin() const
{
    Q_D(const QSGItem);
    return d->origin;
}

void QSGItem::setTransformOrigin(TransformOrigin origin)
{
    Q_D(QSGItem);
    if (origin == d->origin)
        return;

    d->origin = origin;
    d->dirty(QSGItemPrivate::TransformOrigin);

    emit transformOriginChanged(d->origin);
}

QPointF QSGItem::transformOriginPoint() const
{
    Q_D(const QSGItem);
    return d->computeTransformOrigin();
}

qreal QSGItem::z() const
{
    Q_D(const QSGItem);
    return d->z;
}

void QSGItem::setZ(qreal v)
{
    Q_D(QSGItem);
    if (d->z == v)
        return;

    d->z = v;

    d->dirty(QSGItemPrivate::ZValue);
    if (d->parentItem)
        QSGItemPrivate::get(d->parentItem)->dirty(QSGItemPrivate::ChildrenStackingChanged);

    emit zChanged();
}


/*!
  \qmlproperty real QtQuick2::Item::rotation
  This property holds the rotation of the item in degrees clockwise.

  This specifies how many degrees to rotate the item around its transformOrigin.
  The default rotation is 0 degrees (i.e. not rotated at all).

  \table
  \row
  \o \image declarative-rotation.png
  \o
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          rotation: 30
      }
  }
  \endqml
  \endtable

  \sa transform, Rotation
*/

/*!
  \qmlproperty real QtQuick2::Item::scale
  This property holds the scale of the item.

  A scale of less than 1 means the item will be displayed smaller than
  normal, and a scale of greater than 1 means the item will be
  displayed larger than normal.  A negative scale means the item will
  be mirrored.

  By default, items are displayed at a scale of 1 (i.e. at their
  normal size).

  Scaling is from the item's transformOrigin.

  \table
  \row
  \o \image declarative-scale.png
  \o
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "green"
          width: 25; height: 25
      }
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          scale: 1.4
      }
  }
  \endqml
  \endtable

  \sa transform, Scale
*/

/*!
  \qmlproperty real QtQuick2::Item::opacity

  This property holds the opacity of the item.  Opacity is specified as a
  number between 0 (fully transparent) and 1 (fully opaque).  The default is 1.

  When this property is set, the specified opacity is also applied
  individually to child items.  In almost all cases this is what you want,
  but in some cases it may produce undesired results. For example in the
  second set of rectangles below, the red rectangle has specified an opacity
  of 0.5, which affects the opacity of its blue child rectangle even though
  the child has not specified an opacity.

  \table
  \row
  \o \image declarative-item_opacity1.png
  \o
  \qml
    Item {
        Rectangle {
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \row
  \o \image declarative-item_opacity2.png
  \o
  \qml
    Item {
        Rectangle {
            opacity: 0.5
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \endtable

  If an item's opacity is set to 0, the item will no longer receive mouse
  events, but will continue to receive key events and will retain the keyboard
  \l focus if it has been set. (In contrast, setting the \l visible property
  to \c false stops both mouse and keyboard events, and also removes focus
  from the item.)
*/

/*!
  Returns a value indicating whether mouse input should
  remain with this item exclusively.

  \sa setKeepMouseGrab()
 */

qreal QSGItem::rotation() const
{
    Q_D(const QSGItem);
    return d->rotation;
}

void QSGItem::setRotation(qreal r)
{
    Q_D(QSGItem);
    if (d->rotation == r)
        return;

    d->rotation = r;

    d->dirty(QSGItemPrivate::BasicTransform);

    d->itemChange(ItemRotationHasChanged, r);

    emit rotationChanged();
}

qreal QSGItem::scale() const
{
    Q_D(const QSGItem);
    return d->scale;
}

void QSGItem::setScale(qreal s)
{
    Q_D(QSGItem);
    if (d->scale == s)
        return;

    d->scale = s;

    d->dirty(QSGItemPrivate::BasicTransform);

    emit scaleChanged();
}

qreal QSGItem::opacity() const
{
    Q_D(const QSGItem);
    return d->opacity;
}

void QSGItem::setOpacity(qreal o)
{
    Q_D(QSGItem);
    if (d->opacity == o)
        return;

    d->opacity = o;

    d->dirty(QSGItemPrivate::OpacityValue);

    d->itemChange(ItemOpacityHasChanged, o);

    emit opacityChanged();
}

bool QSGItem::isVisible() const
{
    Q_D(const QSGItem);
    return d->effectiveVisible;
}

void QSGItem::setVisible(bool v)
{
    Q_D(QSGItem);
    if (v == d->explicitVisible)
        return;

    d->explicitVisible = v;

    d->setEffectiveVisibleRecur(d->calcEffectiveVisible());
}

bool QSGItem::isEnabled() const
{
    Q_D(const QSGItem);
    return d->effectiveEnable;
}

void QSGItem::setEnabled(bool e)
{
    Q_D(QSGItem);
    if (e == d->explicitEnable)
        return;

    d->explicitEnable = e;

    d->setEffectiveEnableRecur(d->calcEffectiveEnable());
}

bool QSGItemPrivate::calcEffectiveVisible() const
{
    // XXX todo - Should the effective visible of an element with no parent just be the current
    // effective visible?  This would prevent pointless re-processing in the case of an element
    // moving to/from a no-parent situation, but it is different from what graphics view does.
    return explicitVisible && (!parentItem || QSGItemPrivate::get(parentItem)->effectiveVisible);
}

void QSGItemPrivate::setEffectiveVisibleRecur(bool newEffectiveVisible)
{
    Q_Q(QSGItem);

    if (newEffectiveVisible && !explicitVisible) {
        // This item locally overrides visibility
        return;
    }

    if (newEffectiveVisible == effectiveVisible) {
        // No change necessary
        return;
    }

    effectiveVisible = newEffectiveVisible;
    dirty(Visible);
    if (parentItem) QSGItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);

    if (canvas) {
        QSGCanvasPrivate *canvasPriv = QSGCanvasPrivate::get(canvas);
        if (canvasPriv->mouseGrabberItem == q)
            q->ungrabMouse();
    }

    for (int ii = 0; ii < childItems.count(); ++ii)
        QSGItemPrivate::get(childItems.at(ii))->setEffectiveVisibleRecur(newEffectiveVisible);

    for(int ii = 0; ii < changeListeners.count(); ++ii) {
        const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
        if (change.types & QSGItemPrivate::Visibility)
            change.listener->itemVisibilityChanged(q);
    }

    emit q->visibleChanged();
}

bool QSGItemPrivate::calcEffectiveEnable() const
{
    // XXX todo - Should the effective enable of an element with no parent just be the current
    // effective enable?  This would prevent pointless re-processing in the case of an element
    // moving to/from a no-parent situation, but it is different from what graphics view does.
    return explicitEnable && (!parentItem || QSGItemPrivate::get(parentItem)->effectiveEnable);
}

void QSGItemPrivate::setEffectiveEnableRecur(bool newEffectiveEnable)
{
    Q_Q(QSGItem);

    // XXX todo - need to fixup focus

    if (newEffectiveEnable && !explicitEnable) {
        // This item locally overrides enable
        return;
    }

    if (newEffectiveEnable == effectiveEnable) {
        // No change necessary
        return;
    }

    effectiveEnable = newEffectiveEnable;

    if (canvas) {
        QSGCanvasPrivate *canvasPriv = QSGCanvasPrivate::get(canvas);
        if (canvasPriv->mouseGrabberItem == q)
            q->ungrabMouse();
    }

    for (int ii = 0; ii < childItems.count(); ++ii)
        QSGItemPrivate::get(childItems.at(ii))->setEffectiveEnableRecur(newEffectiveEnable);

    emit q->enabledChanged();
}

QString QSGItemPrivate::dirtyToString() const
{
#define DIRTY_TO_STRING(value) if (dirtyAttributes & value) { \
    if (!rv.isEmpty()) \
        rv.append(QLatin1String("|")); \
    rv.append(QLatin1String(#value)); \
}

//    QString rv = QLatin1String("0x") + QString::number(dirtyAttributes, 16);
    QString rv;

    DIRTY_TO_STRING(TransformOrigin);
    DIRTY_TO_STRING(Transform);
    DIRTY_TO_STRING(BasicTransform);
    DIRTY_TO_STRING(Position);
    DIRTY_TO_STRING(Size);
    DIRTY_TO_STRING(ZValue);
    DIRTY_TO_STRING(Content);
    DIRTY_TO_STRING(Smooth);
    DIRTY_TO_STRING(OpacityValue);
    DIRTY_TO_STRING(ChildrenChanged);
    DIRTY_TO_STRING(ChildrenStackingChanged);
    DIRTY_TO_STRING(ParentChanged);
    DIRTY_TO_STRING(Clip);
    DIRTY_TO_STRING(Canvas);
    DIRTY_TO_STRING(EffectReference);
    DIRTY_TO_STRING(Visible);
    DIRTY_TO_STRING(HideReference);

    return rv;
}

void QSGItemPrivate::dirty(DirtyType type)
{
    Q_Q(QSGItem);
    if (type & (TransformOrigin | Transform | BasicTransform | Position | Size))
        transformChanged();

    if (!(dirtyAttributes & type) || (canvas && !prevDirtyItem)) {
        dirtyAttributes |= type;
        if (canvas) {
            addToDirtyList();
            QSGCanvasPrivate::get(canvas)->dirtyItem(q);
        }
    }
}

void QSGItemPrivate::addToDirtyList()
{
    Q_Q(QSGItem);

    Q_ASSERT(canvas);
    if (!prevDirtyItem) {
        Q_ASSERT(!nextDirtyItem);

        QSGCanvasPrivate *p = QSGCanvasPrivate::get(canvas);
        nextDirtyItem = p->dirtyItemList;
        if (nextDirtyItem) QSGItemPrivate::get(nextDirtyItem)->prevDirtyItem = &nextDirtyItem;
        prevDirtyItem = &p->dirtyItemList;
        p->dirtyItemList = q;
        p->dirtyItem(q);
    }
    Q_ASSERT(prevDirtyItem);
}

void QSGItemPrivate::removeFromDirtyList()
{
    if (prevDirtyItem) {
        if (nextDirtyItem) QSGItemPrivate::get(nextDirtyItem)->prevDirtyItem = prevDirtyItem;
        *prevDirtyItem = nextDirtyItem;
        prevDirtyItem = 0;
        nextDirtyItem = 0;
    }
    Q_ASSERT(!prevDirtyItem);
    Q_ASSERT(!nextDirtyItem);
}

void QSGItemPrivate::refFromEffectItem(bool hide)
{
    ++effectRefCount;
    if (1 == effectRefCount) {
        dirty(EffectReference);
        if (parentItem) QSGItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);
    }
    if (hide) {
        if (++hideRefCount == 1)
            dirty(HideReference);
    }
}

void QSGItemPrivate::derefFromEffectItem(bool unhide)
{
    Q_ASSERT(effectRefCount);
    --effectRefCount;
    if (0 == effectRefCount) {
        dirty(EffectReference);
        if (parentItem) QSGItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);
    }
    if (unhide) {
        if (--hideRefCount == 0)
            dirty(HideReference);
    }
}

void QSGItemPrivate::itemChange(QSGItem::ItemChange change, const QSGItem::ItemChangeData &data)
{
    Q_Q(QSGItem);
    switch(change) {
    case QSGItem::ItemChildAddedChange:
        q->itemChange(change, data);
        if (_contents && componentComplete)
            _contents->childAdded(data.item);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QSGItemPrivate::Children) {
                change.listener->itemChildAdded(q, data.item);
            }
        }
        break;
    case QSGItem::ItemChildRemovedChange:
        q->itemChange(change, data);
        if (_contents && componentComplete)
            _contents->childRemoved(data.item);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QSGItemPrivate::Children) {
                change.listener->itemChildRemoved(q, data.item);
            }
        }
        break;
    case QSGItem::ItemSceneChange:
        q->itemChange(change, data);
        break;
    case QSGItem::ItemVisibleHasChanged:
        q->itemChange(change, data);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QSGItemPrivate::Visibility) {
                change.listener->itemVisibilityChanged(q);
            }
        }
        break;
    case QSGItem::ItemParentHasChanged:
        q->itemChange(change, data);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QSGItemPrivate::Parent) {
                change.listener->itemParentChanged(q, data.item);
            }
        }
        break;
    case QSGItem::ItemOpacityHasChanged:
        q->itemChange(change, data);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QSGItemPrivate::Opacity) {
                change.listener->itemOpacityChanged(q);
            }
        }
        break;
    case QSGItem::ItemActiveFocusHasChanged:
        q->itemChange(change, data);
        break;
    case QSGItem::ItemRotationHasChanged:
        q->itemChange(change, data);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QSGItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QSGItemPrivate::Rotation) {
                change.listener->itemRotationChanged(q);
            }
        }
        break;
    }
}

/*!
    \property QSGItem::smooth
    \brief whether the item is smoothly transformed.

    This property is provided purely for the purpose of optimization. Turning
    smooth transforms off is faster, but looks worse; turning smooth
    transformations on is slower, but looks better.

    By default smooth transformations are off.
*/

/*!
    Returns true if the item should be drawn with antialiasing and
    smooth pixmap filtering, false otherwise.

    The default is false.

    \sa setSmooth()
*/
bool QSGItem::smooth() const
{
    Q_D(const QSGItem);
    return d->smooth;
}

/*!
    Sets whether the item should be drawn with antialiasing and
    smooth pixmap filtering to \a smooth.

    \sa smooth()
*/
void QSGItem::setSmooth(bool smooth)
{
    Q_D(QSGItem);
    if (d->smooth == smooth)
        return;

    d->smooth = smooth;
    d->dirty(QSGItemPrivate::Smooth);

    emit smoothChanged(smooth);
}

QSGItem::Flags QSGItem::flags() const
{
    Q_D(const QSGItem);
    return (QSGItem::Flags)d->flags;
}

void QSGItem::setFlag(Flag flag, bool enabled)
{
    Q_D(QSGItem);
    if (enabled)
        setFlags((Flags)(d->flags | (quint32)flag));
    else
        setFlags((Flags)(d->flags & ~(quint32)flag));
}

void QSGItem::setFlags(Flags flags)
{
    Q_D(QSGItem);

    if ((flags & ItemIsFocusScope) != (d->flags & ItemIsFocusScope)) {
        if (flags & ItemIsFocusScope && !d->childItems.isEmpty() && d->canvas) {
            qWarning("QSGItem: Cannot set FocusScope once item has children and is in a canvas.");
            flags &= ~ItemIsFocusScope;
        } else if (d->flags & ItemIsFocusScope) {
            qWarning("QSGItem: Cannot unset FocusScope flag.");
            flags |= ItemIsFocusScope;
        }
    }

    if ((flags & ItemClipsChildrenToShape ) != (d->flags & ItemClipsChildrenToShape))
        d->dirty(QSGItemPrivate::Clip);

    d->flags = flags;
}

qreal QSGItem::x() const
{
    Q_D(const QSGItem);
    return d->x;
}

qreal QSGItem::y() const
{
    Q_D(const QSGItem);
    return d->y;
}

QPointF QSGItem::pos() const
{
    Q_D(const QSGItem);
    return QPointF(d->x, d->y);
}

void QSGItem::setX(qreal v)
{
    Q_D(QSGItem);
    if (d->x == v)
        return;

    qreal oldx = d->x;
    d->x = v;

    d->dirty(QSGItemPrivate::Position);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(oldx, y(), width(), height()));
}

void QSGItem::setY(qreal v)
{
    Q_D(QSGItem);
    if (d->y == v)
        return;

    qreal oldy = d->y;
    d->y = v;

    d->dirty(QSGItemPrivate::Position);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), oldy, width(), height()));
}

void QSGItem::setPos(const QPointF &pos)
{
    Q_D(QSGItem);
    if (QPointF(d->x, d->y) == pos)
        return;

    qreal oldx = d->x;
    qreal oldy = d->y;

    d->x = pos.x();
    d->y = pos.y();

    d->dirty(QSGItemPrivate::Position);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(oldx, oldy, width(), height()));
}

qreal QSGItem::width() const
{
    Q_D(const QSGItem);
    return d->width;
}

void QSGItem::setWidth(qreal w)
{
    Q_D(QSGItem);
    if (qIsNaN(w))
        return;

    d->widthValid = true;
    if (d->width == w)
        return;

    qreal oldWidth = d->width;
    d->width = w;

    d->dirty(QSGItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, height()));
}

void QSGItem::resetWidth()
{
    Q_D(QSGItem);
    d->widthValid = false;
    setImplicitWidth(implicitWidth());
}

void QSGItemPrivate::implicitWidthChanged()
{
    Q_Q(QSGItem);
    emit q->implicitWidthChanged();
}

qreal QSGItemPrivate::getImplicitWidth() const
{
    return implicitWidth;
}
/*!
    Returns the width of the item that is implied by other properties that determine the content.
*/
qreal QSGItem::implicitWidth() const
{
    Q_D(const QSGItem);
    return d->getImplicitWidth();
}

/*!
    \qmlproperty real QtQuick2::Item::implicitWidth
    \qmlproperty real QtQuick2::Item::implicitHeight

    Defines the natural width or height of the Item if no \l width or \l height is specified.

    The default implicit size for most items is 0x0, however some elements have an inherent
    implicit size which cannot be overridden, e.g. Image, Text.

    Setting the implicit size is useful for defining components that have a preferred size
    based on their content, for example:

    \qml
    // Label.qml
    import QtQuick 1.1

    Item {
        property alias icon: image.source
        property alias label: text.text
        implicitWidth: text.implicitWidth + image.implicitWidth
        implicitHeight: Math.max(text.implicitHeight, image.implicitHeight)
        Image { id: image }
        Text {
            id: text
            wrapMode: Text.Wrap
            anchors.left: image.right; anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    \endqml

    \bold Note: using implicitWidth of Text or TextEdit and setting the width explicitly
    incurs a performance penalty as the text must be laid out twice.
*/

/*!
    Sets the implied width of the item to \a w.
    This is the width implied by other properties that determine the content.
*/
void QSGItem::setImplicitWidth(qreal w)
{
    Q_D(QSGItem);
    bool changed = w != d->implicitWidth;
    d->implicitWidth = w;
    if (d->width == w || widthValid()) {
        if (changed)
            d->implicitWidthChanged();
        return;
    }

    qreal oldWidth = d->width;
    d->width = w;

    d->dirty(QSGItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, height()));

    if (changed)
        d->implicitWidthChanged();
}

/*!
    Returns whether the width property has been set explicitly.
*/
bool QSGItem::widthValid() const
{
    Q_D(const QSGItem);
    return d->widthValid;
}

qreal QSGItem::height() const
{
    Q_D(const QSGItem);
    return d->height;
}

void QSGItem::setHeight(qreal h)
{
    Q_D(QSGItem);
    if (qIsNaN(h))
        return;

    d->heightValid = true;
    if (d->height == h)
        return;

    qreal oldHeight = d->height;
    d->height = h;

    d->dirty(QSGItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), width(), oldHeight));
}

void QSGItem::resetHeight()
{
    Q_D(QSGItem);
    d->heightValid = false;
    setImplicitHeight(implicitHeight());
}

void QSGItemPrivate::implicitHeightChanged()
{
    Q_Q(QSGItem);
    emit q->implicitHeightChanged();
}

qreal QSGItemPrivate::getImplicitHeight() const
{
    return implicitHeight;
}

/*!
    Returns the height of the item that is implied by other properties that determine the content.
*/
qreal QSGItem::implicitHeight() const
{
    Q_D(const QSGItem);
    return d->getImplicitHeight();
}


/*!
    Sets the implied height of the item to \a h.
    This is the height implied by other properties that determine the content.
*/
void QSGItem::setImplicitHeight(qreal h)
{
    Q_D(QSGItem);
    bool changed = h != d->implicitHeight;
    d->implicitHeight = h;
    if (d->height == h || heightValid()) {
        if (changed)
            d->implicitHeightChanged();
        return;
    }

    qreal oldHeight = d->height;
    d->height = h;

    d->dirty(QSGItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), width(), oldHeight));

    if (changed)
        d->implicitHeightChanged();
}

/*!
    Returns whether the height property has been set explicitly.
*/
bool QSGItem::heightValid() const
{
    Q_D(const QSGItem);
    return d->heightValid;
}

void QSGItem::setSize(const QSizeF &size)
{
    Q_D(QSGItem);
    d->heightValid = true;
    d->widthValid = true;

    if (QSizeF(d->width, d->height) == size)
        return;

    qreal oldHeight = d->height;
    qreal oldWidth = d->width;
    d->height = size.height();
    d->width = size.width();

    d->dirty(QSGItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, oldHeight));
}

bool QSGItem::hasActiveFocus() const
{
    Q_D(const QSGItem);
    return d->activeFocus;
}

bool QSGItem::hasFocus() const
{
    Q_D(const QSGItem);
    return d->focus;
}

void QSGItem::setFocus(bool focus)
{
    Q_D(QSGItem);
    if (d->focus == focus)
        return;

    if (d->canvas) {
        // Need to find our nearest focus scope
        QSGItem *scope = parentItem();
        while (scope && !scope->isFocusScope())
            scope = scope->parentItem();
        if (focus)
            QSGCanvasPrivate::get(d->canvas)->setFocusInScope(scope, this);
        else
            QSGCanvasPrivate::get(d->canvas)->clearFocusInScope(scope, this);
    } else {
        d->focus = focus;
        emit focusChanged(focus);
    }
}

bool QSGItem::isFocusScope() const
{
    return flags() & ItemIsFocusScope;
}

QSGItem *QSGItem::scopedFocusItem() const
{
    Q_D(const QSGItem);
    if (!isFocusScope())
        return 0;
    else
        return d->subFocusItem;
}


Qt::MouseButtons QSGItem::acceptedMouseButtons() const
{
    Q_D(const QSGItem);
    return d->acceptedMouseButtons;
}

void QSGItem::setAcceptedMouseButtons(Qt::MouseButtons buttons)
{
    Q_D(QSGItem);
    d->acceptedMouseButtons = buttons;
}

bool QSGItem::filtersChildMouseEvents() const
{
    Q_D(const QSGItem);
    return d->filtersChildMouseEvents;
}

void QSGItem::setFiltersChildMouseEvents(bool filter)
{
    Q_D(QSGItem);
    d->filtersChildMouseEvents = filter;
}

bool QSGItem::isUnderMouse() const
{
    Q_D(const QSGItem);
    if (!d->canvas)
        return false;

    QPoint cursorPos = QCursor::pos();
    if (QRectF(0, 0, width(), height()).contains(mapFromScene(d->canvas->mapFromGlobal(cursorPos))))
        return true;
    return false;
}

bool QSGItem::acceptHoverEvents() const
{
    Q_D(const QSGItem);
    return d->hoverEnabled;
}

void QSGItem::setAcceptHoverEvents(bool enabled)
{
    Q_D(QSGItem);
    d->hoverEnabled = enabled;

    if (d->canvas){
        QSGCanvasPrivate *c = QSGCanvasPrivate::get(d->canvas);
        if (d->hoverEnabled){
            if (!d->canvas->hasMouseTracking())
                d->canvas->setMouseTracking(true);
            if (isUnderMouse())
                c->hoverItems.prepend(this);
                c->sendHoverEvent(QEvent::HoverEnter, this, c->lastMousePosition, c->lastMousePosition,
                        QApplication::keyboardModifiers(), true);
        } else {
            c->hoverItems.removeAll(this);
            c->sendHoverEvent(QEvent::HoverLeave, this, c->lastMousePosition, c->lastMousePosition,
                    QApplication::keyboardModifiers(), true);
        }
    }
}

void QSGItem::grabMouse() 
{
    Q_D(QSGItem);
    if (!d->canvas)
        return;
    QSGCanvasPrivate *canvasPriv = QSGCanvasPrivate::get(d->canvas);
    if (canvasPriv->mouseGrabberItem == this)
        return;

    QSGItem *oldGrabber = canvasPriv->mouseGrabberItem;
    canvasPriv->mouseGrabberItem = this;
    if (oldGrabber)
        oldGrabber->mouseUngrabEvent();
}

void QSGItem::ungrabMouse()
{
    Q_D(QSGItem);
    if (!d->canvas)
        return;
    QSGCanvasPrivate *canvasPriv = QSGCanvasPrivate::get(d->canvas);
    if (canvasPriv->mouseGrabberItem != this) {
        qWarning("QSGItem::ungrabMouse(): Item is not the mouse grabber.");
        return;
    }

    canvasPriv->mouseGrabberItem = 0;
    mouseUngrabEvent();
}

bool QSGItem::keepMouseGrab() const
{
    Q_D(const QSGItem);
    return d->keepMouse;
}

/*!
  The flag indicating whether the mouse should remain
  with this item is set to \a keep.

  This is useful for items that wish to grab and keep mouse
  interaction following a predefined gesture.  For example,
  an item that is interested in horizontal mouse movement
  may set keepMouseGrab to true once a threshold has been
  exceeded.  Once keepMouseGrab has been set to true, filtering
  items will not react to mouse events.

  If the item does not indicate that it wishes to retain mouse grab,
  a filtering item may steal the grab. For example, Flickable may attempt
  to steal a mouse grab if it detects that the user has begun to
  move the viewport.

  \sa keepMouseGrab()
 */
void QSGItem::setKeepMouseGrab(bool keep)
{
    Q_D(QSGItem);
    d->keepMouse = keep;
}

/*!
    \qmlmethod object QtQuick2::Item::mapFromItem(Item item, real x, real y)

    Maps the point (\a x, \a y), which is in \a item's coordinate system, to
    this item's coordinate system, and returns an object with \c x and \c y
    properties matching the mapped cooordinate.

    If \a item is a \c null value, this maps the point from the coordinate
    system of the root QML view.
*/
/*!
    \qmlmethod object QtQuick2::Item::mapToItem(Item item, real x, real y)

    Maps the point (\a x, \a y), which is in this item's coordinate system, to
    \a item's coordinate system, and returns an object with \c x and \c y
    properties matching the mapped cooordinate.

    If \a item is a \c null value, this maps \a x and \a y to the coordinate
    system of the root QML view.
*/
QPointF QSGItem::mapToItem(const QSGItem *item, const QPointF &point) const
{
    QPointF p = mapToScene(point);
    if (item)
        p = item->mapFromScene(p);
    return p;
}

QPointF QSGItem::mapToScene(const QPointF &point) const
{
    Q_D(const QSGItem);
    return d->itemToCanvasTransform().map(point);
}

QRectF QSGItem::mapRectToItem(const QSGItem *item, const QRectF &rect) const
{
    Q_D(const QSGItem);
    QTransform t = d->itemToCanvasTransform();
    if (item)
        t *= QSGItemPrivate::get(item)->canvasToItemTransform();
    return t.mapRect(rect);
}

QRectF QSGItem::mapRectToScene(const QRectF &rect) const
{
    Q_D(const QSGItem);
    return d->itemToCanvasTransform().mapRect(rect);
}

QPointF QSGItem::mapFromItem(const QSGItem *item, const QPointF &point) const
{
    QPointF p = item?item->mapToScene(point):point;
    return mapFromScene(p);
}

QPointF QSGItem::mapFromScene(const QPointF &point) const
{
    Q_D(const QSGItem);
    return d->canvasToItemTransform().map(point);
}

QRectF QSGItem::mapRectFromItem(const QSGItem *item, const QRectF &rect) const
{
    Q_D(const QSGItem);
    QTransform t = item?QSGItemPrivate::get(item)->itemToCanvasTransform():QTransform();
    t *= d->canvasToItemTransform();
    return t.mapRect(rect);
}

QRectF QSGItem::mapRectFromScene(const QRectF &rect) const
{
    Q_D(const QSGItem);
    return d->canvasToItemTransform().mapRect(rect);
}


/*!
    \qmlmethod QtQuick2::Item::forceActiveFocus()

    Forces active focus on the item.

    This method sets focus on the item and makes sure that all the focus scopes
    higher in the object hierarchy are also given the focus.
*/

/*!
    Forces active focus on the item.

    This method sets focus on the item and makes sure that all the focus scopes
    higher in the object hierarchy are also given the focus.
*/

/*!
  \qmlmethod QtQuick2::Item::childAt(real x, real y)

  Returns the visible child item at point (\a x, \a y), which is in this
  item's coordinate system, or \c null if there is no such item.
*/

/*!
  Returns the visible child item at point (\a x, \a y), which is in this
  item's coordinate system, or 0 if there is no such item.
*/

/*!
  \qmlproperty list<State> QtQuick2::Item::states
  This property holds a list of states defined by the item.

  \qml
  Item {
      states: [
          State {
              // ...
          },
          State {
              // ...
          }
          // ...
      ]
  }
  \endqml

  \sa {qmlstate}{States}
*/
/*!
  \qmlproperty list<Transition> QtQuick2::Item::transitions
  This property holds a list of transitions defined by the item.

  \qml
  Item {
      transitions: [
          Transition {
              // ...
          },
          Transition {
              // ...
          }
          // ...
      ]
  }
  \endqml

  \sa {QML Animation and Transitions}{Transitions}
*/
/*
  \qmlproperty list<Filter> QtQuick2::Item::filter
  This property holds a list of graphical filters to be applied to the item.

  \l {Filter}{Filters} include things like \l {Blur}{blurring}
  the item, or giving it a \l Reflection.  Some
  filters may not be available on all canvases; if a filter is not
  available on a certain canvas, it will simply not be applied for
  that canvas (but the QML will still be considered valid).

  \qml
  Item {
      filter: [
          Blur {
              // ...
          },
          Reflection {
              // ...
          }
          // ...
      ]
  }
  \endqml
*/

/*!
  \qmlproperty bool QtQuick2::Item::clip
  This property holds whether clipping is enabled. The default clip value is \c false.

  If clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle.

  Non-rectangular clipping regions are not supported for performance reasons.
*/

/*!
  \property QSGItem::clip
  This property holds whether clipping is enabled. The default clip value is \c false.

  If clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle. If you set
  clipping during an item's paint operation, remember to re-set it to
  prevent clipping the rest of your scene.

  Non-rectangular clipping regions are not supported for performance reasons.
*/

/*!
  \qmlproperty string QtQuick2::Item::state

  This property holds the name of the current state of the item.

  This property is often used in scripts to change between states. For
  example:

  \js
  function toggle() {
      if (button.state == 'On')
          button.state = 'Off';
      else
          button.state = 'On';
  }
  \endjs

  If the item is in its base state (i.e. no explicit state has been
  set), \c state will be a blank string. Likewise, you can return an
  item to its base state by setting its current state to \c ''.

  \sa {qmlstates}{States}
*/

/*!
  \qmlproperty list<Transform> QtQuick2::Item::transform
  This property holds the list of transformations to apply.

  For more information see \l Transform.
*/

/*!
    \enum QSGItem::TransformOrigin

    Controls the point about which simple transforms like scale apply.

    \value TopLeft The top-left corner of the item.
    \value Top The center point of the top of the item.
    \value TopRight The top-right corner of the item.
    \value Left The left most point of the vertical middle.
    \value Center The center of the item.
    \value Right The right most point of the vertical middle.
    \value BottomLeft The bottom-left corner of the item.
    \value Bottom The center point of the bottom of the item.
    \value BottomRight The bottom-right corner of the item.
*/


/*!
  \qmlproperty bool QtQuick2::Item::activeFocus

  This property indicates whether the item has active focus.

  An item with active focus will receive keyboard input,
  or is a FocusScope ancestor of the item that will receive keyboard input.

  Usually, activeFocus is gained by setting focus on an item and its enclosing
  FocusScopes. In the following example \c input will have activeFocus.
  \qml
  Rectangle {
      FocusScope {
          focus: true
          TextInput {
              id: input
              focus: true
          }
      }
  }
  \endqml

  \sa focus, {qmlfocus}{Keyboard Focus}
*/

/*!
  \qmlproperty bool QtQuick2::Item::focus
  This property indicates whether the item has focus within the enclosing focus scope. If true, this item
  will gain active focus when the enclosing focus scope gains active focus.
  In the following example, \c input will be given active focus when \c scope gains active focus.
  \qml
  Rectangle {
      FocusScope {
          id: scope
          TextInput {
              id: input
              focus: true
          }
      }
  }
  \endqml

  For the purposes of this property, the scene as a whole is assumed to act like a focus scope.
  On a practical level, that means the following QML will give active focus to \c input on startup.

  \qml
  Rectangle {
      TextInput {
          id: input
          focus: true
      }
  }
  \endqml

  \sa activeFocus, {qmlfocus}{Keyboard Focus}
*/


/*!
  \property QSGItem::anchors
  \internal
*/

/*!
  \property QSGItem::left
  \internal
*/

/*!
  \property QSGItem::right
  \internal
*/

/*!
  \property QSGItem::horizontalCenter
  \internal
*/

/*!
  \property QSGItem::top
  \internal
*/

/*!
  \property QSGItem::bottom
  \internal
*/

/*!
  \property QSGItem::verticalCenter
  \internal
*/

/*!
  \property QSGItem::focus
  \internal
*/

/*!
  \property QSGItem::transform
  \internal
*/

/*!
  \property QSGItem::transformOrigin
  \internal
*/

/*!
  \property QSGItem::activeFocus
  \internal
*/

/*!
  \property QSGItem::baseline
  \internal
*/

/*!
  \property QSGItem::data
  \internal
*/

/*!
  \property QSGItem::resources
  \internal
*/

/*!
  \property QSGItem::state
  \internal
*/

/*!
  \property QSGItem::states
  \internal
*/

/*!
  \property QSGItem::transformOriginPoint
  \internal
*/

/*!
  \property QSGItem::transitions
  \internal
*/

bool QSGItem::event(QEvent *ev)
{
    return QObject::event(ev);

#if 0
    if (ev->type() == QEvent::PolishRequest) {
        Q_D(QSGItem);
        d->polishScheduled = false;
        updatePolish();
        return true;
    } else {
        return QObject::event(ev);
    }
#endif
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, QSGItem *item)
{
    if (!item) {
        debug << "QSGItem(0)";
        return debug;
    }

    debug << item->metaObject()->className() << "(this =" << ((void*)item)
          << ", name=" << item->objectName()
          << ", parent =" << ((void*)item->parentItem())
          << ", geometry =" << QRectF(item->pos(), QSizeF(item->width(), item->height()))
          << ", z =" << item->z() << ')';
    return debug;
}
#endif

qint64 QSGItemPrivate::consistentTime = -1;
void QSGItemPrivate::setConsistentTime(qint64 t)
{
    consistentTime = t;
}

class QElapsedTimerConsistentTimeHack
{
public:
    void start() {
        t1 = QSGItemPrivate::consistentTime;
        t2 = 0;
    }
    qint64 elapsed() {
        return QSGItemPrivate::consistentTime - t1;
    }
    qint64 restart() {
        qint64 val = QSGItemPrivate::consistentTime - t1;
        t1 = QSGItemPrivate::consistentTime;
        t2 = 0;
        return val;
    }

private:
    qint64 t1;
    qint64 t2;
};

void QSGItemPrivate::start(QElapsedTimer &t)
{
    if (QSGItemPrivate::consistentTime == -1)
        t.start();
    else
        ((QElapsedTimerConsistentTimeHack*)&t)->start();
}

qint64 QSGItemPrivate::elapsed(QElapsedTimer &t)
{
    if (QSGItemPrivate::consistentTime == -1)
        return t.elapsed();
    else
        return ((QElapsedTimerConsistentTimeHack*)&t)->elapsed();
}

qint64 QSGItemPrivate::restart(QElapsedTimer &t)
{
    if (QSGItemPrivate::consistentTime == -1)
        return t.restart();
    else
        return ((QElapsedTimerConsistentTimeHack*)&t)->restart();
}

QT_END_NAMESPACE

#include <moc_qsgitem.cpp>
