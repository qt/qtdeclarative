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

#include "qsglistview_p.h"
#include "qsgitemview_p_p.h"
#include "qsgvisualitemmodel_p.h"

#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qevent.h>
#include <QtCore/qmath.h>
#include <QtCore/qcoreapplication.h>

#include <private/qdeclarativesmoothedanimation_p_p.h>
#include <private/qlistmodelinterface_p.h>

QT_BEGIN_NAMESPACE

void QSGViewSection::setProperty(const QString &property)
{
    if (property != m_property) {
        m_property = property;
        emit propertyChanged();
    }
}

void QSGViewSection::setCriteria(QSGViewSection::SectionCriteria criteria)
{
    if (criteria != m_criteria) {
        m_criteria = criteria;
        emit criteriaChanged();
    }
}

void QSGViewSection::setDelegate(QDeclarativeComponent *delegate)
{
    if (delegate != m_delegate) {
        m_delegate = delegate;
        emit delegateChanged();
    }
}

QString QSGViewSection::sectionString(const QString &value)
{
    if (m_criteria == FirstCharacter)
        return value.isEmpty() ? QString() : value.at(0);
    else
        return value;
}

//----------------------------------------------------------------------------

class FxListItemSG : public FxViewItem
{
public:
    FxListItemSG(QSGItem *i, QSGListView *v, bool own) : FxViewItem(i, own), section(0), view(v) {
        attached = static_cast<QSGListViewAttached*>(qmlAttachedPropertiesObject<QSGListView>(item));
        if (attached)
            static_cast<QSGListViewAttached*>(attached)->setView(view);
    }

    ~FxListItemSG() {}

    qreal position() const {
        if (section) {
            if (view->orientation() == QSGListView::Vertical)
                return section->y();
            else
                return (view->effectiveLayoutDirection() == Qt::RightToLeft ? -section->width()-section->x() : section->x());
        } else {
            return itemPosition();
        }
    }
    qreal itemPosition() const {
        if (view->orientation() == QSGListView::Vertical)
            return item->y();
        else
            return (view->effectiveLayoutDirection() == Qt::RightToLeft ? -item->width()-item->x() : item->x());
    }
    qreal size() const {
        if (section)
            return (view->orientation() == QSGListView::Vertical ? item->height()+section->height() : item->width()+section->width());
        else
            return (view->orientation() == QSGListView::Vertical ? item->height() : item->width());
    }
    qreal itemSize() const {
        return (view->orientation() == QSGListView::Vertical ? item->height() : item->width());
    }
    qreal sectionSize() const {
        if (section)
            return (view->orientation() == QSGListView::Vertical ? section->height() : section->width());
        return 0.0;
    }
    qreal endPosition() const {
        if (view->orientation() == QSGListView::Vertical) {
            return item->y() + item->height();
        } else {
            return (view->effectiveLayoutDirection() == Qt::RightToLeft
                    ? -item->x()
                    : item->x() + item->width());
        }
    }
    void setPosition(qreal pos) {
        if (view->orientation() == QSGListView::Vertical) {
            if (section) {
                section->setY(pos);
                pos += section->height();
            }
            item->setY(pos);
        } else {
            if (view->effectiveLayoutDirection() == Qt::RightToLeft) {
                if (section) {
                    section->setX(-section->width()-pos);
                    pos += section->width();
                }
                item->setX(-item->width()-pos);
            } else {
                if (section) {
                    section->setX(pos);
                    pos += section->width();
                }
                item->setX(pos);
            }
        }
    }
    void setSize(qreal size) {
        if (view->orientation() == QSGListView::Vertical)
            item->setHeight(size);
        else
            item->setWidth(size);
    }
    bool contains(qreal x, qreal y) const {
        return (x >= item->x() && x < item->x() + item->width() &&
                y >= item->y() && y < item->y() + item->height());
    }

    QSGItem *section;
    QSGListView *view;
};

//----------------------------------------------------------------------------

class QSGListViewPrivate : public QSGItemViewPrivate
{
    Q_DECLARE_PUBLIC(QSGListView)
public:
    virtual Qt::Orientation layoutOrientation() const;
    virtual bool isContentFlowReversed() const;
    bool isRightToLeft() const;

    virtual qreal positionAt(int index) const;
    virtual qreal endPositionAt(int index) const;
    virtual qreal originPosition() const;
    virtual qreal lastPosition() const;

    FxViewItem *nextVisibleItem() const;
    FxViewItem *itemBefore(int modelIndex) const;
    QString sectionAt(int modelIndex);
    qreal snapPosAt(qreal pos);
    FxViewItem *snapItemAt(qreal pos);

    virtual void init();
    virtual void clear();

    virtual bool addVisibleItems(int fillFrom, int fillTo, bool doBuffer);
    virtual bool removeNonVisibleItems(int bufferFrom, int bufferTo);
    virtual void visibleItemsChanged();

    virtual FxViewItem *newViewItem(int index, QSGItem *item);
    virtual void initializeViewItem(FxViewItem *item);
    virtual void releaseItem(FxViewItem *item);
    virtual void repositionPackageItemAt(QSGItem *item, int index);

    virtual void createHighlight();
    virtual void updateHighlight();
    virtual void resetHighlightPosition();

    virtual void setPosition(qreal pos);
    virtual void layoutVisibleItems();

    virtual void updateSections();
    void createSection(FxListItemSG *);
    void updateCurrentSection();

    virtual qreal headerSize() const;
    virtual qreal footerSize() const;
    virtual void updateHeader();
    virtual void updateFooter();

    virtual void changedVisibleIndex(int newIndex);
    virtual void initializeCurrentItem();

    void updateAverage();

    void itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    virtual void fixupPosition();
    virtual void fixup(AxisData &data, qreal minExtent, qreal maxExtent);
    virtual void flick(QSGItemViewPrivate::AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                        QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity);

    QSGListView::Orientation orient;
    qreal visiblePos;
    qreal averageSize;
    qreal spacing;
    QSGListView::SnapMode snapMode;

    QSmoothedAnimation *highlightPosAnimator;
    QSmoothedAnimation *highlightSizeAnimator;
    qreal highlightMoveSpeed;
    qreal highlightResizeSpeed;
    int highlightResizeDuration;

    QSGViewSection *sectionCriteria;
    QString currentSection;
    static const int sectionCacheSize = 4;
    QSGItem *sectionCache[sectionCacheSize];

    qreal overshootDist;
    bool correctFlick : 1;
    bool inFlickCorrection : 1;

    QSGListViewPrivate()
        : orient(QSGListView::Vertical)
        , visiblePos(0)
        , averageSize(100.0), spacing(0.0)
        , snapMode(QSGListView::NoSnap)
        , highlightPosAnimator(0), highlightSizeAnimator(0)
        , highlightMoveSpeed(400), highlightResizeSpeed(400), highlightResizeDuration(-1)
        , sectionCriteria(0)
        , overshootDist(0.0), correctFlick(false), inFlickCorrection(false)
    {}
};

bool QSGListViewPrivate::isContentFlowReversed() const
{
    return isRightToLeft();
}

Qt::Orientation QSGListViewPrivate::layoutOrientation() const
{
    return static_cast<Qt::Orientation>(orient);
}

bool QSGListViewPrivate::isRightToLeft() const
{
    Q_Q(const QSGListView);
    return orient == QSGListView::Horizontal && q->effectiveLayoutDirection() == Qt::RightToLeft;
}

FxViewItem *QSGListViewPrivate::nextVisibleItem() const
{
    const qreal pos = isRightToLeft() ? -position()-size() : position();
    bool foundFirst = false;
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index != -1) {
            if (foundFirst)
                return item;
            else if (item->position() < pos && item->endPosition() >= pos)
                foundFirst = true;
        }
    }
    return 0;
}

// Returns the item before modelIndex, if created.
// May return an item marked for removal.
FxViewItem *QSGListViewPrivate::itemBefore(int modelIndex) const
{
    if (modelIndex < visibleIndex)
        return 0;
    int idx = 1;
    int lastIndex = -1;
    while (idx < visibleItems.count()) {
        FxViewItem *item = visibleItems.at(idx);
        if (item->index != -1)
            lastIndex = item->index;
        if (item->index == modelIndex)
            return visibleItems.at(idx-1);
        ++idx;
    }
    if (lastIndex == modelIndex-1)
        return visibleItems.last();
    return 0;
}

