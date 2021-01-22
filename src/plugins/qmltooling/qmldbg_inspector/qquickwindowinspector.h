/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
    bool eventFilter(QObject *, QEvent *) override;

private:
    QQuickItem *m_overlay;
    QQuickWindow *m_window;
    QWindow *m_parentWindow;
    InspectTool *m_tool;
};

} // namespace QmlJSDebugger

QT_END_NAMESPACE

#endif // QQUICKWINDOWINSPECTOR_H
