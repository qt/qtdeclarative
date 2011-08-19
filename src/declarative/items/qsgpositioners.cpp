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

#include "qsgpositioners_p.h"
#include "qsgpositioners_p_p.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtCore/qmath.h>
#include <QtCore/qcoreapplication.h>

#include <private/qdeclarativestate_p.h>
#include <private/qdeclarativestategroup_p.h>
#include <private/qdeclarativestateoperations_p.h>
#include <private/qdeclarativetransition_p.h>

QT_BEGIN_NAMESPACE

static const QSGItemPrivate::ChangeTypes watchedChanges
    = QSGItemPrivate::Geometry
    | QSGItemPrivate::SiblingOrder
    | QSGItemPrivate::Visibility
    | QSGItemPrivate::Destroyed;

void QSGBasePositionerPrivate::watchChanges(QSGItem *other)
{
    QSGItemPrivate *otherPrivate = QSGItemPrivate::get(other);
    otherPrivate->addItemChangeListener(this, watchedChanges);
}

void QSGBasePositionerPrivate::unwatchChanges(QSGItem* other)
{
    QSGItemPrivate *otherPrivate = QSGItemPrivate::get(other);
    otherPrivate->removeItemChangeListener(this, watchedChanges);
}

QSGBasePositioner::QSGBasePositioner(PositionerType at, QSGItem *parent)
    : QSGImplicitSizeItem(*(new QSGBasePositionerPrivate), parent)
{
    Q_D(QSGBasePositioner);
    d->init(at);
}
/*!
    \internal
    \class QSGBasePositioner
    \brief The QSGBasePositioner class provides a base for QSGGraphics layouts.

    To create a QSGGraphics Positioner, simply subclass QSGBasePositioner and implement
    doLayout(), which is automatically called when the layout might need
    updating. In doLayout() use the setX and setY functions from QSGBasePositioner, and the
    base class will apply the positions along with the appropriate transitions. The items to
    position are provided in order as the protected member positionedItems.

    You also need to set a PositionerType, to declare whether you are positioning the x, y or both
    for the child items. Depending on the chosen type, only x or y changes will be applied.

    Note that the subclass is responsible for adding the spacing in between items.
*/

QSGBasePositioner::QSGBasePositioner(QSGBasePositionerPrivate &dd, PositionerType at, QSGItem *parent)
    : QSGImplicitSizeItem(dd, parent)
{
    Q_D(QSGBasePositioner);
    d->init(at);
}

QSGBasePositioner::~QSGBasePositioner()
{
    Q_D(QSGBasePositioner);
    for (int i = 0; i < positionedItems.count(); ++i)
        d->unwatchChanges(positionedItems.at(i).item);
    positionedItems.clear();
}

int QSGBasePositioner::spacing() const
{
    Q_D(const QSGBasePositioner);
    return d->spacing;
}

void QSGBasePositioner::setSpacing(int s)
{
    Q_D(QSGBasePositioner);
    if (s==d->spacing)
        return;
    d->spacing = s;
    prePositioning();
    emit spacingChanged();
}

QDeclarativeTransition *QSGBasePositioner::move() const
{
    Q_D(const QSGBasePositioner);
    return d->moveTransition;
}

void QSGBasePositioner::setMove(QDeclarativeTransition *mt)
{
    Q_D(QSGBasePositioner);
    if (mt == d->moveTransition)
        return;
    d->moveTransition = mt;
    emit moveChanged();
}

QDeclarativeTransition *QSGBasePositioner::add() const
{
    Q_D(const QSGBasePositioner);
    return d->addTransition;
}

void QSGBasePositioner::setAdd(QDeclarativeTransition *add)
{
    Q_D(QSGBasePositioner);
    if (add == d->addTransition)
        return;

    d->addTransition = add;
    emit addChanged();
}

void QSGBasePositioner::componentComplete()
{
    QSGItem::componentComplete();
    positionedItems.reserve(childItems().count());
    prePositioning();
    reportConflictingAnchors();
}

void QSGBasePositioner::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QSGBasePositioner);
    if (change == ItemChildAddedChange){
        prePositioning();
    } else if (change == ItemChildRemovedChange) {
        QSGItem *child = value.item;
        QSGBasePositioner::PositionedItem posItem(child);
        int idx = positionedItems.find(posItem);
        if (idx >= 0) {
            d->unwatchChanges(child);
            positionedItems.remove(idx);
        }
        prePositioning();
    }

    QSGItem::itemChange(change, value);
}