void QSGListViewPrivate::setPosition(qreal pos)
{
    Q_Q(QSGListView);
    if (orient == QSGListView::Vertical) {
        q->QSGFlickable::setContentY(pos);
    } else {
        if (isRightToLeft())
            q->QSGFlickable::setContentX(-pos-size());
        else
            q->QSGFlickable::setContentX(pos);
    }
}

qreal QSGListViewPrivate::originPosition() const
{
    qreal pos = 0;
    if (!visibleItems.isEmpty()) {
        pos = (*visibleItems.constBegin())->position();
        if (visibleIndex > 0)
            pos -= visibleIndex * (averageSize + spacing);
    }
    return pos;
}

qreal QSGListViewPrivate::lastPosition() const
{
    qreal pos = 0;
    if (!visibleItems.isEmpty()) {
        int invisibleCount = visibleItems.count() - visibleIndex;
        for (int i = visibleItems.count()-1; i >= 0; --i) {
            if (visibleItems.at(i)->index != -1) {
                invisibleCount = model->count() - visibleItems.at(i)->index - 1;
                break;
            }
        }
        pos = (*(--visibleItems.constEnd()))->endPosition() + invisibleCount * (averageSize + spacing);
    } else if (model && model->count()) {
        pos = (model->count() * averageSize + (model->count()-1) * spacing);
    }
    return pos;
}

qreal QSGListViewPrivate::positionAt(int modelIndex) const
{
    if (FxViewItem *item = visibleItem(modelIndex))
        return item->position();
    if (!visibleItems.isEmpty()) {
        if (modelIndex < visibleIndex) {
            int count = visibleIndex - modelIndex;
            qreal cs = 0;
            if (modelIndex == currentIndex && currentItem) {
                cs = currentItem->size() + spacing;
                --count;
            }
            return (*visibleItems.constBegin())->position() - count * (averageSize + spacing) - cs;
        } else {
            int count = modelIndex - findLastVisibleIndex(visibleIndex) - 1;
            return (*(--visibleItems.constEnd()))->endPosition() + spacing + count * (averageSize + spacing);
        }
    }
    return 0;
}

qreal QSGListViewPrivate::endPositionAt(int modelIndex) const
{
    if (FxViewItem *item = visibleItem(modelIndex))
        return item->endPosition();
    if (!visibleItems.isEmpty()) {
        if (modelIndex < visibleIndex) {
            int count = visibleIndex - modelIndex;
            return (*visibleItems.constBegin())->position() - (count - 1) * (averageSize + spacing) - spacing;
        } else {
            int count = modelIndex - findLastVisibleIndex(visibleIndex) - 1;
            return (*(--visibleItems.constEnd()))->endPosition() + count * (averageSize + spacing);
        }
    }
    return 0;
}

QString QSGListViewPrivate::sectionAt(int modelIndex)
{
    if (FxViewItem *item = visibleItem(modelIndex))
        return item->attached->section();

    QString section;
    if (sectionCriteria) {
        QString propValue = model->stringValue(modelIndex, sectionCriteria->property());
        section = sectionCriteria->sectionString(propValue);
    }

    return section;
}

qreal QSGListViewPrivate::snapPosAt(qreal pos)
{
    if (FxViewItem *snapItem = snapItemAt(pos))
        return snapItem->position();
    if (visibleItems.count()) {
        qreal firstPos = (*visibleItems.constBegin())->position();
        qreal endPos = (*(--visibleItems.constEnd()))->position();
        if (pos < firstPos) {
            return firstPos - qRound((firstPos - pos) / averageSize) * averageSize;
        } else if (pos > endPos)
            return endPos + qRound((pos - endPos) / averageSize) * averageSize;
    }
    return qRound((pos - originPosition()) / averageSize) * averageSize + originPosition();
}

FxViewItem *QSGListViewPrivate::snapItemAt(qreal pos)
{
    FxViewItem *snapItem = 0;
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == -1)
            continue;
        qreal itemTop = item->position();
        if (highlight && itemTop >= pos && item->endPosition() <= pos + highlight->size())
            return item;
        if (itemTop+item->size()/2 >= pos && itemTop-item->size()/2 < pos)
            snapItem = item;
    }
    return snapItem;
}

void QSGListViewPrivate::changedVisibleIndex(int newIndex)
{
    visiblePos = positionAt(newIndex);
    visibleIndex = newIndex;
}

void QSGListViewPrivate::init()
{
    QSGItemViewPrivate::init();
    ::memset(sectionCache, 0, sizeof(QSGItem*) * sectionCacheSize);
}

void QSGListViewPrivate::clear()
{
    for (int i = 0; i < sectionCacheSize; ++i) {
        delete sectionCache[i];
        sectionCache[i] = 0;
    }
    visiblePos = 0;
    QSGItemViewPrivate::clear();
}

FxViewItem *QSGListViewPrivate::newViewItem(int modelIndex, QSGItem *item)
{
    Q_Q(QSGListView);

    FxListItemSG *listItem = new FxListItemSG(item, q, false);
    listItem->index = modelIndex;

    // initialise attached properties
    if (sectionCriteria) {
        QString propValue = model->stringValue(modelIndex, sectionCriteria->property());
        listItem->attached->m_section = sectionCriteria->sectionString(propValue);
        if (modelIndex > 0) {
            if (FxViewItem *item = itemBefore(modelIndex))
                listItem->attached->m_prevSection = item->attached->section();
            else
                listItem->attached->m_prevSection = sectionAt(modelIndex-1);
        }
        if (modelIndex < model->count()-1) {
            if (FxViewItem *item = visibleItem(modelIndex+1))
                listItem->attached->m_nextSection = static_cast<QSGListViewAttached*>(item->attached)->section();
            else
                listItem->attached->m_nextSection = sectionAt(modelIndex+1);
        }
    }

    return listItem;
}

void QSGListViewPrivate::initializeViewItem(FxViewItem *item)
{
    QSGItemViewPrivate::initializeViewItem(item);

    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item->item);
    itemPrivate->addItemChangeListener(this, QSGItemPrivate::Geometry);

    if (sectionCriteria && sectionCriteria->delegate()) {
        if (item->attached->m_prevSection != item->attached->m_section)
            createSection(static_cast<FxListItemSG*>(item));
    }
}

void QSGListViewPrivate::releaseItem(FxViewItem *item)
{
    if (item) {
        FxListItemSG* listItem = static_cast<FxListItemSG*>(item);
        if (listItem->section) {
            int i = 0;
            do {
                if (!sectionCache[i]) {
                    sectionCache[i] = listItem->section;
                    sectionCache[i]->setVisible(false);
                    listItem->section = 0;
                    break;
                }
                ++i;
            } while (i < sectionCacheSize);
            delete listItem->section;
        }
    }
    QSGItemViewPrivate::releaseItem(item);
}

