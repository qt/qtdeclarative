// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    explicit QQuickWindowInspector(QQuickWindow *quickWindow, QObject *parent = nullptr);

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
