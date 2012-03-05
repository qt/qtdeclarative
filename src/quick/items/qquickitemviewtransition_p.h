/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKITEMVIEWTRANSITION_P_P_H
#define QQUICKITEMVIEWTRANSITION_P_P_H

#include <private/qquicktransitionmanager_p_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Quick)

class QQuickItem;
class QQuickViewItem;
class QQuickItemViewTransitionJob;


class QQuickItemViewTransitionChangeListener
{
public:
    QQuickItemViewTransitionChangeListener() {}
    virtual ~QQuickItemViewTransitionChangeListener() {}

    virtual void viewItemTransitionFinished(QQuickViewItem *item) = 0;
};


class QQuickItemViewTransitioner
{
public:
    enum TransitionType {
        NoTransition,
        PopulateTransition,
        AddTransition,
        MoveTransition,
        RemoveTransition
    };

    QQuickItemViewTransitioner();
    virtual ~QQuickItemViewTransitioner();

    bool canTransition(QQuickItemViewTransitioner::TransitionType type, bool asTarget) const;
    void transitionNextReposition(QQuickViewItem *item, QQuickItemViewTransitioner::TransitionType type, bool isTarget);

    QQuickTransition *transitionObject(QQuickItemViewTransitioner::TransitionType type, bool asTarget);
    const QList<int> &targetIndexes(QQuickItemViewTransitioner::TransitionType type) const;
    const QList<QObject *> &targetItems(QQuickItemViewTransitioner::TransitionType type) const;

    inline void setPopulateTransitionEnabled(bool b) { usePopulateTransition = b; }
    inline void setChangeListener(QQuickItemViewTransitionChangeListener *obj) { changeListener = obj; }

    QSet<QQuickItemViewTransitionJob *> runningJobs;

    QList<int> addTransitionIndexes;
    QList<int> moveTransitionIndexes;
    QList<int> removeTransitionIndexes;
    QList<QObject *> addTransitionTargets;
    QList<QObject *> moveTransitionTargets;
    QList<QObject *> removeTransitionTargets;

    QQuickTransition *populateTransition;
    QQuickTransition *addTransition;
    QQuickTransition *addDisplacedTransition;
    QQuickTransition *moveTransition;
    QQuickTransition *moveDisplacedTransition;
    QQuickTransition *removeTransition;
    QQuickTransition *removeDisplacedTransition;
    QQuickTransition *displacedTransition;

private:
    friend class QQuickItemViewTransitionJob;

    QQuickItemViewTransitionChangeListener *changeListener;
    bool usePopulateTransition;

    void finishedTransition(QQuickItemViewTransitionJob *job, QQuickViewItem *item);
};


/*
  An item in a view, that can be transitioned using QQuickViewTransitionJob.
  */
class QQuickViewItem
{
public:
    QQuickViewItem(QQuickItem *i);
    virtual ~QQuickViewItem();

    qreal itemX() const;
    qreal itemY() const;

    void moveTo(const QPointF &pos);
    void setVisible(bool visible);

    bool transitionScheduledOrRunning() const;
    bool transitionRunning() const;
    bool isPendingRemoval() const;

    bool prepareTransition(const QRectF &viewBounds);
    void startTransition(QQuickItemViewTransitioner *transitioner);
    void stopTransition();

    QPointF nextTransitionTo;
    QQuickItem *item;
    QQuickItemViewTransitionJob *transition;
    QQuickItemViewTransitioner::TransitionType nextTransitionType;
    int index;
    bool isTransitionTarget;
    bool nextTransitionToSet;

private:
    friend class QQuickItemViewTransitioner;
    friend class QQuickItemViewTransitionJob;
    void setNextTransition(QQuickItemViewTransitioner::TransitionType, bool isTargetItem);
    bool transitionWillChangePosition() const;
    void finishedTransition();
    void resetTransitionData();
};


class QQuickViewTransitionAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    Q_PROPERTY(QQuickItem* item READ item NOTIFY itemChanged)
    Q_PROPERTY(QPointF destination READ destination NOTIFY destinationChanged)

    Q_PROPERTY(QList<int> targetIndexes READ targetIndexes NOTIFY targetIndexesChanged)
    Q_PROPERTY(QQmlListProperty<QObject> targetItems READ targetItems NOTIFY targetItemsChanged)

public:
    QQuickViewTransitionAttached(QObject *parent);

    int index() const { return m_index; }
    QQuickItem *item() const { return m_item; }
    QPointF destination() const { return m_destination; }

    QList<int> targetIndexes() const { return m_targetIndexes; }
    QQmlListProperty<QObject> targetItems();

    static QQuickViewTransitionAttached *qmlAttachedProperties(QObject *);

signals:
    void indexChanged();
    void itemChanged();
    void destinationChanged();

    void targetIndexesChanged();
    void targetItemsChanged();

private:
    friend class QQuickItemViewTransitionJob;
    QPointF m_destination;
    QList<int> m_targetIndexes;
    QList<QObject *> m_targetItems;

    QQuickItem *m_item;
    int m_index;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickViewTransitionAttached)
QML_DECLARE_TYPEINFO(QQuickViewTransitionAttached, QML_HAS_ATTACHED_PROPERTIES)

QT_END_HEADER

#endif // QQUICKITEMVIEWTRANSITION_P_P_H