void QSGBasePositioner::prePositioning()
{
    Q_D(QSGBasePositioner);
    if (!isComponentComplete())
        return;

    if (d->doingPositioning)
        return;

    d->queuedPositioning = false;
    d->doingPositioning = true;
    //Need to order children by creation order modified by stacking order
    QList<QSGItem *> children = childItems();

    QPODVector<PositionedItem,8> oldItems;
    positionedItems.copyAndClear(oldItems);
    for (int ii = 0; ii < children.count(); ++ii) {
        QSGItem *child = children.at(ii);
        QSGItemPrivate *childPrivate = QSGItemPrivate::get(child);
        PositionedItem *item = 0;
        PositionedItem posItem(child);
        int wIdx = oldItems.find(posItem);
        if (wIdx < 0) {
            d->watchChanges(child);
            positionedItems.append(posItem);
            item = &positionedItems[positionedItems.count()-1];
            item->isNew = true;
            if (!childPrivate->explicitVisible || !child->width() || !child->height())
                item->isVisible = false;
        } else {
            item = &oldItems[wIdx];
            // Items are only omitted from positioning if they are explicitly hidden
            // i.e. their positioning is not affected if an ancestor is hidden.
            if (!childPrivate->explicitVisible || !child->width() || !child->height()) {
                item->isVisible = false;
            } else if (!item->isVisible) {
                item->isVisible = true;
                item->isNew = true;
            } else {
                item->isNew = false;
            }
            positionedItems.append(*item);
        }
    }
    QSizeF contentSize(0,0);
    doPositioning(&contentSize);
    updateAttachedProperties();
    if (!d->addActions.isEmpty() || !d->moveActions.isEmpty())
        finishApplyTransitions();
    d->doingPositioning = false;
    //Set implicit size to the size of its children
    setImplicitHeight(contentSize.height());
    setImplicitWidth(contentSize.width());
}

void QSGBasePositioner::positionX(int x, const PositionedItem &target)
{
    Q_D(QSGBasePositioner);
    if(d->type == Horizontal || d->type == Both){
        if (target.isNew) {
            if (!d->addTransition || !d->addTransition->enabled())
                target.item->setX(x);
            else
                d->addActions << QDeclarativeAction(target.item, QLatin1String("x"), QVariant(x));
        } else if (x != target.item->x()) {
            if (!d->moveTransition || !d->moveTransition->enabled())
                target.item->setX(x);
            else
                d->moveActions << QDeclarativeAction(target.item, QLatin1String("x"), QVariant(x));
        }
    }
}

void QSGBasePositioner::positionY(int y, const PositionedItem &target)
{
    Q_D(QSGBasePositioner);
    if(d->type == Vertical || d->type == Both){
        if (target.isNew) {
            if (!d->addTransition || !d->addTransition->enabled())
                target.item->setY(y);
            else
                d->addActions << QDeclarativeAction(target.item, QLatin1String("y"), QVariant(y));
        } else if (y != target.item->y()) {
            if (!d->moveTransition || !d->moveTransition->enabled())
                target.item->setY(y);
            else
                d->moveActions << QDeclarativeAction(target.item, QLatin1String("y"), QVariant(y));
        }
    }
}

void QSGBasePositioner::finishApplyTransitions()
{
    Q_D(QSGBasePositioner);
    // Note that if a transition is not set the transition manager will
    // apply the changes directly, in the case add/move aren't set
    d->addTransitionManager.transition(d->addActions, d->addTransition);
    d->moveTransitionManager.transition(d->moveActions, d->moveTransition);
    d->addActions.clear();
    d->moveActions.clear();
}

QSGPositionerAttached *QSGBasePositioner::qmlAttachedProperties(QObject *obj)
{
    return new QSGPositionerAttached(obj);
}

void QSGBasePositioner::updateAttachedProperties(QSGPositionerAttached *specificProperty, QSGItem *specificPropertyOwner) const
{
    // If this function is deemed too expensive or shows up in profiles, it could
    // be changed to run only when there are attached properties present. This
    // could be a flag in the positioner that is set by the attached property
    // constructor.
    QSGPositionerAttached *prevLastProperty = 0;
    QSGPositionerAttached *lastProperty = 0;

    int visibleItemIndex = 0;
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (!child.item)
            continue;

        QSGPositionerAttached *property = 0;

        if (specificProperty) {
            if (specificPropertyOwner == child.item) {
                property = specificProperty;
            }
        } else {
            property = static_cast<QSGPositionerAttached *>(qmlAttachedPropertiesObject<QSGBasePositioner>(child.item, false));
        }

        if (child.isVisible) {
            if (property) {
              property->setIndex(visibleItemIndex);
              property->setIsFirstItem(visibleItemIndex == 0);

              if (property->isLastItem())
                prevLastProperty = property;
            }

            lastProperty = property;
            ++visibleItemIndex;
        } else if (property) {
            property->setIndex(-1);
            property->setIsFirstItem(false);
            property->setIsLastItem(false);
        }
    }

    if (prevLastProperty && prevLastProperty != lastProperty)
        prevLastProperty->setIsLastItem(false);
    if (lastProperty)
      lastProperty->setIsLastItem(true);
}

/*!
    \qmlclass Positioner QSGPositionerAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-positioning-elements
    \brief The Positioner type provides attached properties that contain details on where an item exists in a positioner.

    Positioner items (such as Column, Row, Flow and Grid) provide automatic layout
    for child items. Attaching this property allows a child item to determine
    where it exists within the positioner.
*/