bool QSGListViewPrivate::addVisibleItems(int fillFrom, int fillTo, bool doBuffer)
{
    qreal itemEnd = visiblePos;
    if (visibleItems.count()) {
        visiblePos = (*visibleItems.constBegin())->position();
        itemEnd = (*(--visibleItems.constEnd()))->endPosition() + spacing;
    }

    int modelIndex = findLastVisibleIndex();
    bool haveValidItems = modelIndex >= 0;
    modelIndex = modelIndex < 0 ? visibleIndex : modelIndex + 1;

    if (haveValidItems && (fillFrom > itemEnd+averageSize+spacing
        || fillTo < visiblePos - averageSize - spacing)) {
        // We've jumped more than a page.  Estimate which items are now
        // visible and fill from there.
        int count = (fillFrom - itemEnd) / (averageSize + spacing);
        for (int i = 0; i < visibleItems.count(); ++i)
            releaseItem(visibleItems.at(i));
        visibleItems.clear();
        modelIndex += count;
        if (modelIndex >= model->count()) {
            count -= modelIndex - model->count() + 1;
            modelIndex = model->count() - 1;
        } else if (modelIndex < 0) {
            count -= modelIndex;
            modelIndex = 0;
        }
        visibleIndex = modelIndex;
        visiblePos = itemEnd + count * (averageSize + spacing);
        itemEnd = visiblePos;
    }

    bool changed = false;
    FxListItemSG *item = 0;
    qreal pos = itemEnd;
    while (modelIndex < model->count() && pos <= fillTo) {
//        qDebug() << "refill: append item" << modelIndex << "pos" << pos;
        if (!(item = static_cast<FxListItemSG*>(createItem(modelIndex))))
            break;
        item->setPosition(pos);
        pos += item->size() + spacing;
        visibleItems.append(item);
        ++modelIndex;
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }
    while (visibleIndex > 0 && visibleIndex <= model->count() && visiblePos >= fillFrom) {
//        qDebug() << "refill: prepend item" << visibleIndex-1 << "current top pos" << visiblePos;
        if (!(item = static_cast<FxListItemSG*>(createItem(visibleIndex-1))))
            break;
        --visibleIndex;
        visiblePos -= item->size() + spacing;
        item->setPosition(visiblePos);
        visibleItems.prepend(item);
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }

    return changed;
}

bool QSGListViewPrivate::removeNonVisibleItems(int bufferFrom, int bufferTo)
{
    FxViewItem *item = 0;
    bool changed = false;

    while (visibleItems.count() > 1 && (item = visibleItems.first()) && item->endPosition() <= bufferFrom) {
        if (item->attached->delayRemove())
            break;
//            qDebug() << "refill: remove first" << visibleIndex << "top end pos" << item->endPosition();
        if (item->index != -1)
            visibleIndex++;
        visibleItems.removeFirst();
        releaseItem(item);
        changed = true;
    }
    while (visibleItems.count() > 1 && (item = visibleItems.last()) && item->position() > bufferTo) {
        if (item->attached->delayRemove())
            break;
//            qDebug() << "refill: remove last" << visibleIndex+visibleItems.count()-1 << item->position();
        visibleItems.removeLast();
        releaseItem(item);
        changed = true;
    }

    return changed;
}

void QSGListViewPrivate::visibleItemsChanged()
{
    if (visibleItems.count())
        visiblePos = (*visibleItems.constBegin())->position();
    updateAverage();
    if (currentIndex >= 0 && currentItem && !visibleItem(currentIndex)) {
        static_cast<FxListItemSG*>(currentItem)->setPosition(positionAt(currentIndex));
        updateHighlight();
    }
    if (sectionCriteria)
        updateCurrentSection();
    updateHeader();
    updateFooter();
    updateViewport();
    updateUnrequestedPositions();
}

void QSGListViewPrivate::layoutVisibleItems()
{
    if (!visibleItems.isEmpty()) {
        bool fixedCurrent = currentItem && (*visibleItems.constBegin())->item == currentItem->item;
        qreal sum = (*visibleItems.constBegin())->size();
        qreal pos = (*visibleItems.constBegin())->position() + (*visibleItems.constBegin())->size() + spacing;
        for (int i=1; i < visibleItems.count(); ++i) {
            FxListItemSG *item = static_cast<FxListItemSG*>(visibleItems.at(i));
            item->setPosition(pos);
            pos += item->size() + spacing;
            sum += item->size();
            fixedCurrent = fixedCurrent || (currentItem && item->item == currentItem->item);
        }
        averageSize = qRound(sum / visibleItems.count());

        // move current item if it is not a visible item.
        if (currentIndex >= 0 && currentItem && !fixedCurrent) {
            static_cast<FxListItemSG*>(currentItem)->setPosition(positionAt(currentIndex));
        }
    }
}

void QSGListViewPrivate::repositionPackageItemAt(QSGItem *item, int index)
{
    Q_Q(QSGListView);
    qreal pos = position();
    if (orient == QSGListView::Vertical) {
        if (item->y() + item->height() > pos && item->y() < pos + q->height())
            item->setY(positionAt(index));
    } else {
        if (item->x() + item->width() > pos && item->x() < pos + q->width()) {
            if (isRightToLeft())
                item->setX(-positionAt(index)-item->width());
            else
                item->setX(positionAt(index));
        }
    }
}

void QSGListViewPrivate::createHighlight()
{
    Q_Q(QSGListView);
    bool changed = false;
    if (highlight) {
        if (trackedItem == highlight)
            trackedItem = 0;
        delete highlight;
        highlight = 0;

        delete highlightPosAnimator;
        delete highlightSizeAnimator;
        highlightPosAnimator = 0;
        highlightSizeAnimator = 0;

        changed = true;
    }

    if (currentItem) {
        QSGItem *item = createHighlightItem();
        if (item) {
            FxListItemSG *newHighlight = new FxListItemSG(item, q, true);

            if (autoHighlight) {
                newHighlight->setSize(static_cast<FxListItemSG*>(currentItem)->itemSize());
                newHighlight->setPosition(static_cast<FxListItemSG*>(currentItem)->itemPosition());
            }
            const QLatin1String posProp(orient == QSGListView::Vertical ? "y" : "x");
            highlightPosAnimator = new QSmoothedAnimation(q);
            highlightPosAnimator->target = QDeclarativeProperty(item, posProp);
            highlightPosAnimator->velocity = highlightMoveSpeed;
            highlightPosAnimator->userDuration = highlightMoveDuration;

            const QLatin1String sizeProp(orient == QSGListView::Vertical ? "height" : "width");
            highlightSizeAnimator = new QSmoothedAnimation(q);
            highlightSizeAnimator->velocity = highlightResizeSpeed;
            highlightSizeAnimator->userDuration = highlightResizeDuration;
            highlightSizeAnimator->target = QDeclarativeProperty(item, sizeProp);

            highlight = newHighlight;
            changed = true;
        }
    }
    if (changed)
        emit q->highlightItemChanged();
}

void QSGListViewPrivate::updateHighlight()
{
    if ((!currentItem && highlight) || (currentItem && !highlight))
        createHighlight();
    bool strictHighlight = haveHighlightRange && highlightRange == QSGListView::StrictlyEnforceRange;
    if (currentItem && autoHighlight && highlight && (!strictHighlight || !pressed)) {
        // auto-update highlight
        FxListItemSG *listItem = static_cast<FxListItemSG*>(currentItem);
        highlightPosAnimator->to = isRightToLeft()
                ? -listItem->itemPosition()-listItem->itemSize()
                : listItem->itemPosition();
        highlightSizeAnimator->to = listItem->itemSize();
        if (orient == QSGListView::Vertical) {
            if (highlight->item->width() == 0)
                highlight->item->setWidth(currentItem->item->width());
        } else {
            if (highlight->item->height() == 0)
                highlight->item->setHeight(currentItem->item->height());
        }

        highlightPosAnimator->restart();
        highlightSizeAnimator->restart();
    }
    updateTrackedItem();
}

void QSGListViewPrivate::resetHighlightPosition()
{
    if (highlight && currentItem)
        static_cast<FxListItemSG*>(highlight)->setPosition(static_cast<FxListItemSG*>(currentItem)->itemPosition());
}

