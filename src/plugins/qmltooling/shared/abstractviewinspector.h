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

#ifndef ABSTRACTVIEWINSPECTOR_H
#define ABSTRACTVIEWINSPECTOR_H

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "qmlinspectorconstants.h"

QT_BEGIN_NAMESPACE
class QQmlEngine;
class QQmlInspectorService;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QTouchEvent;

QT_END_NAMESPACE

namespace QmlJSDebugger {

class AbstractTool;

/*
 * The common code between QQuickView and QQuickView inspectors lives here,
 */
class AbstractViewInspector : public QObject
{
    Q_OBJECT

public:
    explicit AbstractViewInspector(QObject *parent = 0);

    void handleMessage(const QByteArray &message);

    void createQmlObject(const QString &qml, QObject *parent,
                         const QStringList &importList,
                         const QString &filename = QString());
    void clearComponentCache();

    bool enabled() const { return m_enabled; }

    void sendCurrentObjects(const QList<QObject*> &);

    void sendQmlFileReloaded(bool success);

    QString idStringForObject(QObject *obj) const;

    virtual void changeCurrentObjects(const QList<QObject*> &objects) = 0;
    virtual void reparentQmlObject(QObject *object, QObject *newParent) = 0;
    virtual Qt::WindowFlags windowFlags() const = 0;
    virtual void setWindowFlags(Qt::WindowFlags flags) = 0;
    virtual QQmlEngine *declarativeEngine() const = 0;
    virtual void reloadQmlFile(const QHash<QString, QByteArray> &changesHash) = 0;

    void appendTool(AbstractTool *tool);
    void removeTool(AbstractTool *tool);

protected:
    bool eventFilter(QObject *, QEvent *);

    virtual bool leaveEvent(QEvent *);
    virtual bool mousePressEvent(QMouseEvent *event);
    virtual bool mouseMoveEvent(QMouseEvent *event);
    virtual bool mouseReleaseEvent(QMouseEvent *event);
    virtual bool keyPressEvent(QKeyEvent *event);
    virtual bool keyReleaseEvent(QKeyEvent *keyEvent);
    virtual bool mouseDoubleClickEvent(QMouseEvent *event);
    virtual bool wheelEvent(QWheelEvent *event);
    virtual bool touchEvent(QTouchEvent *event);

private slots:
    void onQmlObjectDestroyed();

private:
    void setEnabled(bool value);

    void setShowAppOnTop(bool appOnTop);

    void setAnimationSpeed(qreal factor);

    bool m_enabled;

    QQmlInspectorService *m_debugService;
    QList<AbstractTool *> m_tools;
    int m_eventId;
    int m_reloadEventId;
    int m_destroyEventId;
};

} // namespace QmlJSDebugger

#endif // ABSTRACTVIEWINSPECTOR_H