QSGPositionerAttached::QSGPositionerAttached(QObject *parent) : QObject(parent), m_index(-1), m_isFirstItem(false), m_isLastItem(false)
{
    QSGItem *attachedItem = qobject_cast<QSGItem *>(parent);
    if (attachedItem) {
        QSGBasePositioner *positioner = qobject_cast<QSGBasePositioner *>(attachedItem->parent());
        if (positioner) {
            positioner->updateAttachedProperties(this, attachedItem);
        }
    }
}

/*!
    \qmlattachedproperty Item QtQuick2::Positioner::index

    This property allows the item to determine
    its index within the positioner.
*/
void QSGPositionerAttached::setIndex(int index)
{
    if (m_index == index)
        return;
    m_index = index;
    emit indexChanged();
}

/*!
    \qmlattachedproperty Item QtQuick2::Positioner::isFirstItem
    \qmlattachedproperty Item QtQuick2::Positioner::isLastItem

    These properties allow the item to determine if it
    is the first or last item in the positioner, respectively.
*/
void QSGPositionerAttached::setIsFirstItem(bool isFirstItem)
{
    if (m_isFirstItem == isFirstItem)
        return;
    m_isFirstItem = isFirstItem;
    emit isFirstItemChanged();
}

void QSGPositionerAttached::setIsLastItem(bool isLastItem)
{
    if (m_isLastItem == isLastItem)
        return;
    m_isLastItem = isLastItem;
    emit isLastItemChanged();
}

/*!
  \qmlclass Column QSGColumn
    \inqmlmodule QtQuick 2
  \ingroup qml-positioning-elements
  \brief The Column item arranges its children vertically.
  \inherits Item

  The Column item positions its child items so that they are vertically
  aligned and not overlapping.

  Spacing between items can be added using the \l spacing property.
  Transitions can be used for cases where items managed by a Column are
  added or moved. These are stored in the \l add and \l move properties
  respectively.

  See \l{Using QML Positioner and Repeater Items} for more details about this item and other
  related items.

  \section1 Example Usage

  The following example positions differently shaped rectangles using a Column
  item.

  \image verticalpositioner_example.png

  \snippet doc/src/snippets/declarative/column/vertical-positioner.qml document

  \section1 Using Transitions

  Transitions can be used to animate items that are added to, moved within,
  or removed from a Column item. The \l add and \l move properties can be set to
  the transitions that will be applied when items are added to, removed from,
  or re-positioned within a Column item.

  The use of transitions with positioners is described in more detail in the
  \l{Using QML Positioner and Repeater Items#Using Transitions}{Using QML
  Positioner and Repeater Items} document.

  \image verticalpositioner_transition.gif

  \qml
  Column {
      spacing: 2
      add: Transition {
          // Define an animation for adding a new item...
      }
      move: Transition {
          // Define an animation for moving items within the column...
      }
      // ...
  }
  \endqml

  \section1 Limitations

  Note that the positioner assumes that the x and y positions of its children
  will not change. If you manually change the x or y properties in script, bind
  the x or y properties, use anchors on a child of a positioner, or have the
  height of a child depend on the position of a child, then the
  positioner may exhibit strange behavior. If you need to perform any of these
  actions, consider positioning the items without the use of a Column.

  Items with a width or height of 0 will not be positioned.

  \sa Row, Grid, Flow, Positioner, {declarative/positioners}{Positioners example}
*/
/*!
    \qmlproperty Transition QtQuick2::Column::add

    This property holds the transition to be applied when adding an
    item to the positioner. The transition will only be applied to the
    added item(s).  Positioner transitions will only affect the
    position (x, y) of items.

    For a positioner, adding an item can mean that either the object
    has been created or reparented, and thus is now a child or the
    positioner, or that the object has had its opacity increased from
    zero, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Column::move

    This property holds the transition to apply when moving an item
    within the positioner.  Positioner transitions will only affect
    the position (x, y) of items.

    This transition can be performed when other items are added or removed
    from the positioner, or when items resize themselves.

    \image positioner-move.gif

    \qml
    Column {
        move: Transition {
            NumberAnimation {
                properties: "y"
                duration: 1000
            }
        }
    }
    \endqml

    \sa add, {declarative/positioners}{Positioners example}
*/
/*!
  \qmlproperty int QtQuick2::Column::spacing

  The spacing is the amount in pixels left empty between adjacent
  items. The default spacing is 0.

  \sa Grid::spacing
*/
QSGColumn::QSGColumn(QSGItem *parent)
: QSGBasePositioner(Vertical, parent)
{
}

void QSGColumn::doPositioning(QSizeF *contentSize)
{
    int voffset = 0;

    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (!child.item || !child.isVisible)
            continue;

        if(child.item->y() != voffset)
            positionY(voffset, child);

        contentSize->setWidth(qMax(contentSize->width(), child.item->width()));

        voffset += child.item->height();
        voffset += spacing();
    }

    if (voffset != 0)//If we positioned any items, undo the spacing from the last item
        voffset -= spacing();
    contentSize->setHeight(voffset);
}