void QSGListViewPrivate::createSection(FxListItemSG *listItem)
{
    Q_Q(QSGListView);
    if (!sectionCriteria || !sectionCriteria->delegate())
        return;
    if (listItem->attached->m_prevSection != listItem->attached->m_section) {
        if (!listItem->section) {
            qreal pos = listItem->position();
            int i = sectionCacheSize-1;
            while (i >= 0 && !sectionCache[i])
                --i;
            if (i >= 0) {
                listItem->section = sectionCache[i];
                sectionCache[i] = 0;
                listItem->section->setVisible(true);
                QDeclarativeContext *context = QDeclarativeEngine::contextForObject(listItem->section)->parentContext();
                context->setContextProperty(QLatin1String("section"), listItem->attached->m_section);
            } else {
                QDeclarativeContext *context = new QDeclarativeContext(qmlContext(q));
                context->setContextProperty(QLatin1String("section"), listItem->attached->m_section);
                QObject *nobj = sectionCriteria->delegate()->beginCreate(context);
                if (nobj) {
                    QDeclarative_setParent_noEvent(context, nobj);
                    listItem->section = qobject_cast<QSGItem *>(nobj);
                    if (!listItem->section) {
                        delete nobj;
                    } else {
                        listItem->section->setZ(1);
                        QDeclarative_setParent_noEvent(listItem->section, q->contentItem());
                        listItem->section->setParentItem(q->contentItem());
                    }
                } else {
                    delete context;
                }
                sectionCriteria->delegate()->completeCreate();
            }
            listItem->setPosition(pos);
        } else {
            QDeclarativeContext *context = QDeclarativeEngine::contextForObject(listItem->section)->parentContext();
            context->setContextProperty(QLatin1String("section"), listItem->attached->m_section);
        }
    } else if (listItem->section) {
        qreal pos = listItem->position();
        int i = 0;
        do {
            if (!sectionCache[i]) {
                sectionCache[i] = listItem->section;
                sectionCache[i]->setVisible(false);
                listItem->section = 0;
                return;
            }
            ++i;
        } while (i < sectionCacheSize);
        delete listItem->section;
        listItem->section = 0;
        listItem->setPosition(pos);
    }
}

void QSGListViewPrivate::updateSections()
{
    QSGItemViewPrivate::updateSections();

    if (sectionCriteria && !visibleItems.isEmpty()) {
        QString prevSection;
        if (visibleIndex > 0)
            prevSection = sectionAt(visibleIndex-1);
        QSGListViewAttached *prevAtt = 0;
        int idx = -1;
        for (int i = 0; i < visibleItems.count(); ++i) {
            QSGListViewAttached *attached = static_cast<QSGListViewAttached*>(visibleItems.at(i)->attached);
            attached->setPrevSection(prevSection);
            if (visibleItems.at(i)->index != -1) {
                QString propValue = model->stringValue(visibleItems.at(i)->index, sectionCriteria->property());
                attached->setSection(sectionCriteria->sectionString(propValue));
                idx = visibleItems.at(i)->index;
            }
            createSection(static_cast<FxListItemSG*>(visibleItems.at(i)));
            if (prevAtt)
                prevAtt->setNextSection(attached->section());
            prevSection = attached->section();
            prevAtt = attached;
        }
        if (prevAtt) {
            if (idx > 0 && idx < model->count()-1)
                prevAtt->setNextSection(sectionAt(idx+1));
            else
                prevAtt->setNextSection(QString());
        }
    }
}

void QSGListViewPrivate::updateCurrentSection()
{
    Q_Q(QSGListView);
    if (!sectionCriteria || visibleItems.isEmpty()) {
        if (!currentSection.isEmpty()) {
            currentSection.clear();
            emit q->currentSectionChanged();
        }
        return;
    }
    int index = 0;
    while (index < visibleItems.count() && visibleItems.at(index)->endPosition() <= position())
        ++index;

    QString newSection = currentSection;
    if (index < visibleItems.count())
        newSection = visibleItems.at(index)->attached->section();
    else
        newSection = (*visibleItems.constBegin())->attached->section();
    if (newSection != currentSection) {
        currentSection = newSection;
        emit q->currentSectionChanged();
    }
}

void QSGListViewPrivate::initializeCurrentItem()
{
    QSGItemViewPrivate::initializeCurrentItem();

    if (currentItem) {
        FxListItemSG *listItem = static_cast<FxListItemSG *>(currentItem);

        if (currentIndex == visibleIndex - 1 && visibleItems.count()) {
            // We can calculate exact postion in this case
            listItem->setPosition(visibleItems.first()->position() - currentItem->size() - spacing);
        } else {
            // Create current item now and position as best we can.
            // Its position will be corrected when it becomes visible.
            listItem->setPosition(positionAt(currentIndex));
        }

        // Avoid showing section delegate twice.  We still need the section heading so that
        // currentItem positioning works correctly.
        // This is slightly sub-optimal, but section heading caching minimizes the impact.
        if (listItem->section)
            listItem->section->setVisible(false);

        if (visibleItems.isEmpty())
            averageSize = listItem->size();
    }
}

void QSGListViewPrivate::updateAverage()
{
    if (!visibleItems.count())
        return;
    qreal sum = 0.0;
    for (int i = 0; i < visibleItems.count(); ++i)
        sum += visibleItems.at(i)->size();
    averageSize = qRound(sum / visibleItems.count());
}

qreal QSGListViewPrivate::headerSize() const
{
    return header ? header->size() : 0.0;
}

qreal QSGListViewPrivate::footerSize() const
{
    return footer ? footer->size() : 0.0;
}

void QSGListViewPrivate::updateFooter()
{
    Q_Q(QSGListView);
    bool created = false;
    if (!footer) {
        QSGItem *item = createComponentItem(footerComponent, true);
        if (!item)
            return;
        item->setZ(1);
        footer = new FxListItemSG(item, q, true);
        created = true;
    }

    FxListItemSG *listItem = static_cast<FxListItemSG*>(footer);
    if (visibleItems.count()) {
        qreal endPos = lastPosition();
        if (findLastVisibleIndex() == model->count()-1) {
            listItem->setPosition(endPos);
        } else {
            qreal visiblePos = position() + q->height();
            if (endPos <= visiblePos || listItem->position() < endPos)
                listItem->setPosition(endPos);
        }
    } else {
        listItem->setPosition(visiblePos);
    }

    if (created)
        emit q->footerItemChanged();
}

void QSGListViewPrivate::updateHeader()
{
    Q_Q(QSGListView);
    bool created = false;
    if (!header) {
        QSGItem *item = createComponentItem(headerComponent, true);
        if (!item)
            return;
        item->setZ(1);
        header = new FxListItemSG(item, q, true);
        created = true;
    }

    FxListItemSG *listItem = static_cast<FxListItemSG*>(header);
    if (listItem) {
        if (visibleItems.count()) {
            qreal startPos = originPosition();
            if (visibleIndex == 0) {
                listItem->setPosition(startPos - headerSize());
            } else {
                if (position() <= startPos || listItem->position() > startPos - headerSize())
                    listItem->setPosition(startPos - headerSize());
            }
        } else {
            listItem->setPosition(-headerSize());
        }
    }

    if (created)
        emit q->headerItemChanged();
}

void QSGListViewPrivate::itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_Q(QSGListView);
    QSGItemViewPrivate::itemGeometryChanged(item, newGeometry, oldGeometry);
    if (!q->isComponentComplete())
        return;
    if (item != contentItem && (!highlight || item != highlight->item)) {
        if ((orient == QSGListView::Vertical && newGeometry.height() != oldGeometry.height())
            || (orient == QSGListView::Horizontal && newGeometry.width() != oldGeometry.width())) {
            scheduleLayout();
        }
    }
}

void QSGListViewPrivate::fixupPosition()
{
    if ((haveHighlightRange && highlightRange == QSGListView::StrictlyEnforceRange)
        || snapMode != QSGListView::NoSnap)
        moveReason = Other;
    if (orient == QSGListView::Vertical)
        fixupY();
    else
        fixupX();
}

