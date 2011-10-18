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
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QSGView;
class QSGItem;
QT_END_NAMESPACE

namespace QmlJSDebugger {

class SGSelectionTool;
class SGSelectionHighlight;

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
    Qt::WindowFlags windowFlags() const;
    void setWindowFlags(Qt::WindowFlags flags);
    QDeclarativeEngine *declarativeEngine() const;

    QSGView *view() const { return m_view; }
    QSGItem *overlay() const { return m_overlay; }

    QSGItem *topVisibleItemAt(const QPointF &pos) const;
    QList<QSGItem *> itemsAt(const QPointF &pos) const;

    QList<QSGItem *> selectedItems() const;
    void setSelectedItems(const QList<QSGItem*> &items);

    QString titleForItem(QSGItem *item) const;

protected:
    bool eventFilter(QObject *obj, QEvent *event);

    bool mouseMoveEvent(QMouseEvent *);

private slots:
    void removeFromSelectedItems(QObject *);

private:
    bool syncSelectedItems(const QList<QSGItem*> &items);

    QSGView *m_view;
    QSGItem *m_overlay;

    SGSelectionTool *m_selectionTool;

    QList<QWeakPointer<QSGItem> > m_selectedItems;
    QHash<QSGItem*, SGSelectionHighlight*> m_highlightItems;

    bool m_designMode;
};

} // namespace QmlJSDebugger

#endif // QSGVIEWINSPECTOR_H
