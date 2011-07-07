/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDRAGEVENT_H
#define QDRAGEVENT_H

#include <QtCore/qcoreevent.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvariant.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGItem;

class Q_DECLARATIVE_EXPORT QSGEvent : public QEvent
{
public:
    // XXX: Merge types into QEvent or formally reserve a suitable range.
    // Alternatively start from QEvent::User and add a SGUser value for use by items.
    enum SGType
    {
        SGDragEnter = 600,
        SGDragExit,
        SGDragMove,
        SGDragDrop
    };

    QSGEvent(QSGEvent::SGType type) : QEvent(Type(type)) {}

    SGType type() const { return SGType(QEvent::type()); }
};

class Q_DECLARATIVE_EXPORT QSGDragEvent : public QSGEvent
{
public:
    QSGDragEvent(
            SGType type,
            const QPointF &scenePosition,
            const QVariant &data,
            const QStringList &keys,
            QSGItem *grabItem = 0)
        : QSGEvent(type)
        , _scenePosition(scenePosition),
          _data(data)
        , _keys(keys)
        , _dropItem(0)
        , _grabItem(grabItem)
    {
    }
    QSGDragEvent(SGType type, const QSGDragEvent &event)
        : QSGEvent(type)
        , _scenePosition(event._scenePosition)
        , _position(event._position)
        , _data(event._data)
        , _keys(event._keys)
        , _dropItem(event._dropItem)
        , _grabItem(event._grabItem)
    {
    }

    QVariant data() const { return _data; }

    qreal x() const { return _position.x(); }
    qreal y() const { return _position.y(); }
    QPointF position() const { return _position; }
    void setPosition(const QPointF &position) { _position = position; }

    QPointF scenePosition() const { return _scenePosition; }

    QStringList keys() const { return _keys; }

    QSGItem *dropItem() const { return _dropItem; }
    void setDropItem(QSGItem *dropItem) { _dropItem = dropItem; }

    QSGItem *grabItem() const { return _grabItem; }
    void setGrabItem(QSGItem *item) { _grabItem = item; }

private:
    QPointF _scenePosition;
    QPointF _position;
    QVariant _data;
    QStringList _keys;
    QSGItem *_dropItem;
    QSGItem *_grabItem;
};


QT_END_NAMESPACE

QT_END_HEADER

#endif

