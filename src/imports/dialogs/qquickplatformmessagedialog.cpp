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

#include "qquickplatformmessagedialog_p.h"
#include "qquickitem.h"

#include <private/qguiapplication_p.h>
#include <QWindow>
#include <QQuickView>
#include <QQuickWindow>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MessageDialog
    \instantiates QQuickPlatformMessageDialog
    \inqmlmodule QtQuick.Dialogs 1
    \ingroup dialogs
    \brief Dialog component for displaying popup messages.
    \since 5.2

    The most basic use case for a MessageDialog is a popup alert. It also
    allows the user to respond in various ways depending on which buttons are
    enabled. The dialog is initially invisible. You need to set the properties
    as desired first, then set \l visible to true or call \l open().

    Here is a minimal example to show an alert and exit after the user
    responds:

    \qml
    import QtQuick 2.2
    import QtQuick.Dialogs 1.1

    MessageDialog {
        id: messageDialog
        title: "May I have your attention please"
        text: "It's so cool that you are using Qt Quick."
        onAccepted: {
            console.log("And of course you could only agree.")
            Qt.quit()
        }
        Component.onCompleted: visible = true
    }
    \endqml

    A MessageDialog window is automatically transient for its parent window. So
    whether you declare the dialog inside an \l Item or inside a \l Window, the
    dialog will appear centered over the window containing the item, or over
    the Window that you declared.

    The implementation of MessageDialog will be a platform message dialog if
    possible. If that isn't possible, then it will try to instantiate a
    \l QMessageBox. If that also isn't possible, then it will fall back to a QML
    implementation, DefaultMessageDialog.qml. In that case you can customize the
    appearance by editing this file. DefaultMessageDialog.qml contains a Rectangle
    to hold the dialog's contents, because certain embedded systems do not
    support multiple top-level windows. When the dialog becomes visible, it
    will automatically be wrapped in a Window if possible, or simply reparented
    on top of the main window if there can only be one window.
*/

/*!
    \qmlsignal QtQuick::Dialogs::MessageDialog::accepted

    This handler is called when the user has pressed OK.
*/

/*!
    \qmlsignal QtQuick::Dialogs::MessageDialog::rejected

    This handler is called when the user has dismissed the dialog,
    either by closing the dialog window or by pressing the Cancel button.
*/

/*!
    \class QQuickPlatformMessageDialog
    \inmodule QtQuick.Dialogs
    \internal

    \brief The QQuickPlatformMessageDialog class provides a message dialog

    The dialog is implemented via the QPlatformMessageDialogHelper when possible;
    otherwise it falls back to a QMessageBox or a QML implementation.

    \since 5.2
*/

/*!
    Constructs a file dialog with parent window \a parent.
*/
QQuickPlatformMessageDialog::QQuickPlatformMessageDialog(QObject *parent) :
    QQuickAbstractMessageDialog(parent)
{
}

/*!
    Destroys the file dialog.
*/
QQuickPlatformMessageDialog::~QQuickPlatformMessageDialog()
{
    if (m_dlgHelper)
        m_dlgHelper->hide();
    delete m_dlgHelper;
}

QPlatformMessageDialogHelper *QQuickPlatformMessageDialog::helper()
{
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        m_parentWindow = parentItem->window();

    if ( !m_dlgHelper && QGuiApplicationPrivate::platformTheme()->
            usePlatformNativeDialog(QPlatformTheme::MessageDialog) ) {
        m_dlgHelper = static_cast<QPlatformMessageDialogHelper *>(QGuiApplicationPrivate::platformTheme()
           ->createPlatformDialogHelper(QPlatformTheme::MessageDialog));
        if (!m_dlgHelper)
            return m_dlgHelper;
        connect(m_dlgHelper, SIGNAL(accept()), this, SLOT(accept()));
        connect(m_dlgHelper, SIGNAL(reject()), this, SLOT(reject()));
    }

    return m_dlgHelper;
}

/*!
    \qmlproperty bool MessageDialog::visible

    This property holds whether the dialog is visible. By default this is
    false.

    \sa modality
*/

/*!
    \qmlproperty Qt::WindowModality MessageDialog::modality

    Whether the dialog should be shown modal with respect to the window
    containing the dialog's parent Item, modal with respect to the whole
    application, or non-modal.

    By default it is \c Qt.WindowModal.

    Modality does not mean that there are any blocking calls to wait for the
    dialog to be accepted or rejected; it's only that the user will be
    prevented from interacting with the parent window and/or the application
    windows at the same time.
*/

/*!
    \qmlmethod void MessageDialog::open()

    Shows the dialog to the user. It is equivalent to setting \l visible to
    true.
*/

/*!
    \qmlmethod void MessageDialog::close()

    Closes the dialog.
*/

/*!
    \qmlproperty string MessageDialog::title

    The title of the dialog window.
*/

QT_END_NAMESPACE
