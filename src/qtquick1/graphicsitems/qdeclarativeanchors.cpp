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

#include "QtQuick1/private/qdeclarativeanchors_p_p.h"

#include "QtQuick1/qdeclarativeitem.h"
#include "QtQuick1/private/qdeclarativeitem_p.h"

#include <QtDeclarative/qdeclarativeinfo.h>

#include <QDebug>

QT_BEGIN_NAMESPACE



//TODO: should we cache relationships, so we don't have to check each time (parent-child or sibling)?
//TODO: support non-parent, non-sibling (need to find lowest common ancestor)

static qreal hcenter(QGraphicsItem *i)
{
    QGraphicsItemPrivate *item = QGraphicsItemPrivate::get(i);

    qreal width = item->width();
    int iw = width;
    if (iw % 2)
        return (width + 1) / 2;
    else
        return width / 2;
}

static qreal vcenter(QGraphicsItem *i)
{
    QGraphicsItemPrivate *item = QGraphicsItemPrivate::get(i);

    qreal height = item->height();
    int ih = height;
    if (ih % 2)
        return (height + 1) / 2;
    else
        return height / 2;
}

//### const item?
//local position
static qreal position(QGraphicsObject *item, QDeclarative1AnchorLine::AnchorLine anchorLine)
{
    qreal ret = 0.0;
    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(item);
    switch(anchorLine) {
    case QDeclarative1AnchorLine::Left:
        ret = item->x();
        break;
    case QDeclarative1AnchorLine::Right:
        ret = item->x() + d->width();
        break;
    case QDeclarative1AnchorLine::Top:
        ret = item->y();
        break;
    case QDeclarative1AnchorLine::Bottom:
        ret = item->y() + d->height();
        break;
    case QDeclarative1AnchorLine::HCenter:
        ret = item->x() + hcenter(item);
        break;
    case QDeclarative1AnchorLine::VCenter:
        ret = item->y() + vcenter(item);
        break;
    case QDeclarative1AnchorLine::Baseline:
        if (d->isDeclarativeItem)
            ret = item->y() + static_cast<QDeclarativeItem*>(item)->baselineOffset();
        break;
    default:
        break;
    }

    return ret;
}

//position when origin is 0,0
static qreal adjustedPosition(QGraphicsObject *item, QDeclarative1AnchorLine::AnchorLine anchorLine)
{
    qreal ret = 0.0;
    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(item);
    switch(anchorLine) {
    case QDeclarative1AnchorLine::Left:
        ret = 0.0;
        break;
    case QDeclarative1AnchorLine::Right:
        ret = d->width();
        break;
    case QDeclarative1AnchorLine::Top:
        ret = 0.0;
        break;
    case QDeclarative1AnchorLine::Bottom:
        ret = d->height();
        break;
    case QDeclarative1AnchorLine::HCenter:
        ret = hcenter(item);
        break;
    case QDeclarative1AnchorLine::VCenter:
        ret = vcenter(item);
        break;
    case QDeclarative1AnchorLine::Baseline:
        if (d->isDeclarativeItem)
            ret = static_cast<QDeclarativeItem*>(item)->baselineOffset();
        break;
    default:
        break;
    }

    return ret;
}

QDeclarative1Anchors::QDeclarative1Anchors(QObject *parent)
  : QObject(*new QDeclarative1AnchorsPrivate(0), parent)
{
    qFatal("QDeclarative1Anchors::QDeclarative1Anchors(QObject*) called");
}

QDeclarative1Anchors::QDeclarative1Anchors(QGraphicsObject *item, QObject *parent)
  : QObject(*new QDeclarative1AnchorsPrivate(item), parent)
{
}

QDeclarative1Anchors::~QDeclarative1Anchors()
{
    Q_D(QDeclarative1Anchors);
    d->remDepend(d->fill);
    d->remDepend(d->centerIn);
    d->remDepend(d->left.item);
    d->remDepend(d->right.item);
    d->remDepend(d->top.item);
    d->remDepend(d->bottom.item);
    d->remDepend(d->vCenter.item);
    d->remDepend(d->hCenter.item);
    d->remDepend(d->baseline.item);
}

void QDeclarative1AnchorsPrivate::fillChanged()
{
    Q_Q(QDeclarative1Anchors);
    if (!fill || !isItemComplete())
        return;

    if (updatingFill < 2) {
        ++updatingFill;

        qreal horizontalMargin = q->mirrored() ? rightMargin : leftMargin;

        if (fill == item->parentItem()) {                         //child-parent
            setItemPos(QPointF(horizontalMargin, topMargin));
        } else if (fill->parentItem() == item->parentItem()) {   //siblings
            setItemPos(QPointF(fill->x()+horizontalMargin, fill->y()+topMargin));
        }
        QGraphicsItemPrivate *fillPrivate = QGraphicsItemPrivate::get(fill);
        setItemSize(QSizeF(fillPrivate->width()-leftMargin-rightMargin, fillPrivate->height()-topMargin-bottomMargin));

        --updatingFill;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QDeclarative1Anchors::tr("Possible anchor loop detected on fill.");
    }

}

