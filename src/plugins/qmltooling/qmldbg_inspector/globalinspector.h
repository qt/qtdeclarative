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
    GlobalInspector(QObject *parent = 0) : QObject(parent), m_eventId(0) {}
    ~GlobalInspector();

    void setSelectedItems(const QList<QQuickItem *> &items);
    void showSelectedItemName(QQuickItem *item, const QPointF &point);

    void addWindow(QQuickWindow *window);
    void setParentWindow(QQuickWindow *window, QWindow *parentWindow);
    void setQmlEngine(QQuickWindow *window, QQmlEngine *engine);
    void removeWindow(QQuickWindow *window);
    void processMessage(const QByteArray &message);

signals:
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