void QSGColumn::reportConflictingAnchors()
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors) {
                QSGAnchors::Anchors usedAnchors = anchors->usedAnchors();
                if (usedAnchors & QSGAnchors::TopAnchor ||
                    usedAnchors & QSGAnchors::BottomAnchor ||
                    usedAnchors & QSGAnchors::VCenterAnchor ||
                    anchors->fill() || anchors->centerIn()) {
                    d->anchorConflict = true;
                    break;
                }
            }
        }
    }
    if (d->anchorConflict) {
        qmlInfo(this) << "Cannot specify top, bottom, verticalCenter, fill or centerIn anchors for items inside Column";
    }
}
/*!
  \qmlclass Row QSGRow
    \inqmlmodule QtQuick 2
  \ingroup qml-positioning-elements
  \brief The Row item arranges its children horizontally.
  \inherits Item

  The Row item positions its child items so that they are horizontally
  aligned and not overlapping.

  Use \l spacing to set the spacing between items in a Row, and use the
  \l add and \l move properties to set the transitions that should be applied
  when items are added to, removed from, or re-positioned within the Row.

  See \l{Using QML Positioner and Repeater Items} for more details about this item and other
  related items.

  \section1 Example Usage

  The following example lays out differently shaped rectangles using a Row.

  \image horizontalpositioner_example.png

  \snippet doc/src/snippets/declarative/row/row.qml document

  \section1 Using Transitions

  Transitions can be used to animate items that are added to, moved within,
  or removed from a Grid item. The \l add and \l move properties can be set to
  the transitions that will be applied when items are added to, removed from,
  or re-positioned within a Row item.

  \section1 Limitations

  Note that the positioner assumes that the x and y positions of its children
  will not change. If you manually change the x or y properties in script, bind
  the x or y properties, use anchors on a child of a positioner, or have the
  width of a child depend on the position of a child, then the
  positioner may exhibit strange behaviour. If you need to perform any of these
  actions, consider positioning the items without the use of a Row.

  Items with a width or height of 0 will not be positioned.

  \sa Column, Grid, Flow, Positioner, {declarative/positioners}{Positioners example}
*/
/*!
    \qmlproperty Transition QtQuick2::Row::add

    This property holds the transition to be applied when adding an
    item to the positioner. The transition will only be applied to the
    added item(s).  Positioner transitions will only affect the
    position (x, y) of items.

    For a positioner, adding an item can mean that either the object
    has been created or reparented, and thus is now a child or the
    positioner, or that the object has had its opacity increased from
    zero, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Row::move

    This property holds the transition to be applied when moving an
    item within the positioner. Positioner transitions will only affect
    the position (x, y) of items.

    This transition can be performed when other items are added or removed
    from the positioner, or when items resize themselves.

    \qml
    Row {
        id: positioner
        move: Transition {
            NumberAnimation {
                properties: "x"
                duration: 1000
            }
        }
    }
    \endqml

    \sa add, {declarative/positioners}{Positioners example}
*/
/*!
  \qmlproperty int QtQuick2::Row::spacing

  The spacing is the amount in pixels left empty between adjacent
  items. The default spacing is 0.

  \sa Grid::spacing
*/

QSGRow::QSGRow(QSGItem *parent)
: QSGBasePositioner(Horizontal, parent)
{
}
/*!
    \qmlproperty enumeration QtQuick2::Row::layoutDirection

    This property holds the layoutDirection of the row.

    Possible values:

    \list
    \o Qt.LeftToRight (default) - Items are laid out from left to right. If the width of the row is explicitly set,
    the left anchor remains to the left of the row.
    \o Qt.RightToLeft - Items are laid out from right to left. If the width of the row is explicitly set,
    the right anchor remains to the right of the row.
    \endlist

    \sa Grid::layoutDirection, Flow::layoutDirection, {declarative/righttoleft/layoutdirection}{Layout directions example}
*/

Qt::LayoutDirection QSGRow::layoutDirection() const
{
    return QSGBasePositionerPrivate::getLayoutDirection(this);
}

void QSGRow::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate* >(QSGBasePositionerPrivate::get(this));
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        // For RTL layout the positioning changes when the width changes.
        if (d->layoutDirection == Qt::RightToLeft)
            d->addItemChangeListener(d, QSGItemPrivate::Geometry);
        else
            d->removeItemChangeListener(d, QSGItemPrivate::Geometry);
        prePositioning();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}