void QDeclarative1AnchorsPrivate::centerInChanged()
{
    Q_Q(QDeclarative1Anchors);
    if (!centerIn || fill || !isItemComplete())
        return;

    if (updatingCenterIn < 2) {
        ++updatingCenterIn;

        qreal effectiveHCenterOffset = q->mirrored() ? -hCenterOffset : hCenterOffset;
        if (centerIn == item->parentItem()) {
            QPointF p(hcenter(item->parentItem()) - hcenter(item) + effectiveHCenterOffset,
                      vcenter(item->parentItem()) - vcenter(item) + vCenterOffset);
            setItemPos(p);

        } else if (centerIn->parentItem() == item->parentItem()) {
            QPointF p(centerIn->x() + hcenter(centerIn) - hcenter(item) + effectiveHCenterOffset,
                      centerIn->y() + vcenter(centerIn) - vcenter(item) + vCenterOffset);
            setItemPos(p);
        }

        --updatingCenterIn;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QDeclarative1Anchors::tr("Possible anchor loop detected on centerIn.");
    }
}

void QDeclarative1AnchorsPrivate::clearItem(QGraphicsObject *item)
{
    if (!item)
        return;
    if (fill == item)
        fill = 0;
    if (centerIn == item)
        centerIn = 0;
    if (left.item == item) {
        left.item = 0;
        usedAnchors &= ~QDeclarative1Anchors::LeftAnchor;
    }
    if (right.item == item) {
        right.item = 0;
        usedAnchors &= ~QDeclarative1Anchors::RightAnchor;
    }
    if (top.item == item) {
        top.item = 0;
        usedAnchors &= ~QDeclarative1Anchors::TopAnchor;
    }
    if (bottom.item == item) {
        bottom.item = 0;
        usedAnchors &= ~QDeclarative1Anchors::BottomAnchor;
    }
    if (vCenter.item == item) {
        vCenter.item = 0;
        usedAnchors &= ~QDeclarative1Anchors::VCenterAnchor;
    }
    if (hCenter.item == item) {
        hCenter.item = 0;
        usedAnchors &= ~QDeclarative1Anchors::HCenterAnchor;
    }
    if (baseline.item == item) {
        baseline.item = 0;
        usedAnchors &= ~QDeclarative1Anchors::BaselineAnchor;
    }
}

void QDeclarative1AnchorsPrivate::addDepend(QGraphicsObject *item)
{
    if (!item)
        return;
    QGraphicsItemPrivate * itemPrivate = QGraphicsItemPrivate::get(item);
    if (itemPrivate->isDeclarativeItem) {
        QDeclarativeItemPrivate *p =
            static_cast<QDeclarativeItemPrivate *>(QGraphicsItemPrivate::get(item));
        p->addItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
    } else if(itemPrivate->isWidget) {
        Q_Q(QDeclarative1Anchors);
        QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
        QObject::connect(widget, SIGNAL(destroyed(QObject*)), q, SLOT(_q_widgetDestroyed(QObject*)));
        QObject::connect(widget, SIGNAL(geometryChanged()), q, SLOT(_q_widgetGeometryChanged()));
    }
}

void QDeclarative1AnchorsPrivate::remDepend(QGraphicsObject *item)
{
    if (!item)
        return;
    QGraphicsItemPrivate * itemPrivate = QGraphicsItemPrivate::get(item);
    if (itemPrivate->isDeclarativeItem) {
        QDeclarativeItemPrivate *p =
            static_cast<QDeclarativeItemPrivate *>(itemPrivate);
        p->removeItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
    } else if(itemPrivate->isWidget) {
        Q_Q(QDeclarative1Anchors);
        QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
        QObject::disconnect(widget, SIGNAL(destroyed(QObject*)), q, SLOT(_q_widgetDestroyed(QObject*)));
        QObject::disconnect(widget, SIGNAL(geometryChanged()), q, SLOT(_q_widgetGeometryChanged()));
    }
}

bool QDeclarative1AnchorsPrivate::isItemComplete() const
{
    return componentComplete;
}

void QDeclarative1Anchors::classBegin()
{
    Q_D(QDeclarative1Anchors);
    d->componentComplete = false;
}

void QDeclarative1Anchors::componentComplete()
{
    Q_D(QDeclarative1Anchors);
    d->componentComplete = true;
}