void QSGListViewPrivate::fixup(AxisData &data, qreal minExtent, qreal maxExtent)
{
    if ((orient == QSGListView::Horizontal && &data == &vData)
        || (orient == QSGListView::Vertical && &data == &hData))
        return;

    correctFlick = false;
    fixupMode = moveReason == Mouse ? fixupMode : Immediate;

    qreal highlightStart;
    qreal highlightEnd;
    qreal viewPos;
    if (isRightToLeft()) {
        // Handle Right-To-Left exceptions
        viewPos = -position()-size();
        highlightStart = highlightRangeStartValid ? size() - highlightRangeEnd : highlightRangeStart;
        highlightEnd = highlightRangeEndValid ? size() - highlightRangeStart : highlightRangeEnd;
    } else {
        viewPos = position();
        highlightStart = highlightRangeStart;
        highlightEnd = highlightRangeEnd;
    }

    if (currentItem && haveHighlightRange && highlightRange == QSGListView::StrictlyEnforceRange
            && moveReason != QSGListViewPrivate::SetIndex) {
        updateHighlight();
        qreal pos = static_cast<FxListItemSG*>(currentItem)->itemPosition();
        if (viewPos < pos + static_cast<FxListItemSG*>(currentItem)->itemSize() - highlightEnd)
            viewPos = pos + static_cast<FxListItemSG*>(currentItem)->itemSize() - highlightEnd;
        if (viewPos > pos - highlightStart)
            viewPos = pos - highlightStart;
        if (isRightToLeft())
            viewPos = -viewPos-size();

        timeline.reset(data.move);
        if (viewPos != position()) {
            if (fixupMode != Immediate) {
                timeline.move(data.move, -viewPos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
                data.fixingUp = true;
            } else {
                timeline.set(data.move, -viewPos);
            }
        }
        vTime = timeline.time();
    } else if (snapMode != QSGListView::NoSnap && moveReason != QSGListViewPrivate::SetIndex) {
        qreal tempPosition = isRightToLeft() ? -position()-size() : position();
        FxViewItem *topItem = snapItemAt(tempPosition+highlightStart);
        FxViewItem *bottomItem = snapItemAt(tempPosition+highlightEnd);
        qreal pos;
        bool isInBounds = -position() > maxExtent && -position() < minExtent;
        if (topItem && isInBounds) {
            if (topItem->index == 0 && header && tempPosition+highlightStart < header->position()+headerSize()/2) {
                pos = isRightToLeft() ? - header->position() + highlightStart - size() : header->position() - highlightStart;
            } else {
                if (isRightToLeft())
                    pos = qMax(qMin(-topItem->position() + highlightStart - size(), -maxExtent), -minExtent);
                else
                    pos = qMax(qMin(topItem->position() - highlightStart, -maxExtent), -minExtent);
            }
        } else if (bottomItem && isInBounds) {
            if (isRightToLeft())
                pos = qMax(qMin(-bottomItem->position() + highlightStart - size(), -maxExtent), -minExtent);
            else
                pos = qMax(qMin(bottomItem->position() - highlightStart, -maxExtent), -minExtent);
        } else {
            QSGItemViewPrivate::fixup(data, minExtent, maxExtent);
            return;
        }

        qreal dist = qAbs(data.move + pos);
        if (dist > 0) {
            timeline.reset(data.move);
            if (fixupMode != Immediate) {
                timeline.move(data.move, -pos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
                data.fixingUp = true;
            } else {
                timeline.set(data.move, -pos);
            }
            vTime = timeline.time();
        }
    } else {
        QSGItemViewPrivate::fixup(data, minExtent, maxExtent);
    }
    data.inOvershoot = false;
    fixupMode = Normal;
}

void QSGListViewPrivate::flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                                        QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity)
{
    Q_Q(QSGListView);

    data.fixingUp = false;
    moveReason = Mouse;
    if ((!haveHighlightRange || highlightRange != QSGListView::StrictlyEnforceRange) && snapMode == QSGListView::NoSnap) {
        correctFlick = true;
        QSGItemViewPrivate::flick(data, minExtent, maxExtent, vSize, fixupCallback, velocity);
        return;
    }
    qreal maxDistance = 0;
    qreal dataValue = isRightToLeft() ? -data.move.value()+size() : data.move.value();
    // -ve velocity means list is moving up/left
    if (velocity > 0) {
        if (data.move.value() < minExtent) {
            if (snapMode == QSGListView::SnapOneItem) {
                if (FxViewItem *item = isRightToLeft() ? nextVisibleItem() : firstVisibleItem())
                    maxDistance = qAbs(item->position() + dataValue);
            } else {
                maxDistance = qAbs(minExtent - data.move.value());
            }
        }
        if (snapMode == QSGListView::NoSnap && highlightRange != QSGListView::StrictlyEnforceRange)
            data.flickTarget = minExtent;
    } else {
        if (data.move.value() > maxExtent) {
            if (snapMode == QSGListView::SnapOneItem) {
                if (FxViewItem *item = isRightToLeft() ? firstVisibleItem() : nextVisibleItem())
                    maxDistance = qAbs(item->position() + dataValue);
            } else {
                maxDistance = qAbs(maxExtent - data.move.value());
            }
        }
        if (snapMode == QSGListView::NoSnap && highlightRange != QSGListView::StrictlyEnforceRange)
            data.flickTarget = maxExtent;
    }
    bool overShoot = boundsBehavior == QSGFlickable::DragAndOvershootBounds;
    qreal highlightStart = isRightToLeft() && highlightRangeStartValid ? size()-highlightRangeEnd : highlightRangeStart;
    if (maxDistance > 0 || overShoot) {
        // These modes require the list to stop exactly on an item boundary.
        // The initial flick will estimate the boundary to stop on.
        // Since list items can have variable sizes, the boundary will be
        // reevaluated and adjusted as we approach the boundary.
        qreal v = velocity;
        if (maxVelocity != -1 && maxVelocity < qAbs(v)) {
            if (v < 0)
                v = -maxVelocity;
            else
                v = maxVelocity;
        }
        if (!flickingHorizontally && !flickingVertically) {
            // the initial flick - estimate boundary
            qreal accel = deceleration;
            qreal v2 = v * v;
            overshootDist = 0.0;
            // + averageSize/4 to encourage moving at least one item in the flick direction
            qreal dist = v2 / (accel * 2.0) + averageSize/4;
            if (maxDistance > 0)
                dist = qMin(dist, maxDistance);
            if (v > 0)
                dist = -dist;
            if ((maxDistance > 0.0 && v2 / (2.0f * maxDistance) < accel) || snapMode == QSGListView::SnapOneItem) {
                qreal distTemp = isRightToLeft() ? -dist : dist;
                data.flickTarget = -snapPosAt(-(dataValue - highlightStart) + distTemp) + highlightStart;
                data.flickTarget = isRightToLeft() ? -data.flickTarget+size() : data.flickTarget;
                if (overShoot) {
                    if (data.flickTarget >= minExtent) {
                        overshootDist = overShootDistance(vSize);
                        data.flickTarget += overshootDist;
                    } else if (data.flickTarget <= maxExtent) {
                        overshootDist = overShootDistance(vSize);
                        data.flickTarget -= overshootDist;
                    }
                }
                qreal adjDist = -data.flickTarget + data.move.value();
                if (qAbs(adjDist) > qAbs(dist)) {
                    // Prevent painfully slow flicking - adjust velocity to suit flickDeceleration
                    qreal adjv2 = accel * 2.0f * qAbs(adjDist);
                    if (adjv2 > v2) {
                        v2 = adjv2;
                        v = qSqrt(v2);
                        if (dist > 0)
                            v = -v;
                    }
                }
                dist = adjDist;
                accel = v2 / (2.0f * qAbs(dist));
            } else if (overShoot) {
                data.flickTarget = data.move.value() - dist;
                if (data.flickTarget >= minExtent) {
                    overshootDist = overShootDistance(vSize);
                    data.flickTarget += overshootDist;
                } else if (data.flickTarget <= maxExtent) {
                    overshootDist = overShootDistance(vSize);
                    data.flickTarget -= overshootDist;
                }
            }
            timeline.reset(data.move);
            timeline.accel(data.move, v, accel, maxDistance + overshootDist);
            timeline.callback(QDeclarativeTimeLineCallback(&data.move, fixupCallback, this));
            if (!flickingHorizontally && q->xflick()) {
                flickingHorizontally = true;
                emit q->flickingChanged();
                emit q->flickingHorizontallyChanged();
                emit q->flickStarted();
            }
            if (!flickingVertically && q->yflick()) {
                flickingVertically = true;
                emit q->flickingChanged();
                emit q->flickingVerticallyChanged();
                emit q->flickStarted();
            }
            correctFlick = true;
        } else {
            // reevaluate the target boundary.
            qreal newtarget = data.flickTarget;
            if (snapMode != QSGListView::NoSnap || highlightRange == QSGListView::StrictlyEnforceRange) {
                qreal tempFlickTarget = isRightToLeft() ? -data.flickTarget+size() : data.flickTarget;
                newtarget = -snapPosAt(-(tempFlickTarget - highlightStart)) + highlightStart;
                newtarget = isRightToLeft() ? -newtarget+size() : newtarget;
            }
            if (velocity < 0 && newtarget <= maxExtent)
                newtarget = maxExtent - overshootDist;
            else if (velocity > 0 && newtarget >= minExtent)
                newtarget = minExtent + overshootDist;
            if (newtarget == data.flickTarget) { // boundary unchanged - nothing to do
                if (qAbs(velocity) < MinimumFlickVelocity)
                    correctFlick = false;
                return;
            }
            data.flickTarget = newtarget;
            qreal dist = -newtarget + data.move.value();
            if ((v < 0 && dist < 0) || (v > 0 && dist > 0)) {
                correctFlick = false;
                timeline.reset(data.move);
                fixup(data, minExtent, maxExtent);
                return;
            }
            timeline.reset(data.move);
            timeline.accelDistance(data.move, v, -dist);
            timeline.callback(QDeclarativeTimeLineCallback(&data.move, fixupCallback, this));
        }
    } else {
        correctFlick = false;
        timeline.reset(data.move);
        fixup(data, minExtent, maxExtent);
    }
}

//----------------------------------------------------------------------------

QSGListView::QSGListView(QSGItem *parent)
    : QSGItemView(*(new QSGListViewPrivate), parent)
{
}

QSGListView::~QSGListView()
{
}

void QSGListView::setHighlightFollowsCurrentItem(bool autoHighlight)
{
    Q_D(QSGListView);
    if (d->autoHighlight != autoHighlight) {
        if (!autoHighlight) {
            if (d->highlightPosAnimator)
                d->highlightPosAnimator->stop();
            if (d->highlightSizeAnimator)
                d->highlightSizeAnimator->stop();
        }
        QSGItemView::setHighlightFollowsCurrentItem(autoHighlight);
    }
}

qreal QSGListView::spacing() const
{
    Q_D(const QSGListView);
    return d->spacing;
}

void QSGListView::setSpacing(qreal spacing)
{
    Q_D(QSGListView);
    if (spacing != d->spacing) {
        d->spacing = spacing;
        d->layout();
        emit spacingChanged();
    }
}

QSGListView::Orientation QSGListView::orientation() const
{
    Q_D(const QSGListView);
    return d->orient;
}

void QSGListView::setOrientation(QSGListView::Orientation orientation)
{
    Q_D(QSGListView);
    if (d->orient != orientation) {
        d->orient = orientation;
        if (d->orient == Vertical) {
            setContentWidth(-1);
            setFlickableDirection(VerticalFlick);
            setContentX(0);
        } else {
            setContentHeight(-1);
            setFlickableDirection(HorizontalFlick);
            setContentY(0);
        }
        d->regenerate();
        emit orientationChanged();
    }
}

QSGViewSection *QSGListView::sectionCriteria()
{
    Q_D(QSGListView);
    if (!d->sectionCriteria) {
        d->sectionCriteria = new QSGViewSection(this);
        connect(d->sectionCriteria, SIGNAL(propertyChanged()), this, SLOT(updateSections()));
    }
    return d->sectionCriteria;
}

QString QSGListView::currentSection() const
{
    Q_D(const QSGListView);
    return d->currentSection;
}

qreal QSGListView::highlightMoveSpeed() const
{
    Q_D(const QSGListView);
    return d->highlightMoveSpeed;
}

void QSGListView::setHighlightMoveSpeed(qreal speed)
{
    Q_D(QSGListView);
    if (d->highlightMoveSpeed != speed) {
        d->highlightMoveSpeed = speed;
        if (d->highlightPosAnimator)
            d->highlightPosAnimator->velocity = d->highlightMoveSpeed;
        emit highlightMoveSpeedChanged();
    }
}

void QSGListView::setHighlightMoveDuration(int duration)
{
    Q_D(QSGListView);
    if (d->highlightMoveDuration != duration) {
        if (d->highlightPosAnimator)
            d->highlightPosAnimator->userDuration = duration;
        QSGItemView::setHighlightMoveDuration(duration);
    }
}

qreal QSGListView::highlightResizeSpeed() const
{
    Q_D(const QSGListView);
    return d->highlightResizeSpeed;
}

void QSGListView::setHighlightResizeSpeed(qreal speed)
{
    Q_D(QSGListView);
    if (d->highlightResizeSpeed != speed) {
        d->highlightResizeSpeed = speed;
        if (d->highlightSizeAnimator)
            d->highlightSizeAnimator->velocity = d->highlightResizeSpeed;
        emit highlightResizeSpeedChanged();
    }
}

int QSGListView::highlightResizeDuration() const
{
    Q_D(const QSGListView);
    return d->highlightResizeDuration;
}

void QSGListView::setHighlightResizeDuration(int duration)
{
    Q_D(QSGListView);
    if (d->highlightResizeDuration != duration) {
        d->highlightResizeDuration = duration;
        if (d->highlightSizeAnimator)
            d->highlightSizeAnimator->userDuration = d->highlightResizeDuration;
        emit highlightResizeDurationChanged();
    }
}

QSGListView::SnapMode QSGListView::snapMode() const
{
    Q_D(const QSGListView);
    return d->snapMode;
}

void QSGListView::setSnapMode(SnapMode mode)
{
    Q_D(QSGListView);
    if (d->snapMode != mode) {
        d->snapMode = mode;
        emit snapModeChanged();
    }
}

void QSGListView::viewportMoved()
{
    Q_D(QSGListView);
    QSGItemView::viewportMoved();
    if (!d->itemCount)
        return;
    // Recursion can occur due to refill changing the content size.
    if (d->inViewportMoved)
        return;
    d->inViewportMoved = true;
    d->lazyRelease = true;
    d->refill();
    if (d->flickingHorizontally || d->flickingVertically || d->movingHorizontally || d->movingVertically)
        d->moveReason = QSGListViewPrivate::Mouse;
    if (d->moveReason != QSGListViewPrivate::SetIndex) {
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange && d->highlight) {
            // reposition highlight
            qreal pos = d->highlight->position();
            qreal viewPos;
            qreal highlightStart;
            qreal highlightEnd;
            if (d->isRightToLeft()) {
                // Handle Right-To-Left exceptions
                viewPos = -d->position()-d->size();
                highlightStart = d->highlightRangeStartValid ? d->size()-d->highlightRangeEnd : d->highlightRangeStart;
                highlightEnd = d->highlightRangeEndValid ? d->size()-d->highlightRangeStart : d->highlightRangeEnd;
            } else {
                viewPos = d->position();
                highlightStart = d->highlightRangeStart;
                highlightEnd = d->highlightRangeEnd;
            }
            if (pos > viewPos + highlightEnd - d->highlight->size())
                pos = viewPos + highlightEnd - d->highlight->size();
            if (pos < viewPos + highlightStart)
                pos = viewPos + highlightStart;
            if (pos != d->highlight->position()) {
                d->highlightPosAnimator->stop();
                static_cast<FxListItemSG*>(d->highlight)->setPosition(pos);
            } else {
                d->updateHighlight();
            }

            // update current index
            if (FxViewItem *snapItem = d->snapItemAt(d->highlight->position())) {
                if (snapItem->index >= 0 && snapItem->index != d->currentIndex)
                    d->updateCurrent(snapItem->index);
            }
        }
    }

    if ((d->flickingHorizontally || d->flickingVertically) && d->correctFlick && !d->inFlickCorrection) {
        d->inFlickCorrection = true;
        // Near an end and it seems that the extent has changed?
        // Recalculate the flick so that we don't end up in an odd position.
        if (yflick() && !d->vData.inOvershoot) {
            if (d->vData.velocity > 0) {
                const qreal minY = minYExtent();
                if ((minY - d->vData.move.value() < height()/2 || d->vData.flickTarget - d->vData.move.value() < height()/2)
                    && minY != d->vData.flickTarget)
                    d->flickY(-d->vData.smoothVelocity.value());
                d->bufferMode = QSGListViewPrivate::BufferBefore;
            } else if (d->vData.velocity < 0) {
                const qreal maxY = maxYExtent();
                if ((d->vData.move.value() - maxY < height()/2 || d->vData.move.value() - d->vData.flickTarget < height()/2)
                    && maxY != d->vData.flickTarget)
                    d->flickY(-d->vData.smoothVelocity.value());
                d->bufferMode = QSGListViewPrivate::BufferAfter;
            }
        }

        if (xflick() && !d->hData.inOvershoot) {
            if (d->hData.velocity > 0) {
                const qreal minX = minXExtent();
                if ((minX - d->hData.move.value() < width()/2 || d->hData.flickTarget - d->hData.move.value() < width()/2)
                    && minX != d->hData.flickTarget)
                    d->flickX(-d->hData.smoothVelocity.value());
                d->bufferMode = d->isRightToLeft() ? QSGListViewPrivate::BufferAfter : QSGListViewPrivate::BufferBefore;
            } else if (d->hData.velocity < 0) {
                const qreal maxX = maxXExtent();
                if ((d->hData.move.value() - maxX < width()/2 || d->hData.move.value() - d->hData.flickTarget < width()/2)
                    && maxX != d->hData.flickTarget)
                    d->flickX(-d->hData.smoothVelocity.value());
                d->bufferMode = d->isRightToLeft() ? QSGListViewPrivate::BufferBefore : QSGListViewPrivate::BufferAfter;
            }
        }
        d->inFlickCorrection = false;
    }
    d->inViewportMoved = false;
}

