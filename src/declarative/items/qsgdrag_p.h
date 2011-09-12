// Commit: c6e6a35aeb8794d68a3ca0c4e27a3a1181c066b5
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

#ifndef QSGDRAG_P_H
#define QSGDRAG_P_H

#include <qsgitem.h>

#include <private/qv8engine_p.h>

#include <QtCore/qmimedata.h>
#include <QtCore/qstringlist.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGItem;
class QSGDrag;
class QSGDragPrivate;

class QSGDragGrabber
{
    class Item : public QDeclarativeGuard<QSGItem>
    {
    public:
        Item(QSGItem *item) : QDeclarativeGuard<QSGItem>(item) {}

        QIntrusiveListNode node;
    protected:
        void objectDestroyed(QSGItem *) { delete this; }
    };

    typedef QIntrusiveList<Item, &Item::node> ItemList;

public:
    QSGDragGrabber() : m_target(0) {}
    ~QSGDragGrabber() { while (!m_items.isEmpty()) delete m_items.first(); }


    QObject *target() const
    {
        if (m_target)
            return m_target;
        else if (!m_items.isEmpty())
            return *m_items.first();
        else
            return 0;
    }
    void setTarget(QObject *target) { m_target = target; }
    void resetTarget() { m_target = 0; }

    typedef ItemList::iterator iterator;
    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }

    void grab(QSGItem *item) { m_items.insert(new Item(item)); }
    iterator release(iterator at) { Item *item = *at; at = at.erase(); delete item; return at; }

private:

    ItemList m_items;
    QObject *m_target;
};

class QSGDropEventEx : public QDropEvent
{
public:
    void setProposedAction(Qt::DropAction action) { default_action = action; drop_action = action; }

    static void setProposedAction(QDropEvent *event, Qt::DropAction action) {
        static_cast<QSGDropEventEx *>(event)->setProposedAction(action);
    }

    void copyActions(const QDropEvent &from) {
        default_action = from.proposedAction(); drop_action = from.dropAction(); }

    static void copyActions(QDropEvent *to, const QDropEvent &from) {
        static_cast<QSGDropEventEx *>(to)->copyActions(from);
    }
};

class QSGDragMimeData : public QMimeData
{
    Q_OBJECT
public:
    QSGDragMimeData()
        : m_source(0)
    {
    }

    QStringList keys() const { return m_keys; }
    QObject *source() const { return m_source; }

private:
    QObject *m_source;
    Qt::DropActions m_supportedActions;
    QStringList m_keys;

    friend class QSGDragAttached;
    friend class QSGDragAttachedPrivate;
};

class QDeclarativeV8Function;

class QSGDragAttachedPrivate;
class QSGDragAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged RESET resetSource)
    Q_PROPERTY(QObject *target READ target NOTIFY targetChanged)
    Q_PROPERTY(QPointF hotSpot READ hotSpot WRITE setHotSpot NOTIFY hotSpotChanged)
    Q_PROPERTY(QStringList keys READ keys WRITE setKeys NOTIFY keysChanged)
    Q_PROPERTY(Qt::DropActions supportedActions READ supportedActions WRITE setSupportedActions NOTIFY supportedActionsChanged)
    Q_PROPERTY(Qt::DropAction proposedAction READ proposedAction WRITE setProposedAction NOTIFY proposedActionChanged)
public:
    QSGDragAttached(QObject *parent);
    ~QSGDragAttached();

    bool isActive() const;
    void setActive(bool active);

    QObject *source() const;
    void setSource(QObject *item);
    void resetSource();

    QObject *target() const;

    QPointF hotSpot() const;
    void setHotSpot(const QPointF &hotSpot);

    QStringList keys() const;
    void setKeys(const QStringList &keys);

    Qt::DropActions supportedActions() const;
    void setSupportedActions(Qt::DropActions actions);

    Qt::DropAction proposedAction() const;
    void setProposedAction(Qt::DropAction action);

    Q_INVOKABLE int drop();

    static QSGDragAttached *properties(QObject *obj);

public Q_SLOTS:
    void start(QDeclarativeV8Function *);
    void cancel();

Q_SIGNALS:
    void activeChanged();
    void sourceChanged();
    void targetChanged();
    void hotSpotChanged();
    void keysChanged();
    void supportedActionsChanged();
    void proposedActionChanged();

private:
    static QHash<QObject*, QSGDragAttached *> attachedProperties;

    Q_DECLARE_PRIVATE(QSGDragAttached)
};


QT_END_NAMESPACE

QT_END_HEADER

#endif
