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

#ifndef QDECLARATIVELAYOUTS_P_H
#define QDECLARATIVELAYOUTS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qdeclarativepositioners_p.h"

#include "private/qdeclarativeimplicitsizeitem_p_p.h"

#include <QtQuick1/private/qdeclarativestate_p.h>
#include <QtQuick1/private/qdeclarativetransitionmanager_p_p.h>
#include <QtQuick1/private/qdeclarativestateoperations_p.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QDebug>

QT_BEGIN_NAMESPACE

class QDeclarative1BasePositionerPrivate : public QDeclarative1ImplicitSizeItemPrivate, public QDeclarativeItemChangeListener
{
    Q_DECLARE_PUBLIC(QDeclarative1BasePositioner)

public:
    QDeclarative1BasePositionerPrivate()
        : spacing(0), type(QDeclarative1BasePositioner::None)
        , moveTransition(0), addTransition(0), queuedPositioning(false)
        , doingPositioning(false), anchorConflict(false), layoutDirection(Qt::LeftToRight)
    {
    }

    void init(QDeclarative1BasePositioner::PositionerType at)
    {
        type = at;
    }

    int spacing;

    QDeclarative1BasePositioner::PositionerType type;
    QDeclarative1Transition *moveTransition;
    QDeclarative1Transition *addTransition;
    QDeclarative1StateOperation::ActionList addActions;
    QDeclarative1StateOperation::ActionList moveActions;
    QDeclarative1TransitionManager addTransitionManager;
    QDeclarative1TransitionManager moveTransitionManager;

    void watchChanges(QGraphicsObject *other);
    void unwatchChanges(QGraphicsObject* other);
    bool queuedPositioning : 1;
    bool doingPositioning : 1;
    bool anchorConflict : 1;

    Qt::LayoutDirection layoutDirection;


    void schedulePositioning()
    {
        Q_Q(QDeclarative1BasePositioner);
        if(!queuedPositioning){
            QTimer::singleShot(0,q,SLOT(prePositioning()));
            queuedPositioning = true;
        }
    }

    void mirrorChange() {
        Q_Q(QDeclarative1BasePositioner);
        if (type != QDeclarative1BasePositioner::Vertical)
            q->prePositioning();
    }
    bool isLeftToRight() const {
        if (type == QDeclarative1BasePositioner::Vertical)
            return true;
        else
            return effectiveLayoutMirror ? layoutDirection == Qt::RightToLeft : layoutDirection == Qt::LeftToRight;
    }

    virtual void itemSiblingOrderChanged(QDeclarativeItem* other)
    {
        Q_UNUSED(other);
        //Delay is due to many children often being reordered at once
        //And we only want to reposition them all once
        schedulePositioning();
    }

    void itemGeometryChanged(QDeclarativeItem *, const QRectF &newGeometry, const QRectF &oldGeometry)
    {
        Q_Q(QDeclarative1BasePositioner);
        if (newGeometry.size() != oldGeometry.size())
            q->prePositioning();
    }

    virtual void itemVisibilityChanged(QDeclarativeItem *)
    {
        schedulePositioning();
    }
    virtual void itemOpacityChanged(QDeclarativeItem *)
    {
        Q_Q(QDeclarative1BasePositioner);
        q->prePositioning();
    }

    void itemDestroyed(QDeclarativeItem *item)
    {
        Q_Q(QDeclarative1BasePositioner);
        q->positionedItems.removeOne(QDeclarative1BasePositioner::PositionedItem(item));
    }

    static Qt::LayoutDirection getLayoutDirection(const QDeclarative1BasePositioner *positioner)
    {
        return positioner->d_func()->layoutDirection;
    }

    static Qt::LayoutDirection getEffectiveLayoutDirection(const QDeclarative1BasePositioner *positioner)
    {
        if (positioner->d_func()->effectiveLayoutMirror)
            return positioner->d_func()->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
        else
            return positioner->d_func()->layoutDirection;
    }
};

QT_END_NAMESPACE
#endif