/*!
    \qmlproperty enumeration QtQuick2::Row::effectiveLayoutDirection
    This property holds the effective layout direction of the row positioner.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the row positioner will be mirrored. However, the
    property \l {Row::layoutDirection}{layoutDirection} will remain unchanged.

    \sa Row::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/

Qt::LayoutDirection QSGRow::effectiveLayoutDirection() const
{
    return QSGBasePositionerPrivate::getEffectiveLayoutDirection(this);
}

void QSGRow::doPositioning(QSizeF *contentSize)
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate* >(QSGBasePositionerPrivate::get(this));
    int hoffset = 0;

    QList<int> hoffsets;
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (!child.item || !child.isVisible)
            continue;

        if (d->isLeftToRight()) {
            if (child.item->x() != hoffset)
                positionX(hoffset, child);
        } else {
            hoffsets << hoffset;
        }

        contentSize->setHeight(qMax(contentSize->height(), child.item->height()));

        hoffset += child.item->width();
        hoffset += spacing();
    }

    if (hoffset != 0)//If we positioned any items, undo the extra spacing from the last item
        hoffset -= spacing();
    contentSize->setWidth(hoffset);

    if (d->isLeftToRight())
        return;

    //Right to Left layout
    int end = 0;
    if (!widthValid())
        end = contentSize->width();
    else
        end = width();

    int acc = 0;
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (!child.item || !child.isVisible)
            continue;
        hoffset = end - hoffsets[acc++] - child.item->width();
        if (child.item->x() != hoffset)
            positionX(hoffset, child);
    }
}

void QSGRow::reportConflictingAnchors()
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors) {
                QSGAnchors::Anchors usedAnchors = anchors->usedAnchors();
                if (usedAnchors & QSGAnchors::LeftAnchor ||
                    usedAnchors & QSGAnchors::RightAnchor ||
                    usedAnchors & QSGAnchors::HCenterAnchor ||
                    anchors->fill() || anchors->centerIn()) {
                    d->anchorConflict = true;
                    break;
                }
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify left, right, horizontalCenter, fill or centerIn anchors for items inside Row";
}

/*!
  \qmlclass Grid QSGGrid
    \inqmlmodule QtQuick 2
  \ingroup qml-positioning-elements
  \brief The Grid item positions its children in a grid.
  \inherits Item

  The Grid item positions its child items so that they are
  aligned in a grid and are not overlapping.

  The grid positioner calculates a grid of rectangular cells of sufficient
  size to hold all items, placing the items in the cells, from left to right
  and top to bottom. Each item is positioned in the top-left corner of its
  cell with position (0, 0).

  A Grid defaults to four columns, and as many rows as are necessary to
  fit all child items. The number of rows and columns can be constrained
  by setting the \l rows and \l columns properties.

  Spacing can be added between child items by setting the \l spacing
  property. The amount of spacing applied will be the same in the
  horizontal and vertical directions.

  See \l{Using QML Positioner and Repeater Items} for more details about this item and other
  related items.

  \section1 Example Usage

  The following example demonstrates this.

  \image gridLayout_example.png

  \snippet doc/src/snippets/declarative/grid/grid.qml document

  \section1 Using Transitions

  Transitions can be used to animate items that are added to, moved within,
  or removed from a Grid item. The \l add and \l move properties can be set to
  the transitions that will be applied when items are added to, removed from,
  or re-positioned within a Grid item.

  \section1 Limitations

  Note that the positioner assumes that the x and y positions of its children
  will not change. If you manually change the x or y properties in script, bind
  the x or y properties, use anchors on a child of a positioner, or have the
  width or height of a child depend on the position of a child, then the
  positioner may exhibit strange behaviour. If you need to perform any of these
  actions, consider positioning the items without the use of a Grid.

  Items with a width or height of 0 will not be positioned.

  \sa Flow, Row, Column, Positioner, {declarative/positioners}{Positioners example}
*/
/*!
    \qmlproperty Transition QtQuick2::Grid::add

    This property holds the transition to be applied when adding an
    item to the positioner. The transition will only be applied to the
    added item(s).  Positioner transitions will only affect the
    position (x, y) of items.

    For a positioner, adding an item can mean that either the object
    has been created or reparented, and thus is now a child or the
    positioner, or that the object has had its opacity increased from
    zero, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Grid::move

    This property holds the transition to be applied when moving an
    item within the positioner. Positioner transitions will only affect
    the position (x, y) of items.

    This transition can be performed when other items are added or removed
    from the positioner, or when items resize themselves.

    \qml
    Grid {
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 1000
            }
        }
    }
    \endqml

    \sa add, {declarative/positioners}{Positioners example}
*/
/*!
  \qmlproperty int QtQuick2::Grid::spacing

  The spacing is the amount in pixels left empty between adjacent
  items. The default spacing is 0.

  The below example places a Grid containing a red, a blue and a
  green rectangle on a gray background. The area the grid positioner
  occupies is colored white. The positioner on the left has the
  no spacing (the default), and the positioner on the right has
  a spacing of 6.

  \inlineimage qml-grid-no-spacing.png
  \inlineimage qml-grid-spacing.png

  \sa rows, columns
*/
QSGGrid::QSGGrid(QSGItem *parent) :
    QSGBasePositioner(Both, parent), m_rows(-1), m_columns(-1), m_rowSpacing(-1), m_columnSpacing(-1), m_flow(LeftToRight)
{
}

/*!
    \qmlproperty int QtQuick2::Grid::columns

    This property holds the number of columns in the grid. The default
    number of columns is 4.

    If the grid does not have enough items to fill the specified
    number of columns, some columns will be of zero width.
*/

/*!
    \qmlproperty int QtQuick2::Grid::rows
    This property holds the number of rows in the grid.

    If the grid does not have enough items to fill the specified
    number of rows, some rows will be of zero width.
*/

void QSGGrid::setColumns(const int columns)
{
    if (columns == m_columns)
        return;
    m_columns = columns;
    prePositioning();
    emit columnsChanged();
}

