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

#ifndef QQUICKWINDOWINSPECTOR_H
#define QQUICKWINDOWINSPECTOR_H

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class QQmlDebugService;
class QQuickWindow;
class QQmlEngine;
class QWindow;
class QQuickItem;

namespace QmlJSDebugger {

class InspectTool;
class GlobalInspector;

/*
 * The common code between QQuickView and QQuickView inspectors lives here,
 */
class QQuickWindowInspector : public QObject
{
    Q_OBJECT

public:
    explicit QQuickWindowInspector(QQuickWindow *quickWindow, QObject *parent = 0);

    QQuickItem *overlay() const { return m_overlay; }
    QQuickItem *topVisibleItemAt(const QPointF &pos) const;
    QList<QQuickItem *> itemsAt(const QPointF &pos) const;

    QQuickWindow *quickWindow() const;

    void setParentWindow(QWindow *parentWindow);
    void setShowAppOnTop(bool appOnTop);

    bool isEnabled() const;
    void setEnabled(bool enabled);

protected:
    bool eventFilter(QObject *, QEvent *);

private:
    QQuickItem *m_overlay;
    QQuickWindow *m_window;
    QWindow *m_parentWindow;
    InspectTool *m_tool;
};

} // namespace QmlJSDebugger

QT_END_NAMESPACE

#endif // QQUICKWINDOWINSPECTOR_H
