/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickabstractdialog_p.h"
#include "qquickitem.h"

#include <private/qguiapplication_p.h>
#include <QWindow>
#include <QQmlComponent>
#include <QQuickWindow>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

QQmlComponent *QQuickAbstractDialog::m_decorationComponent(0);

QQuickAbstractDialog::QQuickAbstractDialog(QObject *parent)
    : QObject(parent)
    , m_parentWindow(0)
    , m_visible(false)
    , m_modality(Qt::WindowModal)
    , m_qmlImplementation(0)
    , m_dialogWindow(0)
    , m_contentItem(0)
    , m_windowDecoration(0)
    , m_hasNativeWindows(QGuiApplicationPrivate::platformIntegration()->
         hasCapability(QPlatformIntegration::MultipleWindows))
{
}

QQuickAbstractDialog::~QQuickAbstractDialog()
{
}

void QQuickAbstractDialog::setVisible(bool v)
{
    if (m_visible == v) return;
    m_visible = v;
    if (helper()) {
        if (v) {
            Qt::WindowFlags flags = Qt::Dialog;
            if (!title().isEmpty())
                flags |= Qt::WindowTitleHint;
            m_visible = helper()->show(flags, m_modality, parentWindow());
        } else {
            helper()->hide();
        }
    } else {
        // For a pure QML implementation, there is no helper.
        // But m_implementation is probably either an Item or a Window at this point.
        if (!m_dialogWindow) {
            m_dialogWindow = qobject_cast<QWindow *>(m_qmlImplementation);
            if (!m_dialogWindow) {
                m_contentItem = qobject_cast<QQuickItem *>(m_qmlImplementation);
                if (m_contentItem) {
                    m_dialogWindow = m_contentItem->window();
                    // An Item-based dialog implementation doesn't come with a window, so
                    // we have to instantiate one iff the platform allows it.
                    if (!m_dialogWindow && m_hasNativeWindows) {
                        QQuickWindow *win = new QQuickWindow;
                        ((QObject *)win)->setParent(this); // memory management only
                        m_dialogWindow = win;
                        m_contentItem->setParentItem(win->contentItem());
                        m_dialogWindow->setMinimumSize(QSize(m_contentItem->implicitWidth(), m_contentItem->implicitHeight()));
                        connect(win, SIGNAL(widthChanged(int)), this, SLOT(windowGeometryChanged()));
                        connect(win, SIGNAL(heightChanged(int)), this, SLOT(windowGeometryChanged()));
                    }

                    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());

                    // If the platform does not support multiple windows, but the dialog is
                    // implemented as an Item, then try to decorate it as a fake window and make it visible.
                    if (parentItem && !m_dialogWindow && !m_windowDecoration) {
                        if (m_decorationComponent) {
                            if (m_decorationComponent->isLoading())
                                connect(m_decorationComponent, SIGNAL(statusChanged(QQmlComponent::Status)),
                                        this, SLOT(decorationLoaded()));
                            else
                                decorationLoaded();
                        }
                        // Window decoration wasn't possible, so just reparent it into the scene
                        else {
                            m_contentItem->setParentItem(parentItem);
                            m_contentItem->setZ(10000);
                        }
                    }
                }
            }
            if (m_dialogWindow)
                connect(m_dialogWindow, SIGNAL(visibleChanged(bool)), this, SLOT(visibleChanged(bool)));
        }
        if (m_windowDecoration) {
            m_windowDecoration->setVisible(v);
        } else if (m_dialogWindow) {
            if (v) {
                m_dialogWindow->setTransientParent(parentWindow());
                m_dialogWindow->setTitle(title());
                m_dialogWindow->setModality(m_modality);
            }
            m_dialogWindow->setVisible(v);
        }
    }

    emit visibilityChanged();
}

void QQuickAbstractDialog::decorationLoaded()
{
    bool ok = false;
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (m_decorationComponent->isError()) {
        qWarning() << m_decorationComponent->errors();
    } else {
        QObject *decoration = m_decorationComponent->create();
        m_windowDecoration = qobject_cast<QQuickItem *>(decoration);
        if (m_windowDecoration) {
            m_windowDecoration->setParentItem(parentItem);
            // Give the window decoration its content to manage
            QVariant contentVariant;
            contentVariant.setValue<QQuickItem*>(m_contentItem);
            m_windowDecoration->setProperty("content", contentVariant);
            connect(m_windowDecoration, SIGNAL(dismissed()), this, SLOT(reject()));
            ok = true;
        } else {
            qWarning() << m_decorationComponent->url() <<
                "cannot be used as a window decoration because it's not an Item";
            delete m_windowDecoration;
            delete m_decorationComponent;
            m_decorationComponent = 0;
        }
    }
    // Window decoration wasn't possible, so just reparent it into the scene
    if (!ok) {
        m_contentItem->setParentItem(parentItem);
        m_contentItem->setZ(10000);
    }
}

void QQuickAbstractDialog::setModality(Qt::WindowModality m)
{
    if (m_modality == m) return;
    m_modality = m;
    emit modalityChanged();
}

void QQuickAbstractDialog::accept()
{
    setVisible(false);
    emit accepted();
}

void QQuickAbstractDialog::reject()
{
    setVisible(false);
    emit rejected();
}

void QQuickAbstractDialog::visibleChanged(bool v)
{
    m_visible = v;
    emit visibilityChanged();
}

void QQuickAbstractDialog::windowGeometryChanged()
{
    QQuickItem *content = qobject_cast<QQuickItem*>(m_qmlImplementation);
    if (m_dialogWindow && content) {
        content->setWidth(m_dialogWindow->width());
        content->setHeight(m_dialogWindow->height());
    }
}

QQuickWindow *QQuickAbstractDialog::parentWindow()
{
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        m_parentWindow = parentItem->window();
    return m_parentWindow;
}

void QQuickAbstractDialog::setQmlImplementation(QObject *obj)
{
    m_qmlImplementation = obj;
    if (m_dialogWindow) {
        disconnect(this, SLOT(visibleChanged(bool)));
        // Can't necessarily delete because m_dialogWindow might have been provided by the QML.
        m_dialogWindow = 0;
    }
}

int QQuickAbstractDialog::x() const
{
    if (m_dialogWindow)
        return m_dialogWindow->x();
    return -1;
}

int QQuickAbstractDialog::y() const
{
    if (m_dialogWindow)
        return m_dialogWindow->y();
    return -1;
}

int QQuickAbstractDialog::width() const
{
    if (m_dialogWindow)
        return m_dialogWindow->width();
    return -1;
}

int QQuickAbstractDialog::height() const
{
    if (m_dialogWindow)
        return m_dialogWindow->height();
    return -1;
}

void QQuickAbstractDialog::setX(int arg)
{
    if (helper()) {
        // TODO
    } else if (m_dialogWindow) {
        m_dialogWindow->setX(arg);
    }
    emit geometryChanged();
}

void QQuickAbstractDialog::setY(int arg)
{
    if (helper()) {
        // TODO
    } else if (m_dialogWindow) {
        m_dialogWindow->setY(arg);
    }
    emit geometryChanged();
}

void QQuickAbstractDialog::setWidth(int arg)
{
    if (helper()) {
        // TODO
    } else if (m_dialogWindow) {
        m_dialogWindow->setWidth(arg);
    }
    emit geometryChanged();
}

void QQuickAbstractDialog::setHeight(int arg)
{
    if (helper()) {
        // TODO
    } else if (m_dialogWindow) {
        m_dialogWindow->setHeight(arg);
    }
    emit geometryChanged();
}

QT_END_NAMESPACE