void QSGGrid::setRows(const int rows)
{
    if (rows == m_rows)
        return;
    m_rows = rows;
    prePositioning();
    emit rowsChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::Grid::flow
    This property holds the flow of the layout.

    Possible values are:

    \list
    \o Grid.LeftToRight (default) - Items are positioned next to
       each other in the \l layoutDirection, then wrapped to the next line.
    \o Grid.TopToBottom - Items are positioned next to each
       other from top to bottom, then wrapped to the next column.
    \endlist
*/
QSGGrid::Flow QSGGrid::flow() const
{
    return m_flow;
}

void QSGGrid::setFlow(Flow flow)
{
    if (m_flow != flow) {
        m_flow = flow;
        prePositioning();
        emit flowChanged();
    }
}

/*!
    \qmlproperty int QtQuick2::Grid::rowSpacing

    This property holds the spacing in pixels between rows.

    \sa columnSpacing
    \since QtQuick2.0
*/
void QSGGrid::setRowSpacing(const int rowSpacing)
{
    if (rowSpacing == m_rowSpacing)
        return;
    m_rowSpacing = rowSpacing;
    prePositioning();
    emit rowSpacingChanged();
}

/*!
    \qmlproperty int QtQuick2::Grid::columnSpacing

    This property holds the spacing in pixels between columns.

    \sa rowSpacing
    \since QtQuick2.0
*/
void QSGGrid::setColumnSpacing(const int columnSpacing)
{
    if (columnSpacing == m_columnSpacing)
        return;
    m_columnSpacing = columnSpacing;
    prePositioning();
    emit columnSpacingChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::Grid::layoutDirection

    This property holds the layout direction of the layout.

    Possible values are:

    \list
    \o Qt.LeftToRight (default) - Items are positioned from the top to bottom,
    and left to right. The flow direction is dependent on the
    \l Grid::flow property.
    \o Qt.RightToLeft - Items are positioned from the top to bottom,
    and right to left. The flow direction is dependent on the
    \l Grid::flow property.
    \endlist

    \sa Flow::layoutDirection, Row::layoutDirection, {declarative/righttoleft/layoutdirection}{Layout directions example}
*/
Qt::LayoutDirection QSGGrid::layoutDirection() const
{
    return QSGBasePositionerPrivate::getLayoutDirection(this);
}

void QSGGrid::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        // For RTL layout the positioning changes when the width changes.
        if (d->layoutDirection == Qt::RightToLeft)
            d->addItemChangeListener(d, QSGItemPrivate::Geometry);
        else
            d->removeItemChangeListener(d, QSGItemPrivate::Geometry);
        prePositioning();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick2::Grid::effectiveLayoutDirection
    This property holds the effective layout direction of the grid positioner.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the grid positioner will be mirrored. However, the
    property \l {Grid::layoutDirection}{layoutDirection} will remain unchanged.

    \sa Grid::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/
Qt::LayoutDirection QSGGrid::effectiveLayoutDirection() const
{
    return QSGBasePositionerPrivate::getEffectiveLayoutDirection(this);
}

void QSGGrid::doPositioning(QSizeF *contentSize)
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    int c = m_columns;
    int r = m_rows;
    //Is allocating the extra QPODVector too much overhead?
    QPODVector<PositionedItem, 8> visibleItems;//we aren't concerned with invisible items
    visibleItems.reserve(positionedItems.count());
    for(int i=0; i<positionedItems.count(); i++)
        if(positionedItems[i].item && positionedItems[i].isVisible)
            visibleItems.append(positionedItems[i]);

    int numVisible = visibleItems.count();
    if (m_columns <= 0 && m_rows <= 0){
        c = 4;
        r = (numVisible+3)/4;
    } else if (m_rows <= 0){
        r = (numVisible+(m_columns-1))/m_columns;
    } else if (m_columns <= 0){
        c = (numVisible+(m_rows-1))/m_rows;
    }

    if(r==0 || c==0)
        return; //Nothing to do

    QList<int> maxColWidth;
    QList<int> maxRowHeight;
    int childIndex =0;
    if (m_flow == LeftToRight) {
        for (int i=0; i < r; i++){
            for (int j=0; j < c; j++){
                if (j==0)
                    maxRowHeight << 0;
                if (i==0)
                    maxColWidth << 0;

                if (childIndex == visibleItems.count())
                    break;

                const PositionedItem &child = visibleItems.at(childIndex++);
                if (child.item->width() > maxColWidth[j])
                    maxColWidth[j] = child.item->width();
                if (child.item->height() > maxRowHeight[i])
                    maxRowHeight[i] = child.item->height();
            }
        }
    } else {
        for (int j=0; j < c; j++){
            for (int i=0; i < r; i++){
                if (j==0)
                    maxRowHeight << 0;
                if (i==0)
                    maxColWidth << 0;

                if (childIndex == visibleItems.count())
                    break;

                const PositionedItem &child = visibleItems.at(childIndex++);
                if (child.item->width() > maxColWidth[j])
                    maxColWidth[j] = child.item->width();
                if (child.item->height() > maxRowHeight[i])
                    maxRowHeight[i] = child.item->height();
            }
        }
    }

    int columnSpacing = m_columnSpacing;
    if (columnSpacing == -1)
        columnSpacing = spacing();

    int rowSpacing = m_rowSpacing;
    if (rowSpacing == -1)
        rowSpacing = spacing();

    int widthSum = 0;
    for (int j=0; j < maxColWidth.size(); j++){
        if (j)
            widthSum += columnSpacing;
        widthSum += maxColWidth[j];
    }

    int heightSum = 0;
    for (int i=0; i < maxRowHeight.size(); i++){
        if (i)
            heightSum += rowSpacing;
        heightSum += maxRowHeight[i];
    }

    contentSize->setHeight(heightSum);
    contentSize->setWidth(widthSum);

    int end = 0;
    if (widthValid())
        end = width();
    else
        end = widthSum;

    int xoffset=0;
    if (!d->isLeftToRight())
        xoffset = end;
    int yoffset=0;
    int curRow =0;
    int curCol =0;
    for (int i = 0; i < visibleItems.count(); ++i) {
        const PositionedItem &child = visibleItems.at(i);
        int childXOffset = xoffset;
        if (!d->isLeftToRight())
            childXOffset -= child.item->width();
        if ((child.item->x() != childXOffset) || (child.item->y() != yoffset)){
            positionX(childXOffset, child);
            positionY(yoffset, child);
        }

        if (m_flow == LeftToRight) {
            if (d->isLeftToRight())
                xoffset += maxColWidth[curCol]+columnSpacing;
            else
                xoffset -= maxColWidth[curCol]+columnSpacing;
            curCol++;
            curCol%=c;
            if (!curCol){
                yoffset += maxRowHeight[curRow]+rowSpacing;
                if (d->isLeftToRight())
                    xoffset = 0;
                else
                    xoffset = end;
                curRow++;
                if (curRow>=r)
                    break;
            }
        } else {
            yoffset+=maxRowHeight[curRow]+rowSpacing;
            curRow++;
            curRow%=r;
            if (!curRow){
                if (d->isLeftToRight())
                    xoffset += maxColWidth[curCol]+columnSpacing;
                else
                    xoffset -= maxColWidth[curCol]+columnSpacing;
                yoffset=0;
                curCol++;
                if (curCol>=c)
                    break;
            }
        }
    }
}

void QSGGrid::reportConflictingAnchors()
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors && (anchors->usedAnchors() || anchors->fill() || anchors->centerIn())) {
                d->anchorConflict = true;
                break;
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify anchors for items inside Grid";
}