void QSGListView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QSGListView);
    if (d->model && d->model->count() && d->interactive) {
        if ((d->orient == QSGListView::Horizontal && !d->isRightToLeft() && event->key() == Qt::Key_Left)
                    || (d->orient == QSGListView::Horizontal && d->isRightToLeft() && event->key() == Qt::Key_Right)
                    || (d->orient == QSGListView::Vertical && event->key() == Qt::Key_Up)) {
            if (currentIndex() > 0 || (d->wrap && !event->isAutoRepeat())) {
                decrementCurrentIndex();
                event->accept();
                return;
            } else if (d->wrap) {
                event->accept();
                return;
            }
        } else if ((d->orient == QSGListView::Horizontal && !d->isRightToLeft() && event->key() == Qt::Key_Right)
                    || (d->orient == QSGListView::Horizontal && d->isRightToLeft() && event->key() == Qt::Key_Left)
                    || (d->orient == QSGListView::Vertical && event->key() == Qt::Key_Down)) {
            if (currentIndex() < d->model->count() - 1 || (d->wrap && !event->isAutoRepeat())) {
                incrementCurrentIndex();
                event->accept();
                return;
            } else if (d->wrap) {
                event->accept();
                return;
            }
        }
    }
    event->ignore();
    QSGItemView::keyPressEvent(event);
}

