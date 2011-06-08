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

#ifndef QSGVIEWINSPECTOR_H
#define QSGVIEWINSPECTOR_H

#include "abstractviewinspector.h"

#include <QtCore/QWeakPointer>

QT_BEGIN_NAMESPACE

class QMouseEvent;
class QKeyEvent;
class QWheelEvent;

class QSGView;
class QSGItem;

class SGAbstractTool;
class SGSelectionTool;

class SGViewInspector : public AbstractViewInspector
{
    Q_OBJECT
public:
    explicit SGViewInspector(QSGView *view, QObject *parent = 0);

    // AbstractViewInspector
    void changeCurrentObjects(const QList<QObject*> &objects);
    void reloadView();
    void reparentQmlObject(QObject *object, QObject *newParent);
    void changeTool(InspectorProtocol::Tool tool);
    QWidget *viewWidget() const;
    QDeclarativeEngine *declarativeEngine() const;

    QSGView *view() const { return m_view; }
    QSGItem *overlay() const { return m_overlay; }

    QList<QSGItem *> selectedItems() const;
    void setSelectedItems(const QList<QSGItem*> &items);

    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void removeFromSelectedItems(QObject *);

private:
    bool leaveEvent(QEvent *);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent(QMouseEvent *event);
    bool keyPressEvent(QKeyEvent *event);
    bool keyReleaseEvent(QKeyEvent *keyEvent);
    bool mouseDoubleClickEvent(QMouseEvent *event);
    bool wheelEvent(QWheelEvent *event);

    QSGView *m_view;
    QSGItem *m_overlay;
    SGAbstractTool *m_currentTool;

    SGSelectionTool *m_selectionTool;

    QList<QWeakPointer<QSGItem> > m_selectedItems;

    bool m_designMode;
};

QT_END_NAMESPACE

#endif // QSGVIEWINSPECTOR_H