bool QDeclarative1Anchors::mirrored()
{
    Q_D(QDeclarative1Anchors);
    QGraphicsItemPrivate * itemPrivate = QGraphicsItemPrivate::get(d->item);
    return itemPrivate->isDeclarativeItem ? static_cast<QDeclarativeItemPrivate *>(itemPrivate)->effectiveLayoutMirror : false;
}

void QDeclarative1AnchorsPrivate::setItemHeight(qreal v)
{
    updatingMe = true;
    QGraphicsItemPrivate::get(item)->setHeight(v);
    updatingMe = false;
}

void QDeclarative1AnchorsPrivate::setItemWidth(qreal v)
{
    updatingMe = true;
    QGraphicsItemPrivate::get(item)->setWidth(v);
    updatingMe = false;
}

void QDeclarative1AnchorsPrivate::setItemX(qreal v)
{
    updatingMe = true;
    item->setX(v);
    updatingMe = false;
}

void QDeclarative1AnchorsPrivate::setItemY(qreal v)
{
    updatingMe = true;
    item->setY(v);
    updatingMe = false;
}

void QDeclarative1AnchorsPrivate::setItemPos(const QPointF &v)
{
    updatingMe = true;
    item->setPos(v);
    updatingMe = false;
}

void QDeclarative1AnchorsPrivate::setItemSize(const QSizeF &v)
{
    updatingMe = true;
    if(QGraphicsItemPrivate::get(item)->isWidget)
        static_cast<QGraphicsWidget *>(item)->resize(v);
    else if (QGraphicsItemPrivate::get(item)->isDeclarativeItem)
        static_cast<QDeclarativeItem *>(item)->setSize(v);
    updatingMe = false;
}

void QDeclarative1AnchorsPrivate::updateMe()
{
    if (updatingMe) {
        updatingMe = false;
        return;
    }

    fillChanged();
    centerInChanged();
    updateHorizontalAnchors();
    updateVerticalAnchors();
}

void QDeclarative1AnchorsPrivate::updateOnComplete()
{
    fillChanged();
    centerInChanged();
    updateHorizontalAnchors();
    updateVerticalAnchors();
}

void QDeclarative1AnchorsPrivate::_q_widgetDestroyed(QObject *obj)
{
    clearItem(qobject_cast<QGraphicsObject*>(obj));
}

void QDeclarative1AnchorsPrivate::_q_widgetGeometryChanged()
{
    fillChanged();
    centerInChanged();
    updateHorizontalAnchors();
    updateVerticalAnchors();
}

void QDeclarative1AnchorsPrivate::itemGeometryChanged(QDeclarativeItem *, const QRectF &newG, const QRectF &oldG)
{
    fillChanged();
    centerInChanged();
    if (newG.x() != oldG.x() || newG.width() != oldG.width())
        updateHorizontalAnchors();
    if (newG.y() != oldG.y() || newG.height() != oldG.height())
        updateVerticalAnchors();
}

QGraphicsObject *QDeclarative1Anchors::fill() const
{
    Q_D(const QDeclarative1Anchors);
    return d->fill;
}

void QDeclarative1Anchors::setFill(QGraphicsObject *f)
{
    Q_D(QDeclarative1Anchors);
    if (d->fill == f)
        return;

    if (!f) {
        d->remDepend(d->fill);
        d->fill = f;
        emit fillChanged();
        return;
    }
    if (f != d->item->parentItem() && f->parentItem() != d->item->parentItem()){
        qmlInfo(d->item) << tr("Cannot anchor to an item that isn't a parent or sibling.");
        return;
    }
    d->remDepend(d->fill);
    d->fill = f;
    d->addDepend(d->fill);
    emit fillChanged();
    d->fillChanged();
}

void QDeclarative1Anchors::resetFill()
{
    setFill(0);
}

QGraphicsObject *QDeclarative1Anchors::centerIn() const
{
    Q_D(const QDeclarative1Anchors);
    return d->centerIn;
}

void QDeclarative1Anchors::setCenterIn(QGraphicsObject* c)
{
    Q_D(QDeclarative1Anchors);
    if (d->centerIn == c)
        return;

    if (!c) {
        d->remDepend(d->centerIn);
        d->centerIn = c;
        emit centerInChanged();
        return;
    }
    if (c != d->item->parentItem() && c->parentItem() != d->item->parentItem()){
        qmlInfo(d->item) << tr("Cannot anchor to an item that isn't a parent or sibling.");
        return;
    }

    d->remDepend(d->centerIn);
    d->centerIn = c;
    d->addDepend(d->centerIn);
    emit centerInChanged();
    d->centerInChanged();
}

