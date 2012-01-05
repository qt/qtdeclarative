/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVELAYOUTS_H
#define QDECLARATIVELAYOUTS_H

#include "qdeclarativeimplicitsizeitem_p.h"

#include <QtQuick1/private/qdeclarativestate_p.h>
#include <private/qpodvector_p.h>

#include <QtCore/QObject>
#include <QtCore/QString>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QDeclarative1BasePositionerPrivate;

class Q_QTQUICK1_EXPORT QDeclarative1BasePositioner : public QDeclarative1ImplicitSizeItem
{
    Q_OBJECT

    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(QDeclarative1Transition *move READ move WRITE setMove NOTIFY moveChanged)
    Q_PROPERTY(QDeclarative1Transition *add READ add WRITE setAdd NOTIFY addChanged)
public:
    enum PositionerType { None = 0x0, Horizontal = 0x1, Vertical = 0x2, Both = 0x3 };
    QDeclarative1BasePositioner(PositionerType, QDeclarativeItem *parent);
    ~QDeclarative1BasePositioner();

    int spacing() const;
    void setSpacing(int);

    QDeclarative1Transition *move() const;
    void setMove(QDeclarative1Transition *);

    QDeclarative1Transition *add() const;
    void setAdd(QDeclarative1Transition *);

protected:
    QDeclarative1BasePositioner(QDeclarative1BasePositionerPrivate &dd, PositionerType at, QDeclarativeItem *parent);
    virtual void componentComplete();
    virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
    void finishApplyTransitions();

Q_SIGNALS:
    void spacingChanged();
    void moveChanged();
    void addChanged();

protected Q_SLOTS:
    void prePositioning();
    void graphicsWidgetGeometryChanged();

protected:
    virtual void doPositioning(QSizeF *contentSize)=0;
    virtual void reportConflictingAnchors()=0;
    class PositionedItem {
    public :
        PositionedItem(QGraphicsObject *i) : item(i), isNew(false), isVisible(true) {}
        bool operator==(const PositionedItem &other) const { return other.item == item; }
        QGraphicsObject *item;
        bool isNew;
        bool isVisible;
    };

    QPODVector<PositionedItem,8> positionedItems;
    void positionX(int,const PositionedItem &target);
    void positionY(int,const PositionedItem &target);

private:
    Q_DISABLE_COPY(QDeclarative1BasePositioner)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarative1BasePositioner)
};

class Q_AUTOTEST_EXPORT QDeclarative1Column : public QDeclarative1BasePositioner
{
    Q_OBJECT
public:
    QDeclarative1Column(QDeclarativeItem *parent=0);
protected:
    virtual void doPositioning(QSizeF *contentSize);
    virtual void reportConflictingAnchors();
private:
    Q_DISABLE_COPY(QDeclarative1Column)
};

class Q_AUTOTEST_EXPORT QDeclarative1Row: public QDeclarative1BasePositioner
{
    Q_OBJECT
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged REVISION 1)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY effectiveLayoutDirectionChanged REVISION 1)
public:
    QDeclarative1Row(QDeclarativeItem *parent=0);

    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection (Qt::LayoutDirection);
    Qt::LayoutDirection effectiveLayoutDirection() const;

Q_SIGNALS:
    Q_REVISION(1) void layoutDirectionChanged();
    Q_REVISION(1) void effectiveLayoutDirectionChanged();

protected:
    virtual void doPositioning(QSizeF *contentSize);
    virtual void reportConflictingAnchors();
private:
    Q_DISABLE_COPY(QDeclarative1Row)
};

class Q_AUTOTEST_EXPORT QDeclarative1Grid : public QDeclarative1BasePositioner
{
    Q_OBJECT
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged REVISION 1)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY effectiveLayoutDirectionChanged REVISION 1)
public:
    QDeclarative1Grid(QDeclarativeItem *parent=0);

    int rows() const {return m_rows;}
    void setRows(const int rows);

    int columns() const {return m_columns;}
    void setColumns(const int columns);

    Q_ENUMS(Flow)
    enum Flow { LeftToRight, TopToBottom };
    Flow flow() const;
    void setFlow(Flow);

    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection (Qt::LayoutDirection);
    Qt::LayoutDirection effectiveLayoutDirection() const;

Q_SIGNALS:
    void rowsChanged();
    void columnsChanged();
    void flowChanged();
    Q_REVISION(1) void layoutDirectionChanged();
    Q_REVISION(1) void effectiveLayoutDirectionChanged();

protected:
    virtual void doPositioning(QSizeF *contentSize);
    virtual void reportConflictingAnchors();

private:
    int m_rows;
    int m_columns;
    Flow m_flow;
    Q_DISABLE_COPY(QDeclarative1Grid)
};

class QDeclarative1FlowPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1Flow: public QDeclarative1BasePositioner
{
    Q_OBJECT
    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged REVISION 1)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY effectiveLayoutDirectionChanged REVISION 1)
public:
    QDeclarative1Flow(QDeclarativeItem *parent=0);

    Q_ENUMS(Flow)
    enum Flow { LeftToRight, TopToBottom };
    Flow flow() const;
    void setFlow(Flow);

    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection (Qt::LayoutDirection);
    Qt::LayoutDirection effectiveLayoutDirection() const;
Q_SIGNALS:
    void flowChanged();
    Q_REVISION(1) void layoutDirectionChanged();
    Q_REVISION(1) void effectiveLayoutDirectionChanged();

protected:
    virtual void doPositioning(QSizeF *contentSize);
    virtual void reportConflictingAnchors();
protected:
    QDeclarative1Flow(QDeclarative1FlowPrivate &dd, QDeclarativeItem *parent);
private:
    Q_DISABLE_COPY(QDeclarative1Flow)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarative1Flow)
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1Column)
QML_DECLARE_TYPE(QDeclarative1Row)
QML_DECLARE_TYPE(QDeclarative1Grid)
QML_DECLARE_TYPE(QDeclarative1Flow)

QT_END_HEADER

#endif
