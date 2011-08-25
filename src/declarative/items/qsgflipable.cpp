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

#include "qsgflipable_p.h"
#include "qsgitem_p.h"

#include <private/qdeclarativeguard_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>

QT_BEGIN_NAMESPACE

// XXX todo - i think this needs work and a bit of a re-think

class QSGLocalTransform : public QSGTransform
{
    Q_OBJECT
public:
    QSGLocalTransform(QObject *parent) : QSGTransform(parent) {}

    void setTransform(const QTransform &t) {
        transform = t;
        update();
    }
    virtual void applyTo(QMatrix4x4 *matrix) const {
        *matrix *= transform;
    }
private:
    QTransform transform;
};

class QSGFlipablePrivate : public QSGItemPrivate
{
    Q_DECLARE_PUBLIC(QSGFlipable)
public:
    QSGFlipablePrivate() : current(QSGFlipable::Front), front(0), back(0), sideDirty(false) {}

    virtual void transformChanged();
    void updateSide();
    void setBackTransform();

    QSGFlipable::Side current;
    QDeclarativeGuard<QSGLocalTransform> backTransform;
    QDeclarativeGuard<QSGItem> front;
    QDeclarativeGuard<QSGItem> back;

    bool sideDirty;
    bool wantBackXFlipped;
    bool wantBackYFlipped;
};

/*!
    \qmlclass Flipable QSGFlipable
    \inqmlmodule QtQuick 2
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

    \snippet doc/src/snippets/declarative/flipable/flipable.qml 0

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
QSGFlipable::QSGFlipable(QSGItem *parent)
: QSGItem(*(new QSGFlipablePrivate), parent)
{
}

QSGFlipable::~QSGFlipable()
{
}

/*!
  \qmlproperty Item QtQuick2::Flipable::front
  \qmlproperty Item QtQuick2::Flipable::back

  The front and back sides of the flipable.
*/

QSGItem *QSGFlipable::front()
{
    Q_D(const QSGFlipable);
    return d->front;
}

void QSGFlipable::setFront(QSGItem *front)
{
    Q_D(QSGFlipable);
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

QSGItem *QSGFlipable::back()
{
    Q_D(const QSGFlipable);
    return d->back;
}

void QSGFlipable::setBack(QSGItem *back)
{
    Q_D(QSGFlipable);
    if (d->back) {
        qmlInfo(this) << tr("back is a write-once property");
        return;
    }
    if (back == 0)
        return;
    d->back = back;
    d->back->setParentItem(this);

    d->backTransform = new QSGLocalTransform(d->back);
    d->backTransform->prependToItem(d->back);

    if (Front == d->current)
        d->back->setOpacity(0.);
    connect(back, SIGNAL(widthChanged()),
            this, SLOT(retransformBack()));
    connect(back, SIGNAL(heightChanged()),
            this, SLOT(retransformBack()));
    emit backChanged();
}

void QSGFlipable::retransformBack()
{
    Q_D(QSGFlipable);
    if (d->current == QSGFlipable::Back && d->back)
        d->setBackTransform();
}

/*!
  \qmlproperty enumeration QtQuick2::Flipable::side

  The side of the Flipable currently visible. Possible values are \c
  Flipable.Front and \c Flipable.Back.
*/
QSGFlipable::Side QSGFlipable::side() const
{
    Q_D(const QSGFlipable);

    const_cast<QSGFlipablePrivate *>(d)->updateSide();
    return d->current;
}

void QSGFlipablePrivate::transformChanged()
{
    Q_Q(QSGFlipable);

    if (!sideDirty) {
        sideDirty = true;
        q->polish();
    }

    QSGItemPrivate::transformChanged();
}

void QSGFlipable::updatePolish()
{
    Q_D(QSGFlipable);
    d->updateSide();
}

// determination on the currently visible side of the flipable
// has to be done on the complete scene transform to give
// correct results.
void QSGFlipablePrivate::updateSide()
{
    Q_Q(QSGFlipable);

    if (!sideDirty)
        return;

    sideDirty = false;

    QTransform sceneTransform;
    itemToParentTransform(sceneTransform);

    QPointF p1(0, 0);
    QPointF p2(1, 0);
    QPointF p3(1, 1);

    QPointF scenep1 = sceneTransform.map(p1);
    QPointF scenep2 = sceneTransform.map(p2);
    QPointF scenep3 = sceneTransform.map(p3);
#if 0
    p1 = q->mapToParent(p1);
    p2 = q->mapToParent(p2);
    p3 = q->mapToParent(p3);
#endif

    qreal cross = (scenep1.x() - scenep2.x()) * (scenep3.y() - scenep2.y()) -
                  (scenep1.y() - scenep2.y()) * (scenep3.x() - scenep2.x());

    wantBackYFlipped = scenep1.x() >= scenep2.x();
    wantBackXFlipped = scenep2.y() >= scenep3.y();

    QSGFlipable::Side newSide;
    if (cross > 0) {
        newSide = QSGFlipable::Back;
    } else {
        newSide = QSGFlipable::Front;
    }

    if (newSide != current) {
        current = newSide;
        if (current == QSGFlipable::Back && back)
            setBackTransform();
        if (front)
            front->setOpacity((current==QSGFlipable::Front)?1.:0.);
        if (back)
            back->setOpacity((current==QSGFlipable::Back)?1.:0.);
        emit q->sideChanged();
    }
}

/* Depends on the width/height of the back item, and so needs reevaulating
   if those change.
*/
void QSGFlipablePrivate::setBackTransform()
{
    QTransform mat;
    mat.translate(back->width()/2,back->height()/2);
    if (back->width() && wantBackYFlipped)
        mat.rotate(180, Qt::YAxis);
    if (back->height() && wantBackXFlipped)
        mat.rotate(180, Qt::XAxis);
    mat.translate(-back->width()/2,-back->height()/2);

    if (backTransform)
        backTransform->setTransform(mat);
}

QT_END_NAMESPACE

#include "qsgflipable.moc"