/*!
  \qmlclass Flow QSGFlow
    \inqmlmodule QtQuick 2
  \ingroup qml-positioning-elements
  \brief The Flow item arranges its children side by side, wrapping as necessary.
  \inherits Item

  The Flow item positions its child items like words on a page, wrapping them
  to create rows or columns of items that do not overlap.

  Spacing between items can be added using the \l spacing property.
  Transitions can be used for cases where items managed by a Column are
  added or moved. These are stored in the \l add and \l move properties
  respectively.

  See \l{Using QML Positioner and Repeater Items} for more details about this item and other
  related items.

  \section1 Example Usage

  The following example positions \l Text items within a parent item using
  a Flow item.

  \image qml-flow-snippet.png

  \snippet doc/src/snippets/declarative/flow.qml flow item

  \section1 Using Transitions

  Transitions can be used to animate items that are added to, moved within,
  or removed from a Flow item. The \l add and \l move properties can be set to
  the transitions that will be applied when items are added to, removed from,
  or re-positioned within a Flow item.

  The use of transitions with positioners is described in more detail in the
  \l{Using QML Positioner and Repeater Items#Using Transitions}{Using QML
  Positioner and Repeater Items} document.

  \section1 Limitations

  Note that the positioner assumes that the x and y positions of its children
  will not change. If you manually change the x or y properties in script, bind
  the x or y properties, use anchors on a child of a positioner, or have the
  width or height of a child depend on the position of a child, then the
  positioner may exhibit strange behaviour.  If you need to perform any of these
  actions, consider positioning the items without the use of a Flow.

  Items with a width or height of 0 will not be positioned.

  \sa Column, Row, Grid, Positioner, {declarative/positioners}{Positioners example}
*/
/*!
    \qmlproperty Transition QtQuick2::Flow::add

    This property holds the transition to be applied when adding an
    item to the positioner. The transition will only be applied to the
    added item(s).  Positioner transitions will only affect the
    position (x, y) of items.

    For a positioner, adding an item can mean that either the object
    has been created or reparented, and thus is now a child or the
    positioner, or that the object has had its opacity increased from
    zero, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Flow::move

    This property holds the transition to be applied when moving an
    item within the positioner. Positioner transitions will only affect
    the position (x, y) of items.

    This transition can be performed when other items are added or removed
    from the positioner, or when items resize themselves.

    \qml
    Flow {
        id: positioner
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                ease: "easeOutBounce"
            }
        }
    }
    \endqml

    \sa add, {declarative/positioners}{Positioners example}
*/
/*!
  \qmlproperty int QtQuick2::Flow::spacing

  spacing is the amount in pixels left empty between each adjacent
  item, and defaults to 0.

  \sa Grid::spacing
*/