void QDeclarative1Anchors::resetCenterIn()
{
    setCenterIn(0);
}

bool QDeclarative1AnchorsPrivate::calcStretch(const QDeclarative1AnchorLine &edge1,
                                    const QDeclarative1AnchorLine &edge2,
                                    qreal offset1,
                                    qreal offset2,
                                    QDeclarative1AnchorLine::AnchorLine line,
                                    qreal &stretch)
{
    bool edge1IsParent = (edge1.item == item->parentItem());
    bool edge2IsParent = (edge2.item == item->parentItem());
    bool edge1IsSibling = (edge1.item->parentItem() == item->parentItem());
    bool edge2IsSibling = (edge2.item->parentItem() == item->parentItem());

    bool invalid = false;
    if ((edge2IsParent && edge1IsParent) || (edge2IsSibling && edge1IsSibling)) {
        stretch = (position(edge2.item, edge2.anchorLine) + offset2)
                    - (position(edge1.item, edge1.anchorLine) + offset1);
    } else if (edge2IsParent && edge1IsSibling) {
        stretch = (position(edge2.item, edge2.anchorLine) + offset2)
                    - (position(item->parentObject(), line)
                    + position(edge1.item, edge1.anchorLine) + offset1);
    } else if (edge2IsSibling && edge1IsParent) {
        stretch = (position(item->parentObject(), line) + position(edge2.item, edge2.anchorLine) + offset2)
                    - (position(edge1.item, edge1.anchorLine) + offset1);
    } else
        invalid = true;

    return invalid;
}

void QDeclarative1AnchorsPrivate::updateVerticalAnchors()
{
    if (fill || centerIn || !isItemComplete())
        return;

    if (updatingVerticalAnchor < 2) {
        ++updatingVerticalAnchor;
        QGraphicsItemPrivate *itemPrivate = QGraphicsItemPrivate::get(item);
        if (usedAnchors & QDeclarative1Anchors::TopAnchor) {
            //Handle stretching
            bool invalid = true;
            qreal height = 0.0;
            if (usedAnchors & QDeclarative1Anchors::BottomAnchor) {
                invalid = calcStretch(top, bottom, topMargin, -bottomMargin, QDeclarative1AnchorLine::Top, height);
            } else if (usedAnchors & QDeclarative1Anchors::VCenterAnchor) {
                invalid = calcStretch(top, vCenter, topMargin, vCenterOffset, QDeclarative1AnchorLine::Top, height);
                height *= 2;
            }
            if (!invalid)
                setItemHeight(height);

            //Handle top
            if (top.item == item->parentItem()) {
                setItemY(adjustedPosition(top.item, top.anchorLine) + topMargin);
            } else if (top.item->parentItem() == item->parentItem()) {
                setItemY(position(top.item, top.anchorLine) + topMargin);
            }
        } else if (usedAnchors & QDeclarative1Anchors::BottomAnchor) {
            //Handle stretching (top + bottom case is handled above)
            if (usedAnchors & QDeclarative1Anchors::VCenterAnchor) {
                qreal height = 0.0;
                bool invalid = calcStretch(vCenter, bottom, vCenterOffset, -bottomMargin,
                                              QDeclarative1AnchorLine::Top, height);
                if (!invalid)
                    setItemHeight(height*2);
            }

            //Handle bottom
            if (bottom.item == item->parentItem()) {
                setItemY(adjustedPosition(bottom.item, bottom.anchorLine) - itemPrivate->height() - bottomMargin);
            } else if (bottom.item->parentItem() == item->parentItem()) {
                setItemY(position(bottom.item, bottom.anchorLine) - itemPrivate->height() - bottomMargin);
            }
        } else if (usedAnchors & QDeclarative1Anchors::VCenterAnchor) {
            //(stetching handled above)

            //Handle vCenter
            if (vCenter.item == item->parentItem()) {
                setItemY(adjustedPosition(vCenter.item, vCenter.anchorLine)
                              - vcenter(item) + vCenterOffset);
            } else if (vCenter.item->parentItem() == item->parentItem()) {
                setItemY(position(vCenter.item, vCenter.anchorLine) - vcenter(item) + vCenterOffset);
            }
        } else if (usedAnchors & QDeclarative1Anchors::BaselineAnchor) {
            //Handle baseline
            if (baseline.item == item->parentItem()) {
                if (itemPrivate->isDeclarativeItem)
                    setItemY(adjustedPosition(baseline.item, baseline.anchorLine)
                        - static_cast<QDeclarativeItem *>(item)->baselineOffset() + baselineOffset);
            } else if (baseline.item->parentItem() == item->parentItem()) {
                if (itemPrivate->isDeclarativeItem)
                    setItemY(position(baseline.item, baseline.anchorLine)
                        - static_cast<QDeclarativeItem *>(item)->baselineOffset() + baselineOffset);
            }
        }
        --updatingVerticalAnchor;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QDeclarative1Anchors::tr("Possible anchor loop detected on vertical anchor.");
    }
}

