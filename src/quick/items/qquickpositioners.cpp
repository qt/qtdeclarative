/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickpositioners_p.h"
#include "qquickpositioners_p_p.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtCore/qmath.h>
#include <QtCore/qcoreapplication.h>

#include <QtQuick/private/qdeclarativestate_p.h>
#include <QtQuick/private/qdeclarativestategroup_p.h>
#include <private/qdeclarativestateoperations_p.h>
#include <QtQuick/private/qdeclarativetransition_p.h>

QT_BEGIN_NAMESPACE

static const QQuickItemPrivate::ChangeTypes watchedChanges
    = QQuickItemPrivate::Geometry
    | QQuickItemPrivate::SiblingOrder
    | QQuickItemPrivate::Visibility
    | QQuickItemPrivate::Destroyed;

void QQuickBasePositionerPrivate::watchChanges(QQuickItem *other)
{
    QQuickItemPrivate *otherPrivate = QQuickItemPrivate::get(other);
    otherPrivate->addItemChangeListener(this, watchedChanges);
}

void QQuickBasePositionerPrivate::unwatchChanges(QQuickItem* other)
{
    QQuickItemPrivate *otherPrivate = QQuickItemPrivate::get(other);
    otherPrivate->removeItemChangeListener(this, watchedChanges);
}

QQuickBasePositioner::QQuickBasePositioner(PositionerType at, QQuickItem *parent)
    : QQuickImplicitSizeItem(*(new QQuickBasePositionerPrivate), parent)
{
    Q_D(QQuickBasePositioner);
    d->init(at);
}
/*!
    \internal
    \class QQuickBasePositioner
    \brief The QQuickBasePositioner class provides a base for QQuickGraphics layouts.

    To create a QQuickGraphics Positioner, simply subclass QQuickBasePositioner and implement
    doLayout(), which is automatically called when the layout might need
    updating. In doLayout() use the setX and setY functions from QQuickBasePositioner, and the
    base class will apply the positions along with the appropriate transitions. The items to
    position are provided in order as the protected member positionedItems.

    You also need to set a PositionerType, to declare whether you are positioning the x, y or both
    for the child items. Depending on the chosen type, only x or y changes will be applied.

    Note that the subclass is responsible for adding the spacing in between items.

    Positioning is batched and synchronized with painting to reduce the number of
    calculations needed. This means that positioners may not reposition items immediately
    when changes occur, but it will have moved by the next frame.
*/

QQuickBasePositioner::QQuickBasePositioner(QQuickBasePositionerPrivate &dd, PositionerType at, QQuickItem *parent)
    : QQuickImplicitSizeItem(dd, parent)
{
    Q_D(QQuickBasePositioner);
    d->init(at);
}

QQuickBasePositioner::~QQuickBasePositioner()
{
    Q_D(QQuickBasePositioner);
    for (int i = 0; i < positionedItems.count(); ++i)
        d->unwatchChanges(positionedItems.at(i).item);
    positionedItems.clear();
}

void QQuickBasePositioner::updatePolish()
{
    Q_D(QQuickBasePositioner);
    if (d->positioningDirty)
        prePositioning();
}

qreal QQuickBasePositioner::spacing() const
{
    Q_D(const QQuickBasePositioner);
    return d->spacing;
}

void QQuickBasePositioner::setSpacing(qreal s)
{
    Q_D(QQuickBasePositioner);
    if (s == d->spacing)
        return;
    d->spacing = s;
    d->setPositioningDirty();
    emit spacingChanged();
}

QDeclarativeTransition *QQuickBasePositioner::move() const
{
    Q_D(const QQuickBasePositioner);
    return d->moveTransition;
}

void QQuickBasePositioner::setMove(QDeclarativeTransition *mt)
{
    Q_D(QQuickBasePositioner);
    if (mt == d->moveTransition)
        return;
    d->moveTransition = mt;
    emit moveChanged();
}

QDeclarativeTransition *QQuickBasePositioner::add() const
{
    Q_D(const QQuickBasePositioner);
    return d->addTransition;
}

void QQuickBasePositioner::setAdd(QDeclarativeTransition *add)
{
    Q_D(QQuickBasePositioner);
    if (add == d->addTransition)
        return;

    d->addTransition = add;
    emit addChanged();
}

