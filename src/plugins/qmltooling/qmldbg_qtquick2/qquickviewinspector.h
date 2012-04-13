/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKVIEWINSPECTOR_H
#define QQUICKVIEWINSPECTOR_H

#include "abstractviewinspector.h"

#include <QtCore/QWeakPointer>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QQuickView;
class QQuickItem;
QT_END_NAMESPACE

namespace QmlJSDebugger {
namespace QtQuick2 {

class InspectTool;
class SelectionHighlight;

class QQuickViewInspector : public AbstractViewInspector
{
    Q_OBJECT
public:
    explicit QQuickViewInspector(QQuickView *view, QObject *parent = 0);

    // AbstractViewInspector
    void changeCurrentObjects(const QList<QObject*> &objects);
    void reloadView();
    void reparentQmlObject(QObject *object, QObject *newParent);
    void changeTool(InspectorProtocol::Tool tool);
    Qt::WindowFlags windowFlags() const;
    void setWindowFlags(Qt::WindowFlags flags);
    QQmlEngine *declarativeEngine() const;

    QQuickView *view() const { return m_view; }
    QQuickItem *overlay() const { return m_overlay; }

    QQuickItem *topVisibleItemAt(const QPointF &pos) const;
    QList<QQuickItem *> itemsAt(const QPointF &pos) const;

    QList<QQuickItem *> selectedItems() const;
    void setSelectedItems(const QList<QQuickItem*> &items);

    QString titleForItem(QQuickItem *item) const;

protected:
    bool eventFilter(QObject *obj, QEvent *event);

    bool mouseMoveEvent(QMouseEvent *);

private slots:
    void removeFromSelectedItems(QObject *);

private:
    bool syncSelectedItems(const QList<QQuickItem*> &items);

    QQuickView *m_view;
    QQuickItem *m_overlay;

    InspectTool *m_inspectTool;

    QList<QWeakPointer<QQuickItem> > m_selectedItems;
    QHash<QQuickItem*, SelectionHighlight*> m_highlightItems;

    bool m_designMode;
};

} // namespace QtQuick2
} // namespace QmlJSDebugger

#endif // QQUICKVIEWINSPECTOR_H