void QSGListView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QSGListView);
    if (d->isRightToLeft() && d->orient == QSGListView::Horizontal) {
        // maintain position relative to the right edge
        int dx = newGeometry.width() - oldGeometry.width();
        setContentX(contentX() - dx);
    }
    QSGItemView::geometryChanged(newGeometry, oldGeometry);
}


void QSGListView::incrementCurrentIndex()
{
    Q_D(QSGListView);
    int count = d->model ? d->model->count() : 0;
    if (count && (currentIndex() < count - 1 || d->wrap)) {
        d->moveReason = QSGListViewPrivate::SetIndex;
        int index = currentIndex()+1;
        setCurrentIndex((index >= 0 && index < count) ? index : 0);
    }
}

void QSGListView::decrementCurrentIndex()
{
    Q_D(QSGListView);
    int count = d->model ? d->model->count() : 0;
    if (count && (currentIndex() > 0 || d->wrap)) {
        d->moveReason = QSGListViewPrivate::SetIndex;
        int index = currentIndex()-1;
        setCurrentIndex((index >= 0 && index < count) ? index : count-1);
    }
}

void QSGListView::updateSections()
{
    Q_D(QSGListView);
    if (isComponentComplete() && d->model) {
        QList<QByteArray> roles;
        if (d->sectionCriteria && !d->sectionCriteria->property().isEmpty())
            roles << d->sectionCriteria->property().toUtf8();
        d->model->setWatchedRoles(roles);
        d->updateSections();
        if (d->itemCount)
            d->layout();
    }
}

void QSGListView::itemsInserted(int modelIndex, int count)
{
    Q_D(QSGListView);
    if (!isComponentComplete() || !d->model || !d->model->isValid())
        return;
    d->updateUnrequestedIndexes();
    d->moveReason = QSGListViewPrivate::Other;

    qreal tempPos = d->isRightToLeft() ? -d->position()-d->size() : d->position();
    int index = d->visibleItems.count() ? d->mapFromModel(modelIndex) : 0;
    if (index < 0) {
        int i = d->visibleItems.count() - 1;
        while (i > 0 && d->visibleItems.at(i)->index == -1)
            --i;
        if (i == 0 && d->visibleItems.first()->index == -1) {
            // there are no visible items except items marked for removal
            index = d->visibleItems.count();
        } else if (d->visibleItems.at(i)->index + 1 == modelIndex
            && d->visibleItems.at(i)->endPosition() <= d->buffer+tempPos+d->size()) {
            // Special case of appending an item to the model.
            index = d->visibleItems.count();
        } else {
            if (modelIndex < d->visibleIndex) {
                // Insert before visible items
                d->visibleIndex += count;
                for (int i = 0; i < d->visibleItems.count(); ++i) {
                    FxViewItem *item = d->visibleItems.at(i);
                    if (item->index != -1 && item->index >= modelIndex)
                        item->index += count;
                }
            }
            if (d->currentIndex >= modelIndex) {
                // adjust current item index
                d->currentIndex += count;
                if (d->currentItem)
                    d->currentItem->index = d->currentIndex;
                emit currentIndexChanged();
            }
            d->scheduleLayout();
            d->itemCount += count;
            emit countChanged();
            return;
        }
    }

    // index can be the next item past the end of the visible items list (i.e. appended)
    int pos = 0;
    if (d->visibleItems.count()) {
        pos = index < d->visibleItems.count() ? d->visibleItems.at(index)->position()
                                                : d->visibleItems.last()->endPosition()+d->spacing;
    }

    int initialPos = pos;
    int diff = 0;
    QList<FxListItemSG*> added;
    bool addedVisible = false;
    FxViewItem *firstVisible = d->firstVisibleItem();
    if (firstVisible && pos < firstVisible->position()) {
        // Insert items before the visible item.
        int insertionIdx = index;
        int i = 0;
        int from = tempPos - d->buffer;
        for (i = count-1; i >= 0 && pos > from; --i) {
            if (!addedVisible) {
                d->scheduleLayout();
                addedVisible = true;
            }
            FxListItemSG *item = static_cast<FxListItemSG*>(d->createItem(modelIndex + i));
            d->visibleItems.insert(insertionIdx, item);
            pos -= item->size() + d->spacing;
            item->setPosition(pos);
            index++;
        }
        if (i >= 0) {
            // If we didn't insert all our new items - anything
            // before the current index is not visible - remove it.
            while (insertionIdx--) {
                FxListItemSG *item = static_cast<FxListItemSG*>(d->visibleItems.takeFirst());
                if (item->index != -1)
                    d->visibleIndex++;
                d->releaseItem(item);
            }
        } else {
            // adjust pos of items before inserted items.
            for (int i = insertionIdx-1; i >= 0; i--) {
                FxListItemSG *listItem = static_cast<FxListItemSG*>(d->visibleItems.at(i));
                listItem->setPosition(listItem->position() - (initialPos - pos));
            }
        }
    } else {
        int i = 0;
        int to = d->buffer+tempPos+d->size();
        for (i = 0; i < count && pos <= to; ++i) {
            if (!addedVisible) {
                d->scheduleLayout();
                addedVisible = true;
            }
            FxListItemSG *item = static_cast<FxListItemSG*>(d->createItem(modelIndex + i));
            d->visibleItems.insert(index, item);
            item->setPosition(pos);
            added.append(item);
            pos += item->size() + d->spacing;
            ++index;
        }
        if (i != count) {
            // We didn't insert all our new items, which means anything
            // beyond the current index is not visible - remove it.
            while (d->visibleItems.count() > index)
                d->releaseItem(d->visibleItems.takeLast());
        }
        diff = pos - initialPos;
    }
    if (d->itemCount && d->currentIndex >= modelIndex) {
        // adjust current item index
        d->currentIndex += count;
        if (d->currentItem) {
            d->currentItem->index = d->currentIndex;
            static_cast<FxListItemSG *>(d->currentItem)->setPosition(static_cast<FxListItemSG *>(d->currentItem)->position() + diff);
        }
        emit currentIndexChanged();
    } else if (!d->itemCount && (!d->currentIndex || (d->currentIndex < 0 && !d->currentIndexCleared))) {
        d->updateCurrent(0);
    }
    // Update the indexes of the following visible items.
    for (; index < d->visibleItems.count(); ++index) {
        FxViewItem *item = d->visibleItems.at(index);
        if (d->currentItem && item->item != d->currentItem->item)
            static_cast<FxListItemSG*>(item)->setPosition(item->position() + diff);
        if (item->index != -1)
            item->index += count;
    }
    // everything is in order now - emit add() signal
    for (int j = 0; j < added.count(); ++j)
        added.at(j)->attached->emitAdd();

    d->updateSections();
    d->itemCount += count;
    emit countChanged();
}