void QQuickBasePositioner::componentComplete()
{
    QQuickItem::componentComplete();
    positionedItems.reserve(childItems().count());
    prePositioning();
}

void QQuickBasePositioner::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickBasePositioner);
    if (change == ItemChildAddedChange) {
        d->setPositioningDirty();
    } else if (change == ItemChildRemovedChange) {
        QQuickItem *child = value.item;
        QQuickBasePositioner::PositionedItem posItem(child);
        int idx = positionedItems.find(posItem);
        if (idx >= 0) {
            d->unwatchChanges(child);
            positionedItems.remove(idx);
        }
        d->setPositioningDirty();
    }

    QQuickItem::itemChange(change, value);
}

void QQuickBasePositioner::prePositioning()
{
    Q_D(QQuickBasePositioner);
    if (!isComponentComplete())
        return;

    if (d->doingPositioning)
        return;

    d->positioningDirty = false;
    d->doingPositioning = true;
    //Need to order children by creation order modified by stacking order
    QList<QQuickItem *> children = childItems();

    QPODVector<PositionedItem,8> oldItems;
    positionedItems.copyAndClear(oldItems);
    for (int ii = 0; ii < unpositionedItems.count(); ii++)
        oldItems.append(unpositionedItems[ii]);
    unpositionedItems.clear();

    for (int ii = 0; ii < children.count(); ++ii) {
        QQuickItem *child = children.at(ii);
        QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(child);
        PositionedItem posItem(child);
        int wIdx = oldItems.find(posItem);
        if (wIdx < 0) {
            d->watchChanges(child);
            posItem.isNew = true;
            if (!childPrivate->explicitVisible || !child->width() || !child->height()) {
                posItem.isVisible = false;
                unpositionedItems.append(posItem);
            } else {
                positionedItems.append(posItem);
            }
        } else {
            PositionedItem *item = &oldItems[wIdx];
            // Items are only omitted from positioning if they are explicitly hidden
            // i.e. their positioning is not affected if an ancestor is hidden.
            if (!childPrivate->explicitVisible || !child->width() || !child->height()) {
                item->isVisible = false;
                unpositionedItems.append(*item);
            } else if (!item->isVisible) {
                item->isVisible = true;
                item->isNew = true;
                positionedItems.append(*item);
            } else {
                item->isNew = false;
                positionedItems.append(*item);
            }
        }
    }
    QSizeF contentSize(0,0);
    reportConflictingAnchors();
    if (!d->anchorConflict) {
        doPositioning(&contentSize);
        updateAttachedProperties();
    }
    if (!d->addActions.isEmpty() || !d->moveActions.isEmpty())
        finishApplyTransitions();
    d->doingPositioning = false;
    //Set implicit size to the size of its children
    setImplicitSize(contentSize.width(), contentSize.height());
}