inline QDeclarative1AnchorLine::AnchorLine reverseAnchorLine(QDeclarative1AnchorLine::AnchorLine anchorLine) {
    if (anchorLine == QDeclarative1AnchorLine::Left) {
        return QDeclarative1AnchorLine::Right;
    } else if (anchorLine == QDeclarative1AnchorLine::Right) {
        return QDeclarative1AnchorLine::Left;
    } else {
        return anchorLine;
    }
}

void QDeclarative1AnchorsPrivate::updateHorizontalAnchors()
{
    Q_Q(QDeclarative1Anchors);
    if (fill || centerIn || !isItemComplete())
        return;

    if (updatingHorizontalAnchor < 3) {
        ++updatingHorizontalAnchor;
        qreal effectiveRightMargin, effectiveLeftMargin, effectiveHorizontalCenterOffset;
        QDeclarative1AnchorLine effectiveLeft, effectiveRight, effectiveHorizontalCenter;
        QDeclarative1Anchors::Anchor effectiveLeftAnchor, effectiveRightAnchor;
        if (q->mirrored()) {
            effectiveLeftAnchor = QDeclarative1Anchors::RightAnchor;
            effectiveRightAnchor = QDeclarative1Anchors::LeftAnchor;
            effectiveLeft.item = right.item;
            effectiveLeft.anchorLine = reverseAnchorLine(right.anchorLine);
            effectiveRight.item = left.item;
            effectiveRight.anchorLine = reverseAnchorLine(left.anchorLine);
            effectiveHorizontalCenter.item = hCenter.item;
            effectiveHorizontalCenter.anchorLine = reverseAnchorLine(hCenter.anchorLine);
            effectiveLeftMargin = rightMargin;
            effectiveRightMargin = leftMargin;
            effectiveHorizontalCenterOffset = -hCenterOffset;
        } else {
            effectiveLeftAnchor = QDeclarative1Anchors::LeftAnchor;
            effectiveRightAnchor = QDeclarative1Anchors::RightAnchor;
            effectiveLeft = left;
            effectiveRight = right;
            effectiveHorizontalCenter = hCenter;
            effectiveLeftMargin = leftMargin;
            effectiveRightMargin = rightMargin;
            effectiveHorizontalCenterOffset = hCenterOffset;
        }

        QGraphicsItemPrivate *itemPrivate = QGraphicsItemPrivate::get(item);
        if (usedAnchors & effectiveLeftAnchor) {
            //Handle stretching
            bool invalid = true;
            qreal width = 0.0;
            if (usedAnchors & effectiveRightAnchor) {
                invalid = calcStretch(effectiveLeft, effectiveRight, effectiveLeftMargin, -effectiveRightMargin, QDeclarative1AnchorLine::Left, width);
            } else if (usedAnchors & QDeclarative1Anchors::HCenterAnchor) {
                invalid = calcStretch(effectiveLeft, effectiveHorizontalCenter, effectiveLeftMargin, effectiveHorizontalCenterOffset, QDeclarative1AnchorLine::Left, width);
                width *= 2;
            }
            if (!invalid)
                setItemWidth(width);

            //Handle left
            if (effectiveLeft.item == item->parentItem()) {
                setItemX(adjustedPosition(effectiveLeft.item, effectiveLeft.anchorLine) + effectiveLeftMargin);
            } else if (effectiveLeft.item->parentItem() == item->parentItem()) {
                setItemX(position(effectiveLeft.item, effectiveLeft.anchorLine) + effectiveLeftMargin);
            }
        } else if (usedAnchors & effectiveRightAnchor) {
            //Handle stretching (left + right case is handled in updateLeftAnchor)
            if (usedAnchors & QDeclarative1Anchors::HCenterAnchor) {
                qreal width = 0.0;
                bool invalid = calcStretch(effectiveHorizontalCenter, effectiveRight, effectiveHorizontalCenterOffset, -effectiveRightMargin,
                                              QDeclarative1AnchorLine::Left, width);
                if (!invalid)
                    setItemWidth(width*2);
            }

            //Handle right
            if (effectiveRight.item == item->parentItem()) {
                setItemX(adjustedPosition(effectiveRight.item, effectiveRight.anchorLine) - itemPrivate->width() - effectiveRightMargin);
            } else if (effectiveRight.item->parentItem() == item->parentItem()) {
                setItemX(position(effectiveRight.item, effectiveRight.anchorLine) - itemPrivate->width() - effectiveRightMargin);
            }
        } else if (usedAnchors & QDeclarative1Anchors::HCenterAnchor) {
            //Handle hCenter
            if (effectiveHorizontalCenter.item == item->parentItem()) {
                setItemX(adjustedPosition(effectiveHorizontalCenter.item, effectiveHorizontalCenter.anchorLine) - hcenter(item) + effectiveHorizontalCenterOffset);
            } else if (effectiveHorizontalCenter.item->parentItem() == item->parentItem()) {
                setItemX(position(effectiveHorizontalCenter.item, effectiveHorizontalCenter.anchorLine) - hcenter(item) + effectiveHorizontalCenterOffset);
            }
        }
        --updatingHorizontalAnchor;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QDeclarative1Anchors::tr("Possible anchor loop detected on horizontal anchor.");
    }
}

