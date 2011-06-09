/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGDRAGTARGET_P_H
#define QSGDRAGTARGET_P_H

#include "qsgitem.h"
#include "qsgevent.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGDragTargetEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x)
    Q_PROPERTY(qreal y READ y)
    Q_PROPERTY(QVariant data READ data)
    Q_PROPERTY(QStringList keys READ keys)
    Q_PROPERTY(bool accepted READ accepted WRITE setAccepted)
public:
    QSGDragTargetEvent(QSGDragEvent *event) : _event(event) {}

    qreal x() const { return _event->x(); }
    qreal y() const { return _event->y(); }

    QVariant data() const { return _event->data(); }
    QStringList keys() const { return _event->keys(); }

    bool accepted() const { return _event->isAccepted(); }
    void setAccepted(bool accepted) { _event->setAccepted(accepted); }

private:
    QSGDragEvent *_event;
};

class QSGDragTargetPrivate;
class Q_AUTOTEST_EXPORT QSGDragTarget : public QSGItem
{
    Q_OBJECT
    Q_PROPERTY(bool containsDrag READ containsDrag NOTIFY containsDragChanged)
    Q_PROPERTY(QSGItem *dropItem READ dropItem WRITE setDropItem NOTIFY dropItemChanged RESET resetDropItem)
    Q_PROPERTY(QStringList keys READ keys WRITE setKeys NOTIFY keysChanged)
    Q_PROPERTY(qreal dragX READ dragX NOTIFY dragPositionChanged)
    Q_PROPERTY(qreal dragY READ dragY NOTIFY dragPositionChanged)
    Q_PROPERTY(QVariant dragData READ dragData NOTIFY dragDataChanged)

public:
    QSGDragTarget(QSGItem *parent=0);
    ~QSGDragTarget();

    bool containsDrag() const;
    void setContainsDrag(bool drag);

    QStringList keys() const;
    void setKeys(const QStringList &keys);

    QSGItem *dropItem() const;
    void setDropItem(QSGItem *item);
    void resetDropItem();

    qreal dragX() const;
    qreal dragY() const;
    QVariant dragData() const;

Q_SIGNALS:
    void containsDragChanged();
    void keysChanged();
    void dropItemChanged();
    void dragPositionChanged();
    void dragDataChanged();

    void entered(QSGDragTargetEvent *drag);
    void exited(QSGDragTargetEvent *drag);
    void positionChanged(QSGDragTargetEvent *drag);
    void dropped(QSGDragTargetEvent *drag);

protected:
    void dragMoveEvent(QSGDragEvent *event);
    void dragEnterEvent(QSGDragEvent *event);
    void dragExitEvent(QSGDragEvent *event);
    void dragDropEvent(QSGDragEvent *event);

private:
    Q_DISABLE_COPY(QSGDragTarget)
    Q_DECLARE_PRIVATE(QSGDragTarget)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGDragTargetEvent)
QML_DECLARE_TYPE(QSGDragTarget)

QT_END_HEADER

#endif // QSGDRAGTARGET_P_H
