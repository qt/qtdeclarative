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

#include "sgviewinspector.h"

#include "qdeclarativeinspectorprotocol.h"

#include "sgabstracttool.h"
#include "sgselectiontool.h"

#include <QtDeclarative/private/qdeclarativeinspectorservice_p.h>
#include <QtDeclarative/private/qdeclarativedebughelper_p.h>

#include <QtDeclarative/QSGView>
#include <QtDeclarative/QSGItem>
#include <QtGui/QMouseEvent>

#include <cfloat>

QT_BEGIN_NAMESPACE

SGViewInspector::SGViewInspector(QSGView *view, QObject *parent) :
    QObject(parent),
    m_view(view),
    m_overlay(new QSGItem),
    m_currentTool(0),
    m_selectionTool(new SGSelectionTool(this)),
    m_designMode(true)
{
    // Try to make sure the overlay is always on top
    m_overlay->setZ(FLT_MAX);

    // Make sure mouse hover events are received
    m_view->setMouseTracking(true);

    if (QSGItem *root = view->rootItem())
        m_overlay->setParentItem(root);

    view->installEventFilter(this);
    m_currentTool = m_selectionTool;
}

bool SGViewInspector::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_view || !m_designMode)
        return QObject::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::Leave:
        if (leaveEvent(event))
            return true;
        break;
    case QEvent::MouseButtonPress:
        if (mousePressEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    case QEvent::MouseMove:
        if (mouseMoveEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    case QEvent::MouseButtonRelease:
        if (mouseReleaseEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    case QEvent::KeyPress:
        if (keyPressEvent(static_cast<QKeyEvent*>(event)))
            return true;
        break;
    case QEvent::KeyRelease:
        if (keyReleaseEvent(static_cast<QKeyEvent*>(event)))
            return true;
        break;
    case QEvent::MouseButtonDblClick:
        if (mouseDoubleClickEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    case QEvent::Wheel:
        if (wheelEvent(static_cast<QWheelEvent*>(event)))
            return true;
        break;
    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

bool SGViewInspector::leaveEvent(QEvent *event)
{
    m_currentTool->leaveEvent(event);
    return true;
}

bool SGViewInspector::mousePressEvent(QMouseEvent *event)
{
    m_currentTool->mousePressEvent(event);
    return true;
}

bool SGViewInspector::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons()) {
        m_currentTool->mouseMoveEvent(event);
    } else {
        m_currentTool->hoverMoveEvent(event);
    }
    return true;
}

bool SGViewInspector::mouseReleaseEvent(QMouseEvent *event)
{
    m_currentTool->mouseReleaseEvent(event);
    return true;
}

bool SGViewInspector::keyPressEvent(QKeyEvent *event)
{
    m_currentTool->keyPressEvent(event);
    return true;
}

bool SGViewInspector::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    default:
        break;
    }

    m_currentTool->keyReleaseEvent(event);
    return true;
}

bool SGViewInspector::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_currentTool->mouseDoubleClickEvent(event);
    return true;
}

bool SGViewInspector::wheelEvent(QWheelEvent *event)
{
    m_currentTool->wheelEvent(event);
    return true;
}

QT_END_NAMESPACE