QDeclarative1AnchorLine QDeclarative1Anchors::top() const
{
    Q_D(const QDeclarative1Anchors);
    return d->top;
}

void QDeclarative1Anchors::setTop(const QDeclarative1AnchorLine &edge)
{
    Q_D(QDeclarative1Anchors);
    if (!d->checkVAnchorValid(edge) || d->top == edge)
        return;

    d->usedAnchors |= TopAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~TopAnchor;
        return;
    }

    d->remDepend(d->top.item);
    d->top = edge;
    d->addDepend(d->top.item);
    emit topChanged();
    d->updateVerticalAnchors();
}

void QDeclarative1Anchors::resetTop()
{
    Q_D(QDeclarative1Anchors);
    d->usedAnchors &= ~TopAnchor;
    d->remDepend(d->top.item);
    d->top = QDeclarative1AnchorLine();
    emit topChanged();
    d->updateVerticalAnchors();
}

QDeclarative1AnchorLine QDeclarative1Anchors::bottom() const
{
    Q_D(const QDeclarative1Anchors);
    return d->bottom;
}

void QDeclarative1Anchors::setBottom(const QDeclarative1AnchorLine &edge)
{
    Q_D(QDeclarative1Anchors);
    if (!d->checkVAnchorValid(edge) || d->bottom == edge)
        return;

    d->usedAnchors |= BottomAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~BottomAnchor;
        return;
    }

    d->remDepend(d->bottom.item);
    d->bottom = edge;
    d->addDepend(d->bottom.item);
    emit bottomChanged();
    d->updateVerticalAnchors();
}

void QDeclarative1Anchors::resetBottom()
{
    Q_D(QDeclarative1Anchors);
    d->usedAnchors &= ~BottomAnchor;
    d->remDepend(d->bottom.item);
    d->bottom = QDeclarative1AnchorLine();
    emit bottomChanged();
    d->updateVerticalAnchors();
}

QDeclarative1AnchorLine QDeclarative1Anchors::verticalCenter() const
{
    Q_D(const QDeclarative1Anchors);
    return d->vCenter;
}

void QDeclarative1Anchors::setVerticalCenter(const QDeclarative1AnchorLine &edge)
{
    Q_D(QDeclarative1Anchors);
    if (!d->checkVAnchorValid(edge) || d->vCenter == edge)
        return;

    d->usedAnchors |= VCenterAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~VCenterAnchor;
        return;
    }

    d->remDepend(d->vCenter.item);
    d->vCenter = edge;
    d->addDepend(d->vCenter.item);
    emit verticalCenterChanged();
    d->updateVerticalAnchors();
}

void QDeclarative1Anchors::resetVerticalCenter()
{
    Q_D(QDeclarative1Anchors);
    d->usedAnchors &= ~VCenterAnchor;
    d->remDepend(d->vCenter.item);
    d->vCenter = QDeclarative1AnchorLine();
    emit verticalCenterChanged();
    d->updateVerticalAnchors();
}

QDeclarative1AnchorLine QDeclarative1Anchors::baseline() const
{
    Q_D(const QDeclarative1Anchors);
    return d->baseline;
}

void QDeclarative1Anchors::setBaseline(const QDeclarative1AnchorLine &edge)
{
    Q_D(QDeclarative1Anchors);
    if (!d->checkVAnchorValid(edge) || d->baseline == edge)
        return;

    d->usedAnchors |= BaselineAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~BaselineAnchor;
        return;
    }

    d->remDepend(d->baseline.item);
    d->baseline = edge;
    d->addDepend(d->baseline.item);
    emit baselineChanged();
    d->updateVerticalAnchors();
}

void QDeclarative1Anchors::resetBaseline()
{
    Q_D(QDeclarative1Anchors);
    d->usedAnchors &= ~BaselineAnchor;
    d->remDepend(d->baseline.item);
    d->baseline = QDeclarative1AnchorLine();
    emit baselineChanged();
    d->updateVerticalAnchors();
}