class QSGFlowPrivate : public QSGBasePositionerPrivate
{
    Q_DECLARE_PUBLIC(QSGFlow)

public:
    QSGFlowPrivate()
        : QSGBasePositionerPrivate(), flow(QSGFlow::LeftToRight)
    {}

    QSGFlow::Flow flow;
};

QSGFlow::QSGFlow(QSGItem *parent)
: QSGBasePositioner(*(new QSGFlowPrivate), Both, parent)
{
    Q_D(QSGFlow);
    // Flow layout requires relayout if its own size changes too.
    d->addItemChangeListener(d, QSGItemPrivate::Geometry);
}

/*!
    \qmlproperty enumeration QtQuick2::Flow::flow
    This property holds the flow of the layout.

    Possible values are:

    \list
    \o Flow.LeftToRight (default) - Items are positioned next to
    to each other according to the \l layoutDirection until the width of the Flow
    is exceeded, then wrapped to the next line.
    \o Flow.TopToBottom - Items are positioned next to each
    other from top to bottom until the height of the Flow is exceeded,
    then wrapped to the next column.
    \endlist
*/
QSGFlow::Flow QSGFlow::flow() const
{
    Q_D(const QSGFlow);
    return d->flow;
}

void QSGFlow::setFlow(Flow flow)
{
    Q_D(QSGFlow);
    if (d->flow != flow) {
        d->flow = flow;
        prePositioning();
        emit flowChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick2::Flow::layoutDirection

    This property holds the layout direction of the layout.

    Possible values are:

    \list
    \o Qt.LeftToRight (default) - Items are positioned from the top to bottom,
    and left to right. The flow direction is dependent on the
    \l Flow::flow property.
    \o Qt.RightToLeft - Items are positioned from the top to bottom,
    and right to left. The flow direction is dependent on the
    \l Flow::flow property.
    \endlist

    \sa Grid::layoutDirection, Row::layoutDirection, {declarative/righttoleft/layoutdirection}{Layout directions example}
*/

Qt::LayoutDirection QSGFlow::layoutDirection() const
{
    Q_D(const QSGFlow);
    return d->layoutDirection;
}

void QSGFlow::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    Q_D(QSGFlow);
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        prePositioning();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick2::Flow::effectiveLayoutDirection
    This property holds the effective layout direction of the flow positioner.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the grid positioner will be mirrored. However, the
    property \l {Flow::layoutDirection}{layoutDirection} will remain unchanged.

    \sa Flow::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/

Qt::LayoutDirection QSGFlow::effectiveLayoutDirection() const
{
    return QSGBasePositionerPrivate::getEffectiveLayoutDirection(this);
}

void QSGFlow::doPositioning(QSizeF *contentSize)
{
    Q_D(QSGFlow);

    int hoffset = 0;
    int voffset = 0;
    int linemax = 0;
    QList<int> hoffsets;

    for (int i = 0; i < positionedItems.count(); ++i) {
        const PositionedItem &child = positionedItems.at(i);
        if (!child.item || !child.isVisible)
            continue;

        if (d->flow == LeftToRight)  {
            if (widthValid() && hoffset && hoffset + child.item->width() > width()) {
                hoffset = 0;
                voffset += linemax + spacing();
                linemax = 0;
            }
        } else {
            if (heightValid() && voffset && voffset + child.item->height() > height()) {
                voffset = 0;
                hoffset += linemax + spacing();
                linemax = 0;
            }
        }

        if (d->isLeftToRight()) {
            if (child.item->x() != hoffset)
                positionX(hoffset, child);
        } else {
            hoffsets << hoffset;
        }
        if (child.item->y() != voffset)
            positionY(voffset, child);

        contentSize->setWidth(qMax(contentSize->width(), hoffset + child.item->width()));
        contentSize->setHeight(qMax(contentSize->height(), voffset + child.item->height()));

        if (d->flow == LeftToRight)  {
            hoffset += child.item->width();
            hoffset += spacing();
            linemax = qMax(linemax, qCeil(child.item->height()));
        } else {
            voffset += child.item->height();
            voffset += spacing();
            linemax = qMax(linemax, qCeil(child.item->width()));
        }
    }
    if (d->isLeftToRight())
        return;

    int end;
    if (widthValid())
        end = width();
    else
        end = contentSize->width();
    int acc = 0;
    for (int i = 0; i < positionedItems.count(); ++i) {
        const PositionedItem &child = positionedItems.at(i);
        if (!child.item || !child.isVisible)
            continue;
        hoffset = end - hoffsets[acc++] - child.item->width();
        if (child.item->x() != hoffset)
            positionX(hoffset, child);
    }
}

void QSGFlow::reportConflictingAnchors()
{
    Q_D(QSGFlow);
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors && (anchors->usedAnchors() || anchors->fill() || anchors->centerIn())) {
                d->anchorConflict = true;
                break;
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify anchors for items inside Flow";
}

QT_END_NAMESPACE
