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

#ifndef QQUICKPOSITIONERS_P_P_H
#define QQUICKPOSITIONERS_P_P_H

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

#include "qquickpositioners_p.h"
#include "qquickimplicitsizeitem_p_p.h"

#include <QtQuick/private/qdeclarativestate_p.h>
#include <private/qdeclarativetransitionmanager_p_p.h>
#include <private/qdeclarativestateoperations_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

class QQuickItemViewTransitioner;

class QQuickBasePositionerPrivate : public QQuickImplicitSizeItemPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickBasePositioner)

public:
    QQuickBasePositionerPrivate()
        : spacing(0), type(QQuickBasePositioner::None)
        , transitioner(0), positioningDirty(false)
        , doingPositioning(false), anchorConflict(false), layoutDirection(Qt::LeftToRight)
    {
    }

    void init(QQuickBasePositioner::PositionerType at)
    {
        type = at;
        childrenDoNotOverlap = true;
    }

    qreal spacing;

    QQuickBasePositioner::PositionerType type;
    QQuickItemViewTransitioner *transitioner;

    void watchChanges(QQuickItem *other);
    void unwatchChanges(QQuickItem* other);
    void setPositioningDirty() {
        Q_Q(QQuickBasePositioner);
        if (!positioningDirty) {
            positioningDirty = true;
            q->polish();
        }
    }

    bool positioningDirty : 1;
    bool doingPositioning : 1;
    bool anchorConflict : 1;

    Qt::LayoutDirection layoutDirection;

    void mirrorChange() {
        Q_Q(QQuickBasePositioner);
        if (type != QQuickBasePositioner::Vertical)
            q->prePositioning(); //Don't postpone, as it might be the only trigger for visible changes.
    }
    bool isLeftToRight() const {
        if (type == QQuickBasePositioner::Vertical)
            return true;
        else
            return effectiveLayoutMirror ? layoutDirection == Qt::RightToLeft : layoutDirection == Qt::LeftToRight;
    }

    virtual void itemSiblingOrderChanged(QQuickItem* other)
    {
        Q_UNUSED(other);
        setPositioningDirty();
    }

    void itemGeometryChanged(QQuickItem *, const QRectF &newGeometry, const QRectF &oldGeometry)
    {
        if (newGeometry.size() != oldGeometry.size())
            setPositioningDirty();
    }

    virtual void itemVisibilityChanged(QQuickItem *)
    {
        setPositioningDirty();
    }

    void itemDestroyed(QQuickItem *item)
    {
        Q_Q(QQuickBasePositioner);
        q->positionedItems.removeOne(QQuickBasePositioner::PositionedItem(item));
    }

    static Qt::LayoutDirection getLayoutDirection(const QQuickBasePositioner *positioner)
    {
        return positioner->d_func()->layoutDirection;
    }

    static Qt::LayoutDirection getEffectiveLayoutDirection(const QQuickBasePositioner *positioner)
    {
        if (positioner->d_func()->effectiveLayoutMirror)
            return positioner->d_func()->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
        else
            return positioner->d_func()->layoutDirection;
    }
};

QT_END_NAMESPACE

#endif // QQUICKPOSITIONERS_P_P_H
