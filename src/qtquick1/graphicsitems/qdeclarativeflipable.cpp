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

#include "QtQuick1/private/qdeclarativeflipable_p.h"

#include "QtQuick1/private/qdeclarativeitem_p.h"
#include "QtDeclarative/private/qdeclarativeguard_p.h"

#include <QtDeclarative/qdeclarativeinfo.h>

#include <QtGui/qgraphicstransform.h>

QT_BEGIN_NAMESPACE



class QDeclarative1FlipablePrivate : public QDeclarativeItemPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1Flipable)
public:
    QDeclarative1FlipablePrivate() : current(QDeclarative1Flipable::Front), front(0), back(0) {}

    void updateSceneTransformFromParent();
    void setBackTransform();

    QDeclarative1Flipable::Side current;
    QDeclarativeGuard<QGraphicsObject> front;
    QDeclarativeGuard<QGraphicsObject> back;

    bool wantBackXFlipped;
    bool wantBackYFlipped;
};

/*!
    \qmlclass Flipable QDeclarative1Flipable
    \inqmlmodule QtQuick 1
    \since QtQuick 1.0
    \ingroup qml-basic-interaction-elements
    \brief The Flipable item provides a surface that can be flipped.
    \inherits Item

    Flipable is an item that can be visibly "flipped" between its front and
    back sides, like a card. It is used together with \l Rotation, \l State
    and \l Transition elements to produce a flipping effect.

    The \l front and \l back properties are used to hold the items that are
    shown respectively on the front and back sides of the flipable item.

    \section1 Example Usage

    The following example shows a Flipable item that flips whenever it is
    clicked, rotating about the y-axis.

    This flipable item has a \c flipped boolean property that is toggled
    whenever the MouseArea within the flipable is clicked. When
    \c flipped is true, the item changes to the "back" state; in this
    state, the \c angle of the \l Rotation item is changed to 180
    degrees to produce the flipping effect. When \c flipped is false, the
    item reverts to the default state, in which the \c angle value is 0.

    \snippet doc/src/snippets/qtquick1/flipable/flipable.qml 0

    \image flipable.gif

    The \l Transition creates the animation that changes the angle over
    four seconds. When the item changes between its "back" and
    default states, the NumberAnimation animates the angle between
    its old and new values.

    See \l {QML States} for details on state changes and the default
    state, and \l {QML Animation and Transitions} for more information on how
    animations work within transitions.

    \sa {declarative/ui-components/flipable}{Flipable example}
*/

QDeclarative1Flipable::QDeclarative1Flipable(QDeclarativeItem *parent)
: QDeclarativeItem(*(new QDeclarative1FlipablePrivate), parent)
{
}

QDeclarative1Flipable::~QDeclarative1Flipable()
{
}

/*!
  \qmlproperty Item QtQuick1::Flipable::front
  \qmlproperty Item QtQuick1::Flipable::back

  The front and back sides of the flipable.
*/

QGraphicsObject *QDeclarative1Flipable::front()
{
    Q_D(const QDeclarative1Flipable);
    return d->front;
}

void QDeclarative1Flipable::setFront(QGraphicsObject *front)
{
    Q_D(QDeclarative1Flipable);
    if (d->front) {
        qmlInfo(this) << tr("front is a write-once property");
        return;
    }
    d->front = front;
    d->front->setParentItem(this);
    if (Back == d->current)
        d->front->setOpacity(0.);
    emit frontChanged();
}

QGraphicsObject *QDeclarative1Flipable::back()
{
    Q_D(const QDeclarative1Flipable);
    return d->back;
}

void QDeclarative1Flipable::setBack(QGraphicsObject *back)
{
    Q_D(QDeclarative1Flipable);
    if (d->back) {
        qmlInfo(this) << tr("back is a write-once property");
        return;
    }
    d->back = back;
    d->back->setParentItem(this);
    if (Front == d->current)
        d->back->setOpacity(0.);
    connect(back, SIGNAL(widthChanged()),
            this, SLOT(retransformBack()));
    connect(back, SIGNAL(heightChanged()),
            this, SLOT(retransformBack()));
    emit backChanged();
}

void QDeclarative1Flipable::retransformBack()
{
    Q_D(QDeclarative1Flipable);
    if (d->current == QDeclarative1Flipable::Back && d->back)
        d->setBackTransform();
}

/*!
  \qmlproperty enumeration QtQuick1::Flipable::side

  The side of the Flipable currently visible. Possible values are \c
  Flipable.Front and \c Flipable.Back.
*/
QDeclarative1Flipable::Side QDeclarative1Flipable::side() const
{
    Q_D(const QDeclarative1Flipable);
    if (d->dirtySceneTransform)
        const_cast<QDeclarative1FlipablePrivate *>(d)->ensureSceneTransform();

    return d->current;
}

// determination on the currently visible side of the flipable
// has to be done on the complete scene transform to give
// correct results.
void QDeclarative1FlipablePrivate::updateSceneTransformFromParent()
{
    Q_Q(QDeclarative1Flipable);

    QDeclarativeItemPrivate::updateSceneTransformFromParent();
    QPointF p1(0, 0);
    QPointF p2(1, 0);
    QPointF p3(1, 1);

    QPointF scenep1 = sceneTransform.map(p1);
    QPointF scenep2 = sceneTransform.map(p2);
    QPointF scenep3 = sceneTransform.map(p3);
    p1 = q->mapToParent(p1);
    p2 = q->mapToParent(p2);
    p3 = q->mapToParent(p3);

    qreal cross = (scenep1.x() - scenep2.x()) * (scenep3.y() - scenep2.y()) -
                  (scenep1.y() - scenep2.y()) * (scenep3.x() - scenep2.x());

    wantBackYFlipped = p1.x() >= p2.x();
    wantBackXFlipped = p2.y() >= p3.y();

    QDeclarative1Flipable::Side newSide;
    if (cross > 0) {
        newSide = QDeclarative1Flipable::Back;
    } else {
        newSide = QDeclarative1Flipable::Front;
    }

    if (newSide != current) {
        current = newSide;
        if (current == QDeclarative1Flipable::Back && back)
            setBackTransform();
        if (front)
            front->setOpacity((current==QDeclarative1Flipable::Front)?1.:0.);
        if (back)
            back->setOpacity((current==QDeclarative1Flipable::Back)?1.:0.);
        emit q->sideChanged();
    }
}

/* Depends on the width/height of the back item, and so needs reevaulating
   if those change.
*/
void QDeclarative1FlipablePrivate::setBackTransform()
{
    QTransform mat;
    QGraphicsItemPrivate *dBack = QGraphicsItemPrivate::get(back);
    mat.translate(dBack->width()/2,dBack->height()/2);
    if (dBack->width() && wantBackYFlipped)
        mat.rotate(180, Qt::YAxis);
    if (dBack->height() && wantBackXFlipped)
        mat.rotate(180, Qt::XAxis);
    mat.translate(-dBack->width()/2,-dBack->height()/2);
    back->setTransform(mat);
}



QT_END_NAMESPACE