void QQuickBasePositioner::positionX(qreal x, const PositionedItem &target)
{
    Q_D(QQuickBasePositioner);
    if (d->type == Horizontal || d->type == Both) {
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

void QQuickBasePositioner::positionY(qreal y, const PositionedItem &target)
{
    Q_D(QQuickBasePositioner);
    if (d->type == Vertical || d->type == Both) {
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

void QQuickBasePositioner::finishApplyTransitions()
{
    Q_D(QQuickBasePositioner);
    // Note that if a transition is not set the transition manager will
    // apply the changes directly, in the case add/move aren't set
    d->addTransitionManager.transition(d->addActions, d->addTransition);
    d->moveTransitionManager.transition(d->moveActions, d->moveTransition);
    d->addActions.clear();
    d->moveActions.clear();
}

QQuickPositionerAttached *QQuickBasePositioner::qmlAttachedProperties(QObject *obj)
{
    return new QQuickPositionerAttached(obj);
}

void QQuickBasePositioner::updateAttachedProperties(QQuickPositionerAttached *specificProperty, QQuickItem *specificPropertyOwner) const
{
    // If this function is deemed too expensive or shows up in profiles, it could
    // be changed to run only when there are attached properties present. This
    // could be a flag in the positioner that is set by the attached property
    // constructor.
    QQuickPositionerAttached *prevLastProperty = 0;
    QQuickPositionerAttached *lastProperty = 0;

    int visibleItemIndex = 0;
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (!child.item)
            continue;

        QQuickPositionerAttached *property = 0;

        if (specificProperty) {
            if (specificPropertyOwner == child.item) {
                property = specificProperty;
            }
        } else {
            property = static_cast<QQuickPositionerAttached *>(qmlAttachedPropertiesObject<QQuickBasePositioner>(child.item, false));
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
    \qmlclass Positioner QQuickPositionerAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-positioning-elements
    \brief The Positioner type provides attached properties that contain details on where an item exists in a positioner.

    Positioner is an attached property that is attached to the top-level child item within a
    Column, Row, Flow or Grid. It provides properties that allow a child item to determine
    where it exists within the layout of its parent Column, Row, Flow or Grid.

    For example, below is a \l Grid with 16 child rectangles, as created through a \l Repeater.
    Each \l Rectangle displays its index in the Grid using \l Positioner.index, and the first
    item is colored differently by taking \l Positioner.isFirstItem into account:

    \code
    Grid {
        Repeater {
            model: 16

            Rectangle {
                id: rect
                width: 30; height: 30
                border.width: 1
                color: Positioner.isFirstItem ? "yellow" : "lightsteelblue"

                Text { text: rect.Positioner.index }
            }
        }
    }
    \endcode

    \image positioner-example.png
*/

QQuickPositionerAttached::QQuickPositionerAttached(QObject *parent) : QObject(parent), m_index(-1), m_isFirstItem(false), m_isLastItem(false)
{
    QQuickItem *attachedItem = qobject_cast<QQuickItem *>(parent);
    if (attachedItem) {
        QQuickBasePositioner *positioner = qobject_cast<QQuickBasePositioner *>(attachedItem->parent());
        if (positioner) {
            positioner->updateAttachedProperties(this, attachedItem);
        }
    }
}

/*!
    \qmlattachedproperty int QtQuick2::Positioner::index

    This property allows the item to determine
    its index within the positioner.
*/
void QQuickPositionerAttached::setIndex(int index)
{
    if (m_index == index)
        return;
    m_index = index;
    emit indexChanged();
}

/*!
    \qmlattachedproperty bool QtQuick2::Positioner::isFirstItem
    \qmlattachedproperty bool QtQuick2::Positioner::isLastItem

    These properties allow the item to determine if it
    is the first or last item in the positioner, respectively.
*/
void QQuickPositionerAttached::setIsFirstItem(bool isFirstItem)
{
    if (m_isFirstItem == isFirstItem)
        return;
    m_isFirstItem = isFirstItem;
    emit isFirstItemChanged();
}

void QQuickPositionerAttached::setIsLastItem(bool isLastItem)
{
    if (m_isLastItem == isLastItem)
        return;
    m_isLastItem = isLastItem;
    emit isLastItemChanged();
}

/*!
    \qmlclass Column QQuickColumn
    \inqmlmodule QtQuick 2
    \ingroup qml-positioning-elements
    \brief The Column element positions its children in a column.
    \inherits Item

    Column is an element that positions its child items along a single column.
    It can be used as a convenient way to vertically position a series of items without
    using \l {Anchor-based Layout in QML}{anchors}.

    Below is a Column that contains three rectangles of various sizes:

    \snippet doc/src/snippets/declarative/column/vertical-positioner.qml document

    The Column automatically positions these items in a vertical formation, like this:

    \image verticalpositioner_example.png

    If an item within a Column is not \l {Item::}{visible}, or if it has a width or
    height of 0, the item will not be laid out and it will not be visible within the
    column. Also, since a Column automatically positions its children vertically, a child
    item within a Column should not set its \l {Item::y}{y} position or vertically
    anchor itself using the \l {Item::anchors.top}{top}, \l {Item::anchors.bottom}{bottom},
    \l {Item::anchors.verticalCenter}{anchors.verticalCenter}, \l {Item::anchors.fill}{fill}
    or \l {Item::anchors.centerIn}{centerIn} anchors. If you need to perform these actions,
    consider positioning the items without the use of a Column.

    Note that items in a Column can use the \l Positioner attached property to access
    more information about its position within the Column.

    For more information on using Column and other related positioner-type elements, see
    \l{Item Layouts}.


    \section1 Using Transitions

    A Column animate items using specific transitions when items are added to or moved
    within a Column.

    For example, the Column below sets the \l move property to a specific \l Transition:

    \snippet doc/src/snippets/declarative/column/column-transitions.qml document

    When the Space key is pressed, the \l {Item::visible}{visible} value of the green
    \l Rectangle is toggled. As it appears and disappears, the blue \l Rectangle moves within
    the Column, and the \l move transition is automatically applied to the blue \l Rectangle:

    \image verticalpositioner_transition.gif

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
    positioner, or that the object has changed its \l visible property
    from false to true, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Column::move

    This property holds the transition to apply to any item that has moved
    within the positioner. Positioner transitions will only affect
    the position (x, y) of items.

    This transition is applied to items that are displaced as a result of the
    addition or removal of other items in the positioner, or when items move due to
    a move operation in a related model, or when items resize themselves.

    \sa add, {declarative/positioners}{Positioners example}
*/
/*!
  \qmlproperty real QtQuick2::Column::spacing

  The spacing is the amount in pixels left empty between adjacent
  items. The default spacing is 0.

  \sa Grid::spacing
*/
QQuickColumn::QQuickColumn(QQuickItem *parent)
: QQuickBasePositioner(Vertical, parent)
{
}

void QQuickColumn::doPositioning(QSizeF *contentSize)
{
    //Precondition: All items in the positioned list have a valid item pointer and should be positioned
    qreal voffset = 0;

    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);

        if (child.item->y() != voffset)
            positionY(voffset, child);

        contentSize->setWidth(qMax(contentSize->width(), child.item->width()));

        voffset += child.item->height();
        voffset += spacing();
    }

    if (voffset != 0)//If we positioned any items, undo the spacing from the last item
        voffset -= spacing();
    contentSize->setHeight(voffset);
}

void QQuickColumn::reportConflictingAnchors()
{
    QQuickBasePositionerPrivate *d = static_cast<QQuickBasePositionerPrivate*>(QQuickBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QQuickAnchors *anchors = QQuickItemPrivate::get(static_cast<QQuickItem *>(child.item))->_anchors;
            if (anchors) {
                QQuickAnchors::Anchors usedAnchors = anchors->usedAnchors();
                if (usedAnchors & QQuickAnchors::TopAnchor ||
                    usedAnchors & QQuickAnchors::BottomAnchor ||
                    usedAnchors & QQuickAnchors::VCenterAnchor ||
                    anchors->fill() || anchors->centerIn()) {
                    d->anchorConflict = true;
                    break;
                }
            }
        }
    }
    if (d->anchorConflict) {
        qmlInfo(this) << "Cannot specify top, bottom, verticalCenter, fill or centerIn anchors for items inside Column."
            << " Column will not function.";
    }
}
/*!
    \qmlclass Row QQuickRow
    \inqmlmodule QtQuick 2
    \ingroup qml-positioning-elements
    \brief The Row element positions its children in a row.
    \inherits Item

    Row is an element that positions its child items along a single row.
    It can be used as a convenient way to horizontally position a series of items without
    using \l {Anchor-based Layout in QML}{anchors}.

    Below is a Row that contains three rectangles of various sizes:

    \snippet doc/src/snippets/declarative/row/row.qml document

    The Row automatically positions these items in a horizontal formation, like this:

    \image horizontalpositioner_example.png

    If an item within a Row is not \l {Item::}{visible}, or if it has a width or
    height of 0, the item will not be laid out and it will not be visible within the
    row. Also, since a Row automatically positions its children horizontally, a child
    item within a Row should not set its \l {Item::x}{x} position or horizontally
    anchor itself using the \l {Item::anchors.left}{left}, \l {Item::anchors.right}{right},
    \l {Item::anchors.horizontalCenter}{anchors.horizontalCenter}, \l {Item::anchors.fill}{fill}
    or \l {Item::anchors.centerIn}{centerIn} anchors. If you need to perform these actions,
    consider positioning the items without the use of a Row.

    Note that items in a Row can use the \l Positioner attached property to access
    more information about its position within the Row.

    For more information on using Row and other related positioner-type elements, see
    \l{Item Layouts}.


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
    positioner, or that the object has changed its \l visible property
    from false to true, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Row::move

    This property holds the transition to apply to any item that has moved
    within the positioner. Positioner transitions will only affect
    the position (x, y) of items.

    This transition is applied to items that are displaced as a result of the
    addition or removal of other items in the positioner, or when items move due to
    a move operation in a related model, or when items resize themselves.

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
  \qmlproperty real QtQuick2::Row::spacing

  The spacing is the amount in pixels left empty between adjacent
  items. The default spacing is 0.

  \sa Grid::spacing
*/

QQuickRow::QQuickRow(QQuickItem *parent)
: QQuickBasePositioner(Horizontal, parent)
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

Qt::LayoutDirection QQuickRow::layoutDirection() const
{
    return QQuickBasePositionerPrivate::getLayoutDirection(this);
}

void QQuickRow::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    QQuickBasePositionerPrivate *d = static_cast<QQuickBasePositionerPrivate* >(QQuickBasePositionerPrivate::get(this));
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        // For RTL layout the positioning changes when the width changes.
        if (d->layoutDirection == Qt::RightToLeft)
            d->addItemChangeListener(d, QQuickItemPrivate::Geometry);
        else
            d->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        prePositioning();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}
/*!
    \qmlproperty enumeration QtQuick2::Row::effectiveLayoutDirection
    This property holds the effective layout direction of the row.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the row positioner will be mirrored. However, the
    property \l {Row::layoutDirection}{layoutDirection} will remain unchanged.

    \sa Row::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/

Qt::LayoutDirection QQuickRow::effectiveLayoutDirection() const
{
    return QQuickBasePositionerPrivate::getEffectiveLayoutDirection(this);
}

void QQuickRow::doPositioning(QSizeF *contentSize)
{
    //Precondition: All items in the positioned list have a valid item pointer and should be positioned
    QQuickBasePositionerPrivate *d = static_cast<QQuickBasePositionerPrivate* >(QQuickBasePositionerPrivate::get(this));
    qreal hoffset = 0;

    QList<qreal> hoffsets;
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);

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
    qreal end = 0;
    if (!widthValid())
        end = contentSize->width();
    else
        end = width();

    int acc = 0;
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        hoffset = end - hoffsets[acc++] - child.item->width();
        if (child.item->x() != hoffset)
            positionX(hoffset, child);
    }
}

void QQuickRow::reportConflictingAnchors()
{
    QQuickBasePositionerPrivate *d = static_cast<QQuickBasePositionerPrivate*>(QQuickBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QQuickAnchors *anchors = QQuickItemPrivate::get(static_cast<QQuickItem *>(child.item))->_anchors;
            if (anchors) {
                QQuickAnchors::Anchors usedAnchors = anchors->usedAnchors();
                if (usedAnchors & QQuickAnchors::LeftAnchor ||
                    usedAnchors & QQuickAnchors::RightAnchor ||
                    usedAnchors & QQuickAnchors::HCenterAnchor ||
                    anchors->fill() || anchors->centerIn()) {
                    d->anchorConflict = true;
                    break;
                }
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify left, right, horizontalCenter, fill or centerIn anchors for items inside Row."
            << " Row will not function.";
}

/*!
    \qmlclass Grid QQuickGrid
    \inqmlmodule QtQuick 2
    \ingroup qml-positioning-elements
    \brief The Grid element positions its children in grid formation.
    \inherits Item

    Grid is an element that positions its child items in grid formation.

    A Grid creates a grid of cells that is large enough to hold all of its
    child items, and places these items in the cells from left to right
    and top to bottom. Each item is positioned at the top-left corner of its
    cell with position (0, 0).

    A Grid defaults to four columns, and creates as many rows as are necessary to
    fit all of its child items. The number of rows and columns can be constrained
    by setting the \l rows and \l columns properties.

    For example, below is a Grid that contains five rectangles of various sizes:

    \snippet doc/src/snippets/declarative/grid/grid.qml document

    The Grid automatically positions the child items in a grid formation:

    \image gridLayout_example.png

    If an item within a Column is not \l {Item::}{visible}, or if it has a width or
    height of 0, the item will not be laid out and it will not be visible within the
    column. Also, since a Grid automatically positions its children, a child
    item within a Grid should not set its \l {Item::x}{x} or \l {Item::y}{y} positions
    or anchor itself with any of the \l {Item::anchors}{anchor} properties.

    For more information on using Grid and other related positioner-type elements, see
    \l{Item Layouts}.


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
    positioner, or that the object has changed its \l visible property
    from false to true, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Grid::move

    This property holds the transition to apply to any item that has moved
    within the positioner. Positioner transitions will only affect
    the position (x, y) of items.

    This transition is applied to items that are displaced as a result of the
    addition or removal of other items in the positioner, or when items move due to
    a move operation in a related model, or when items resize themselves.

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
  \qmlproperty qreal QtQuick2::Grid::spacing

  The spacing is the amount in pixels left empty between adjacent
  items. The amount of spacing applied will be the same in the
  horizontal and vertical directions. The default spacing is 0.

  The below example places a Grid containing a red, a blue and a
  green rectangle on a gray background. The area the grid positioner
  occupies is colored white. The positioner on the left has the
  no spacing (the default), and the positioner on the right has
  a spacing of 6.

  \inlineimage qml-grid-no-spacing.png
  \inlineimage qml-grid-spacing.png

  \sa rows, columns
*/
QQuickGrid::QQuickGrid(QQuickItem *parent)
    : QQuickBasePositioner(Both, parent)
    , m_rows(-1)
    , m_columns(-1)
    , m_rowSpacing(-1)
    , m_columnSpacing(-1)
    , m_useRowSpacing(false)
    , m_useColumnSpacing(false)
    , m_flow(LeftToRight)
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

void QQuickGrid::setColumns(const int columns)
{
    if (columns == m_columns)
        return;
    m_columns = columns;
    prePositioning();
    emit columnsChanged();
}

void QQuickGrid::setRows(const int rows)
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
QQuickGrid::Flow QQuickGrid::flow() const
{
    return m_flow;
}

void QQuickGrid::setFlow(Flow flow)
{
    if (m_flow != flow) {
        m_flow = flow;
        prePositioning();
        emit flowChanged();
    }
}

/*!
    \qmlproperty qreal QtQuick2::Grid::rowSpacing

    This property holds the spacing in pixels between rows.

    If this property is not set, then spacing is used for the row spacing.

    By default this property is not set.

    \sa columnSpacing
    \since QtQuick2.0
*/
void QQuickGrid::setRowSpacing(const qreal rowSpacing)
{
    if (rowSpacing == m_rowSpacing)
        return;
    m_rowSpacing = rowSpacing;
    m_useRowSpacing = true;
    prePositioning();
    emit rowSpacingChanged();
}

/*!
    \qmlproperty qreal QtQuick2::Grid::columnSpacing

    This property holds the spacing in pixels between columns.

    If this property is not set, then spacing is used for the column spacing.

    By default this property is not set.

    \sa rowSpacing
    \since QtQuick2.0
*/
void QQuickGrid::setColumnSpacing(const qreal columnSpacing)
{
    if (columnSpacing == m_columnSpacing)
        return;
    m_columnSpacing = columnSpacing;
    m_useColumnSpacing = true;
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
Qt::LayoutDirection QQuickGrid::layoutDirection() const
{
    return QQuickBasePositionerPrivate::getLayoutDirection(this);
}

void QQuickGrid::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    QQuickBasePositionerPrivate *d = static_cast<QQuickBasePositionerPrivate*>(QQuickBasePositionerPrivate::get(this));
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        // For RTL layout the positioning changes when the width changes.
        if (d->layoutDirection == Qt::RightToLeft)
            d->addItemChangeListener(d, QQuickItemPrivate::Geometry);
        else
            d->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        prePositioning();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick2::Grid::effectiveLayoutDirection
    This property holds the effective layout direction of the grid.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the grid positioner will be mirrored. However, the
    property \l {Grid::layoutDirection}{layoutDirection} will remain unchanged.

    \sa Grid::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/
Qt::LayoutDirection QQuickGrid::effectiveLayoutDirection() const
{
    return QQuickBasePositionerPrivate::getEffectiveLayoutDirection(this);
}

void QQuickGrid::doPositioning(QSizeF *contentSize)
{
    //Precondition: All items in the positioned list have a valid item pointer and should be positioned
    QQuickBasePositionerPrivate *d = static_cast<QQuickBasePositionerPrivate*>(QQuickBasePositionerPrivate::get(this));
    int c = m_columns;
    int r = m_rows;
    int numVisible = positionedItems.count();

    if (m_columns <= 0 && m_rows <= 0) {
        c = 4;
        r = (numVisible+3)/4;
    } else if (m_rows <= 0) {
        r = (numVisible+(m_columns-1))/m_columns;
    } else if (m_columns <= 0) {
        c = (numVisible+(m_rows-1))/m_rows;
    }

    if (r == 0 || c == 0)
        return; //Nothing to do

    QList<qreal> maxColWidth;
    QList<qreal> maxRowHeight;
    int childIndex =0;
    if (m_flow == LeftToRight) {
        for (int i = 0; i < r; i++) {
            for (int j = 0; j < c; j++) {
                if (j == 0)
                    maxRowHeight << 0;
                if (i == 0)
                    maxColWidth << 0;

                if (childIndex == numVisible)
                    break;

                const PositionedItem &child = positionedItems.at(childIndex++);
                if (child.item->width() > maxColWidth[j])
                    maxColWidth[j] = child.item->width();
                if (child.item->height() > maxRowHeight[i])
                    maxRowHeight[i] = child.item->height();
            }
        }
    } else {
        for (int j = 0; j < c; j++) {
            for (int i = 0; i < r; i++) {
                if (j == 0)
                    maxRowHeight << 0;
                if (i == 0)
                    maxColWidth << 0;

                if (childIndex == numVisible)
                    break;

                const PositionedItem &child = positionedItems.at(childIndex++);
                if (child.item->width() > maxColWidth[j])
                    maxColWidth[j] = child.item->width();
                if (child.item->height() > maxRowHeight[i])
                    maxRowHeight[i] = child.item->height();
            }
        }
    }

    qreal columnSpacing = m_useColumnSpacing ? m_columnSpacing : spacing();
    qreal rowSpacing = m_useRowSpacing ? m_rowSpacing : spacing();

    qreal widthSum = 0;
    for (int j = 0; j < maxColWidth.size(); j++) {
        if (j)
            widthSum += columnSpacing;
        widthSum += maxColWidth[j];
    }

    qreal heightSum = 0;
    for (int i = 0; i < maxRowHeight.size(); i++) {
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

    qreal xoffset = 0;
    if (!d->isLeftToRight())
        xoffset = end;
    qreal yoffset = 0;
    int curRow =0;
    int curCol =0;
    for (int i = 0; i < positionedItems.count(); ++i) {
        const PositionedItem &child = positionedItems.at(i);
        qreal childXOffset = xoffset;
        if (!d->isLeftToRight())
            childXOffset -= child.item->width();
        if ((child.item->x() != childXOffset) || (child.item->y() != yoffset)) {
            positionX(childXOffset, child);
            positionY(yoffset, child);
        }

        if (m_flow == LeftToRight) {
            if (d->isLeftToRight())
                xoffset += maxColWidth[curCol]+columnSpacing;
            else
                xoffset -= maxColWidth[curCol]+columnSpacing;
            curCol++;
            curCol %= c;
            if (!curCol) {
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
            yoffset += maxRowHeight[curRow]+rowSpacing;
            curRow++;
            curRow %= r;
            if (!curRow) {
                if (d->isLeftToRight())
                    xoffset += maxColWidth[curCol]+columnSpacing;
                else
                    xoffset -= maxColWidth[curCol]+columnSpacing;
                yoffset = 0;
                curCol++;
                if (curCol>=c)
                    break;
            }
        }
    }
}

void QQuickGrid::reportConflictingAnchors()
{
    QQuickBasePositionerPrivate *d = static_cast<QQuickBasePositionerPrivate*>(QQuickBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QQuickAnchors *anchors = QQuickItemPrivate::get(static_cast<QQuickItem *>(child.item))->_anchors;
            if (anchors && (anchors->usedAnchors() || anchors->fill() || anchors->centerIn())) {
                d->anchorConflict = true;
                break;
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify anchors for items inside Grid." << " Grid will not function.";
}

/*!
    \qmlclass Flow QQuickFlow
    \inqmlmodule QtQuick 2
    \ingroup qml-positioning-elements
    \brief The Flow element positions its children side by side, wrapping as necessary.
    \inherits Item

    The Flow item positions its child items like words on a page, wrapping them
    to create rows or columns of items.

    Below is a Flow that contains various \l Text items:

    \snippet doc/src/snippets/declarative/flow.qml flow item

    The Flow item automatically positions the child \l Text items side by
    side, wrapping as necessary:

    \image qml-flow-snippet.png

    If an item within a Flow is not \l {Item::}{visible}, or if it has a width or
    height of 0, the item will not be laid out and it will not be visible within the
    Flow. Also, since a Flow automatically positions its children, a child
    item within a Flow should not set its \l {Item::x}{x} or \l {Item::y}{y} positions
    or anchor itself with any of the \l {Item::anchors}{anchor} properties.

    For more information on using Flow and other related positioner-type elements, see
    \l{Item Layouts}.

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
    positioner, or that the object has changed its \l visible property
    from false to true, and thus is now visible.

    \sa move
*/
/*!
    \qmlproperty Transition QtQuick2::Flow::move

    This property holds the transition to apply to any item that has moved
    within the positioner. Positioner transitions will only affect
    the position (x, y) of items.

    This transition is applied to items that are displaced as a result of the
    addition or removal of other items in the positioner, or when items move due to
    a move operation in a related model, or when items resize themselves.

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
  \qmlproperty real QtQuick2::Flow::spacing

  spacing is the amount in pixels left empty between each adjacent
  item, and defaults to 0.

  \sa Grid::spacing
*/

class QQuickFlowPrivate : public QQuickBasePositionerPrivate
{
    Q_DECLARE_PUBLIC(QQuickFlow)

public:
    QQuickFlowPrivate()
        : QQuickBasePositionerPrivate(), flow(QQuickFlow::LeftToRight)
    {}

    QQuickFlow::Flow flow;
};

QQuickFlow::QQuickFlow(QQuickItem *parent)
: QQuickBasePositioner(*(new QQuickFlowPrivate), Both, parent)
{
    Q_D(QQuickFlow);
    // Flow layout requires relayout if its own size changes too.
    d->addItemChangeListener(d, QQuickItemPrivate::Geometry);
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
QQuickFlow::Flow QQuickFlow::flow() const
{
    Q_D(const QQuickFlow);
    return d->flow;
}

void QQuickFlow::setFlow(Flow flow)
{
    Q_D(QQuickFlow);
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

Qt::LayoutDirection QQuickFlow::layoutDirection() const
{
    Q_D(const QQuickFlow);
    return d->layoutDirection;
}

void QQuickFlow::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    Q_D(QQuickFlow);
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        prePositioning();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick2::Flow::effectiveLayoutDirection
    This property holds the effective layout direction of the flow.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the grid positioner will be mirrored. However, the
    property \l {Flow::layoutDirection}{layoutDirection} will remain unchanged.

    \sa Flow::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/

Qt::LayoutDirection QQuickFlow::effectiveLayoutDirection() const
{
    return QQuickBasePositionerPrivate::getEffectiveLayoutDirection(this);
}

void QQuickFlow::doPositioning(QSizeF *contentSize)
{
    //Precondition: All items in the positioned list have a valid item pointer and should be positioned
    Q_D(QQuickFlow);

    qreal hoffset = 0;
    qreal voffset = 0;
    qreal linemax = 0;
    QList<qreal> hoffsets;

    for (int i = 0; i < positionedItems.count(); ++i) {
        const PositionedItem &child = positionedItems.at(i);

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
            linemax = qMax(linemax, child.item->height());
        } else {
            voffset += child.item->height();
            voffset += spacing();
            linemax = qMax(linemax, child.item->width());
        }
    }
    if (d->isLeftToRight())
        return;

    qreal end;
    if (widthValid())
        end = width();
    else
        end = contentSize->width();
    int acc = 0;
    for (int i = 0; i < positionedItems.count(); ++i) {
        const PositionedItem &child = positionedItems.at(i);
        hoffset = end - hoffsets[acc++] - child.item->width();
        if (child.item->x() != hoffset)
            positionX(hoffset, child);
    }
}

void QQuickFlow::reportConflictingAnchors()
{
    Q_D(QQuickFlow);
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QQuickAnchors *anchors = QQuickItemPrivate::get(static_cast<QQuickItem *>(child.item))->_anchors;
            if (anchors && (anchors->usedAnchors() || anchors->fill() || anchors->centerIn())) {
                d->anchorConflict = true;
                break;
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify anchors for items inside Flow." << " Flow will not function.";
}

QT_END_NAMESPACE
