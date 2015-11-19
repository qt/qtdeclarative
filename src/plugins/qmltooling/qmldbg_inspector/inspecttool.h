/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef INSPECTTOOL_H
#define INSPECTTOOL_H

#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QQuickItem;
class QMouseEvent;
class QKeyEvent;
class QTouchEvent;

namespace QmlJSDebugger {

class GlobalInspector;
class QQuickWindowInspector;
class HoverHighlight;

class InspectTool : public QObject
{
    Q_OBJECT
public:
    InspectTool(QQuickWindowInspector *inspector, QQuickWindow *view);

    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void hoverMoveEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *) {}
    void keyReleaseEvent(QKeyEvent *);
    void touchEvent(QTouchEvent *event);

private:
    void selectNextItem();
    void selectItem();
    void showItemName();

    QQuickWindowInspector *inspector() const;
    GlobalInspector *globalInspector() const;

    bool m_tapEvent;
    QPointer<QQuickItem> m_contentItem;
    QPointF m_mousePosition;
    ulong m_touchTimestamp;
    QTimer m_nameDisplayTimer;

    HoverHighlight *m_hoverHighlight;
    QQuickItem *m_lastItem;
    QQuickItem *m_lastClickedItem;
};

} // namespace QmlJSDebugger

QT_END_NAMESPACE

#endif // INSPECTTOOL_H