void QSGListView::itemsRemoved(int modelIndex, int count)
{
    Q_D(QSGListView);
    if (!isComponentComplete() || !d->model || !d->model->isValid())
        return;
    d->moveReason = QSGListViewPrivate::Other;
    d->updateUnrequestedIndexes();
    d->itemCount -= count;

    FxViewItem *firstVisible = d->firstVisibleItem();
    int preRemovedSize = 0;
    bool removedVisible = false;
    // Remove the items from the visible list, skipping anything already marked for removal
    QList<FxViewItem*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxViewItem *item = *it;
        if (item->index == -1 || item->index < modelIndex) {
            // already removed, or before removed items
            ++it;
        } else if (item->index >= modelIndex + count) {
            // after removed items
            item->index -= count;
            ++it;
        } else {
            // removed item
            if (!removedVisible) {
                d->scheduleLayout();
                removedVisible = true;
            }
            item->attached->emitRemove();
            if (item->attached->delayRemove()) {
                item->index = -1;
                connect(item->attached, SIGNAL(delayRemoveChanged()), this, SLOT(destroyRemoved()), Qt::QueuedConnection);
                ++it;
            } else {
                if (item == firstVisible)
                    firstVisible = 0;
                if (firstVisible && item->position() < firstVisible->position())
                    preRemovedSize += item->size();
                it = d->visibleItems.erase(it);
                d->releaseItem(item);
            }
        }
    }

    if (firstVisible && d->visibleItems.first() != firstVisible)
        static_cast<FxListItemSG*>(d->visibleItems.first())->setPosition(d->visibleItems.first()->position() + preRemovedSize);

    // update visibleIndex
    bool haveVisibleIndex = false;
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            haveVisibleIndex = true;
            break;
        }
    }

    // fix current
    if (d->currentIndex >= modelIndex + count) {
        d->currentIndex -= count;
        if (d->currentItem)
            d->currentItem->index -= count;
        emit currentIndexChanged();
    } else if (d->currentIndex >= modelIndex && d->currentIndex < modelIndex + count) {
        // current item has been removed.
        if (d->currentItem) {
            d->currentItem->attached->setIsCurrentItem(false);
            d->releaseItem(d->currentItem);
            d->currentItem = 0;
        }
        d->currentIndex = -1;
        if (d->itemCount)
            d->updateCurrent(qMin(modelIndex, d->itemCount-1));
        else
            emit currentIndexChanged();
    }

    if (!haveVisibleIndex) {
        d->timeline.clear();
        if (removedVisible && d->itemCount == 0) {
            d->visibleIndex = 0;
            d->visiblePos = 0;
            d->setPosition(d->contentStartPosition());
            d->updateHeader();
            d->updateFooter();
        } else {
            if (modelIndex < d->visibleIndex)
                d->visibleIndex = modelIndex+1;
            d->visibleIndex = qMax(qMin(d->visibleIndex, d->itemCount-1), 0);
        }
    }

    d->updateSections();
    emit countChanged();
}

void QSGListView::itemsMoved(int from, int to, int count)
{
    Q_D(QSGListView);
    if (!isComponentComplete() || !d->isValid())
        return;
    d->updateUnrequestedIndexes();

    if (d->visibleItems.isEmpty()) {
        d->refill();
        return;
    }

    d->moveReason = QSGListViewPrivate::Other;

    bool movingBackwards = from > to;
    d->adjustMoveParameters(&from, &to, &count);

    QHash<int,FxViewItem*> moved;
    int moveBy = 0;
    FxViewItem *firstVisible = d->firstVisibleItem();
    int firstItemIndex = firstVisible ? firstVisible->index : -1;

    // if visibleItems.first() is above the content start pos, and the items
    // beneath it are moved, ensure this first item is later repositioned correctly
    // (to above the next visible item) so that subsequent layout() is correct
    bool repositionFirstItem = firstVisible
            && d->visibleItems.first()->position() < firstVisible->position()
            && from > d->visibleItems.first()->index;

    QList<FxViewItem*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxViewItem *item = *it;
        if (item->index >= from && item->index < from + count) {
            // take the items that are moving
            item->index += (to-from);
            moved.insert(item->index, item);
            if (repositionFirstItem)
                moveBy += item->size();
            it = d->visibleItems.erase(it);
        } else {
            // move everything after the moved items.
            if (item->index > from && item->index != -1)
                item->index -= count;
            ++it;
        }
    }

    int movedCount = 0;
    int endIndex = d->visibleIndex;
    it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxViewItem *item = *it;
        if (movedCount < count && item->index >= to && item->index < to + count) {
            // place items in the target position, reusing any existing items
            int targetIndex = item->index + movedCount;
            FxViewItem *movedItem = moved.take(targetIndex);
            if (!movedItem)
                movedItem = d->createItem(targetIndex);
            it = d->visibleItems.insert(it, movedItem);
            ++it;
            ++movedCount;
        } else {
            if (item->index != -1) {
                if (item->index >= to) {
                    // update everything after the moved items.
                    item->index += count;
                }
                endIndex = item->index;
            }
            ++it;
        }
    }

    // If we have moved items to the end of the visible items
    // then add any existing moved items that we have
    while (FxViewItem *item = moved.take(endIndex+1)) {
        d->visibleItems.append(item);
        ++endIndex;
    }

    // update visibleIndex
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    // if first visible item is moving but another item is moving up to replace it,
    // do this positioning now to avoid shifting all content forwards
    if (movingBackwards && firstItemIndex >= 0) {
        for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
            if ((*it)->index == firstItemIndex) {
                static_cast<FxListItemSG*>(*it)->setPosition(firstVisible->position());
                break;
            }
        }
    }

    // Fix current index
    if (d->currentIndex >= 0 && d->currentItem) {
        int oldCurrent = d->currentIndex;
        d->currentIndex = d->model->indexOf(d->currentItem->item, this);
        if (oldCurrent != d->currentIndex) {
            d->currentItem->index = d->currentIndex;
            emit currentIndexChanged();
        }
    }

    // Whatever moved items remain are no longer visible items.
    while (moved.count()) {
        int idx = moved.begin().key();
        FxViewItem *item = moved.take(idx);
        if (d->currentItem && item->item == d->currentItem->item)
            static_cast<FxListItemSG*>(item)->setPosition(d->positionAt(idx));
        d->releaseItem(item);
    }

    // Ensure we don't cause an ugly list scroll.
    if (d->visibleItems.count())
        static_cast<FxListItemSG*>(d->visibleItems.first())->setPosition(d->visibleItems.first()->position() + moveBy);

    d->updateSections();
    d->layout();
}


QSGListViewAttached *QSGListView::qmlAttachedProperties(QObject *obj)
{
    return new QSGListViewAttached(obj);
}

QT_END_NAMESPACE