QDeclarative1AnchorLine QDeclarative1Anchors::left() const
{
    Q_D(const QDeclarative1Anchors);
    return d->left;
}

void QDeclarative1Anchors::setLeft(const QDeclarative1AnchorLine &edge)
{
    Q_D(QDeclarative1Anchors);
    if (!d->checkHAnchorValid(edge) || d->left == edge)
        return;

    d->usedAnchors |= LeftAnchor;

    if (!d->checkHValid()) {
        d->usedAnchors &= ~LeftAnchor;
        return;
    }

    d->remDepend(d->left.item);
    d->left = edge;
    d->addDepend(d->left.item);
    emit leftChanged();
    d->updateHorizontalAnchors();
}

void QDeclarative1Anchors::resetLeft()
{
    Q_D(QDeclarative1Anchors);
    d->usedAnchors &= ~LeftAnchor;
    d->remDepend(d->left.item);
    d->left = QDeclarative1AnchorLine();
    emit leftChanged();
    d->updateHorizontalAnchors();
}

QDeclarative1AnchorLine QDeclarative1Anchors::right() const
{
    Q_D(const QDeclarative1Anchors);
    return d->right;
}

void QDeclarative1Anchors::setRight(const QDeclarative1AnchorLine &edge)
{
    Q_D(QDeclarative1Anchors);
    if (!d->checkHAnchorValid(edge) || d->right == edge)
        return;

    d->usedAnchors |= RightAnchor;

    if (!d->checkHValid()) {
        d->usedAnchors &= ~RightAnchor;
        return;
    }

    d->remDepend(d->right.item);
    d->right = edge;
    d->addDepend(d->right.item);
    emit rightChanged();
    d->updateHorizontalAnchors();
}

void QDeclarative1Anchors::resetRight()
{
    Q_D(QDeclarative1Anchors);
    d->usedAnchors &= ~RightAnchor;
    d->remDepend(d->right.item);
    d->right = QDeclarative1AnchorLine();
    emit rightChanged();
    d->updateHorizontalAnchors();
}

QDeclarative1AnchorLine QDeclarative1Anchors::horizontalCenter() const
{
    Q_D(const QDeclarative1Anchors);
    return d->hCenter;
}

void QDeclarative1Anchors::setHorizontalCenter(const QDeclarative1AnchorLine &edge)
{
    Q_D(QDeclarative1Anchors);
    if (!d->checkHAnchorValid(edge) || d->hCenter == edge)
        return;

    d->usedAnchors |= HCenterAnchor;

    if (!d->checkHValid()) {
        d->usedAnchors &= ~HCenterAnchor;
        return;
    }

    d->remDepend(d->hCenter.item);
    d->hCenter = edge;
    d->addDepend(d->hCenter.item);
    emit horizontalCenterChanged();
    d->updateHorizontalAnchors();
}

void QDeclarative1Anchors::resetHorizontalCenter()
{
    Q_D(QDeclarative1Anchors);
    d->usedAnchors &= ~HCenterAnchor;
    d->remDepend(d->hCenter.item);
    d->hCenter = QDeclarative1AnchorLine();
    emit horizontalCenterChanged();
    d->updateHorizontalAnchors();
}

qreal QDeclarative1Anchors::leftMargin() const
{
    Q_D(const QDeclarative1Anchors);
    return d->leftMargin;
}

void QDeclarative1Anchors::setLeftMargin(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->leftMargin == offset)
        return;
    d->leftMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateHorizontalAnchors();
    emit leftMarginChanged();
}

qreal QDeclarative1Anchors::rightMargin() const
{
    Q_D(const QDeclarative1Anchors);
    return d->rightMargin;
}

void QDeclarative1Anchors::setRightMargin(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->rightMargin == offset)
        return;
    d->rightMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateHorizontalAnchors();
    emit rightMarginChanged();
}

qreal QDeclarative1Anchors::margins() const
{
    Q_D(const QDeclarative1Anchors);
    return d->margins;
}

void QDeclarative1Anchors::setMargins(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->margins == offset)
        return;
    //###Is it significantly faster to set them directly so we can call fillChanged only once?
    if(!d->rightMargin || d->rightMargin == d->margins)
        setRightMargin(offset);
    if(!d->leftMargin || d->leftMargin == d->margins)
        setLeftMargin(offset);
    if(!d->topMargin || d->topMargin == d->margins)
        setTopMargin(offset);
    if(!d->bottomMargin || d->bottomMargin == d->margins)
        setBottomMargin(offset);
    d->margins = offset;
    emit marginsChanged();

}

