// Commit: 95814418f9d6adeba365c795462e8afb00138211
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

#ifndef QSGGRIDVIEW_P_H
#define QSGGRIDVIEW_P_H

#include "qsgitemview_p.h"

#include <private/qdeclarativeguard_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QSGVisualModel;
class QSGGridViewAttached;
class QSGGridViewPrivate;
class Q_AUTOTEST_EXPORT QSGGridView : public QSGItemView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGGridView)

    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
    Q_PROPERTY(int cellWidth READ cellWidth WRITE setCellWidth NOTIFY cellWidthChanged)
    Q_PROPERTY(int cellHeight READ cellHeight WRITE setCellHeight NOTIFY cellHeightChanged)

    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged)

    Q_ENUMS(SnapMode)
    Q_ENUMS(Flow)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    QSGGridView(QSGItem *parent=0);
    ~QSGGridView();

    virtual void setHighlightFollowsCurrentItem(bool);
    virtual void setHighlightMoveDuration(int);

    enum Flow { LeftToRight, TopToBottom };
    Flow flow() const;
    void setFlow(Flow);

    int cellWidth() const;
    void setCellWidth(int);

    int cellHeight() const;
    void setCellHeight(int);

    enum SnapMode { NoSnap, SnapToRow, SnapOneRow };
    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    static QSGGridViewAttached *qmlAttachedProperties(QObject *);

public Q_SLOTS:
    void moveCurrentIndexUp();
    void moveCurrentIndexDown();
    void moveCurrentIndexLeft();
    void moveCurrentIndexRight();

Q_SIGNALS:
    void cellWidthChanged();
    void cellHeightChanged();
    void highlightMoveDurationChanged();
    void flowChanged();
    void snapModeChanged();

protected:
    virtual void viewportMoved();
    virtual void keyPressEvent(QKeyEvent *);

private Q_SLOTS:
    void itemsInserted(int index, int count);
    void itemsRemoved(int index, int count);
    void itemsMoved(int from, int to, int count);
};

class QSGGridViewAttached : public QSGItemViewAttached
{
    Q_OBJECT
public:
    QSGGridViewAttached(QObject *parent)
        : QSGItemViewAttached(parent), m_view(0) {}
    ~QSGGridViewAttached() {}

    Q_PROPERTY(QSGGridView *view READ view NOTIFY viewChanged)
    QSGGridView *view() { return m_view; }
    void setView(QSGGridView *view) {
        if (view != m_view) {
            m_view = view;
            emit viewChanged();
        }
    }

Q_SIGNALS:
    void viewChanged();

public:
    QDeclarativeGuard<QSGGridView> m_view;
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGGridView)
QML_DECLARE_TYPEINFO(QSGGridView, QML_HAS_ATTACHED_PROPERTIES)

QT_END_HEADER

#endif // QSGGRIDVIEW_P_H
