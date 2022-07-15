// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef GLOBALINSPECTOR_H
#define GLOBALINSPECTOR_H

#include "qquickwindowinspector.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QHash>
#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

namespace QmlJSDebugger {

class SelectionHighlight;

class GlobalInspector : public QObject
{
    Q_OBJECT
public:
    GlobalInspector(QObject *parent = nullptr) : QObject(parent), m_eventId(0) {}
    ~GlobalInspector();

    void setSelectedItems(const QList<QQuickItem *> &items);
    void showSelectedItemName(QQuickItem *item, const QPointF &point);

    void addWindow(QQuickWindow *window);
    void setParentWindow(QQuickWindow *window, QWindow *parentWindow);
    void setQmlEngine(QQuickWindow *window, QQmlEngine *engine);
    void removeWindow(QQuickWindow *window);
    void processMessage(const QByteArray &message);

Q_SIGNALS:
    void messageToClient(const QString &name, const QByteArray &data);

private:
    void sendResult(int requestId, bool success);
    void sendCurrentObjects(const QList<QObject *> &objects);
    void removeFromSelectedItems(QObject *object);
    QString titleForItem(QQuickItem *item) const;
    QString idStringForObject(QObject *obj) const;
    bool createQmlObject(int requestId, const QString &qml, QObject *parent,
                         const QStringList &importList, const QString &filename);
    bool destroyQmlObject(QObject *object, int requestId, int debugId);
    bool syncSelectedItems(const QList<QQuickItem *> &items);

    // Hash< object to be destroyed, QPair<destroy eventId, object debugId> >
    QList<QQuickItem *> m_selectedItems;
    QHash<QQuickItem *, SelectionHighlight *> m_highlightItems;
    QList<QQuickWindowInspector *> m_windowInspectors;
    int m_eventId;
};

} // QmlJSDebugger

QT_END_NAMESPACE

#endif // GLOBALINSPECTOR_H