qreal QDeclarative1Anchors::horizontalCenterOffset() const
{
    Q_D(const QDeclarative1Anchors);
    return d->hCenterOffset;
}

void QDeclarative1Anchors::setHorizontalCenterOffset(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->hCenterOffset == offset)
        return;
    d->hCenterOffset = offset;
    if(d->centerIn)
        d->centerInChanged();
    else
        d->updateHorizontalAnchors();
    emit horizontalCenterOffsetChanged();
}

qreal QDeclarative1Anchors::topMargin() const
{
    Q_D(const QDeclarative1Anchors);
    return d->topMargin;
}

void QDeclarative1Anchors::setTopMargin(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->topMargin == offset)
        return;
    d->topMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateVerticalAnchors();
    emit topMarginChanged();
}

qreal QDeclarative1Anchors::bottomMargin() const
{
    Q_D(const QDeclarative1Anchors);
    return d->bottomMargin;
}

void QDeclarative1Anchors::setBottomMargin(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->bottomMargin == offset)
        return;
    d->bottomMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateVerticalAnchors();
    emit bottomMarginChanged();
}

qreal QDeclarative1Anchors::verticalCenterOffset() const
{
    Q_D(const QDeclarative1Anchors);
    return d->vCenterOffset;
}

void QDeclarative1Anchors::setVerticalCenterOffset(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->vCenterOffset == offset)
        return;
    d->vCenterOffset = offset;
    if(d->centerIn)
        d->centerInChanged();
    else
        d->updateVerticalAnchors();
    emit verticalCenterOffsetChanged();
}

qreal QDeclarative1Anchors::baselineOffset() const
{
    Q_D(const QDeclarative1Anchors);
    return d->baselineOffset;
}

void QDeclarative1Anchors::setBaselineOffset(qreal offset)
{
    Q_D(QDeclarative1Anchors);
    if (d->baselineOffset == offset)
        return;
    d->baselineOffset = offset;
    d->updateVerticalAnchors();
    emit baselineOffsetChanged();
}

QDeclarative1Anchors::Anchors QDeclarative1Anchors::usedAnchors() const
{
    Q_D(const QDeclarative1Anchors);
    return d->usedAnchors;
}

bool QDeclarative1AnchorsPrivate::checkHValid() const
{
    if (usedAnchors & QDeclarative1Anchors::LeftAnchor &&
        usedAnchors & QDeclarative1Anchors::RightAnchor &&
        usedAnchors & QDeclarative1Anchors::HCenterAnchor) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot specify left, right, and hcenter anchors.");
        return false;
    }

    return true;
}

bool QDeclarative1AnchorsPrivate::checkHAnchorValid(QDeclarative1AnchorLine anchor) const
{
    if (!anchor.item) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor to a null item.");
        return false;
    } else if (anchor.anchorLine & QDeclarative1AnchorLine::Vertical_Mask) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor a horizontal edge to a vertical edge.");
        return false;
    } else if (anchor.item != item->parentItem() && anchor.item->parentItem() != item->parentItem()){
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor to an item that isn't a parent or sibling.");
        return false;
    } else if (anchor.item == item) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor item to self.");
        return false;
    }

    return true;
}

bool QDeclarative1AnchorsPrivate::checkVValid() const
{
    if (usedAnchors & QDeclarative1Anchors::TopAnchor &&
        usedAnchors & QDeclarative1Anchors::BottomAnchor &&
        usedAnchors & QDeclarative1Anchors::VCenterAnchor) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot specify top, bottom, and vcenter anchors.");
        return false;
    } else if (usedAnchors & QDeclarative1Anchors::BaselineAnchor &&
               (usedAnchors & QDeclarative1Anchors::TopAnchor ||
                usedAnchors & QDeclarative1Anchors::BottomAnchor ||
                usedAnchors & QDeclarative1Anchors::VCenterAnchor)) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Baseline anchor cannot be used in conjunction with top, bottom, or vcenter anchors.");
        return false;
    }

    return true;
}

bool QDeclarative1AnchorsPrivate::checkVAnchorValid(QDeclarative1AnchorLine anchor) const
{
    if (!anchor.item) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor to a null item.");
        return false;
    } else if (anchor.anchorLine & QDeclarative1AnchorLine::Horizontal_Mask) {
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor a vertical edge to a horizontal edge.");
        return false;
    } else if (anchor.item != item->parentItem() && anchor.item->parentItem() != item->parentItem()){
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor to an item that isn't a parent or sibling.");
        return false;
    } else if (anchor.item == item){
        qmlInfo(item) << QDeclarative1Anchors::tr("Cannot anchor item to self.");
        return false;
    }

    return true;
}



QT_END_NAMESPACE

#include <moc_qdeclarativeanchors_p.cpp>

