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

#include "qquickplatformcolordialog_p.h"
#include "qquickitem.h"

#include <private/qguiapplication_p.h>
#include <QWindow>
#include <QQuickView>
#include <QQuickWindow>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ColorDialog
    \instantiates QQuickPlatformColorDialog
    \inqmlmodule QtQuick.Dialogs 1
    \ingroup dialogs
    \brief Dialog component for choosing a color.
    \since Qt 5.1

    ColorDialog allows the user to select a color. The dialog is initially
    invisible. You need to set the properties as desired first, then set
    \l visible to true or call \l open().

    Here is a minimal example to open a color dialog and exit after the user
    chooses a color:

    \qml
    import QtQuick 2.1
    import QtQuick.Dialogs 1.0

    ColorDialog {
        id: colorDialog
        title: "Please choose a color"
        onAccepted: {
            console.log("You chose: " + colorDialog.color)
            Qt.quit()
        }
        onRejected: {
            console.log("Canceled")
            Qt.quit()
        }
        Component.onCompleted: visible = true
    }
    \endqml

    A ColorDialog window is automatically transient for its parent window. So
    whether you declare the dialog inside an \l Item or inside a \l Window, the
    dialog will appear centered over the window containing the item, or over
    the Window that you declared.

    The implementation of ColorDialog will be a platform color dialog if
    possible. If that isn't possible, then it will try to instantiate a
    \l QColorDialog. If that also isn't possible, then it will fall back to a QML
    implementation, DefaultColorDialog.qml. In that case you can customize the
    appearance by editing this file. DefaultColorDialog.qml contains a Rectangle
    to hold the dialog's contents, because certain embedded systems do not
    support multiple top-level windows. When the dialog becomes visible, it
    will automatically be wrapped in a Window if possible, or simply reparented
    on top of the main window if there can only be one window.
*/

/*!
    \qmlsignal QtQuick::Dialogs::ColorDialog::accepted

    This handler is called when the user has finished using the
    dialog. You can then inspect the \l color property to get the selection.

    Example:

    \qml
    ColorDialog {
        onAccepted: { console.log("Selected color: " + color) }
    }
    \endqml
*/

/*!
    \qmlsignal QtQuick::Dialogs::ColorDialog::rejected

    This handler is called when the user has dismissed the dialog,
    either by closing the dialog window or by pressing the Cancel button.
*/

/*!
    \class QQuickPlatformColorDialog
    \inmodule QtQuick.Dialogs
    \internal

    \brief The QQuickPlatformColorDialog class provides a color dialog

    The dialog is implemented via the QPlatformColorDialogHelper when possible;
    otherwise it falls back to a QColorDialog or a QML implementation.

    \since 5.1
*/

/*!
    Constructs a color dialog with parent window \a parent.
*/
QQuickPlatformColorDialog::QQuickPlatformColorDialog(QObject *parent) :
    QQuickAbstractColorDialog(parent)
{
}

/*!
    Destroys the color dialog.
*/
QQuickPlatformColorDialog::~QQuickPlatformColorDialog()
{
    if (m_dlgHelper)
        m_dlgHelper->hide();
    delete m_dlgHelper;
}

QPlatformColorDialogHelper *QQuickPlatformColorDialog::helper()
{
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        m_parentWindow = parentItem->window();

    if ( !m_dlgHelper && QGuiApplicationPrivate::platformTheme()->
            usePlatformNativeDialog(QPlatformTheme::ColorDialog) ) {
        m_dlgHelper = static_cast<QPlatformColorDialogHelper *>(QGuiApplicationPrivate::platformTheme()
           ->createPlatformDialogHelper(QPlatformTheme::ColorDialog));
        if (!m_dlgHelper)
            return m_dlgHelper;
        connect(m_dlgHelper, SIGNAL(accept()), this, SLOT(accept()));
        connect(m_dlgHelper, SIGNAL(reject()), this, SLOT(reject()));
        connect(m_dlgHelper, SIGNAL(currentColorChanged(QColor)), this, SLOT(setColor(QColor)));
        connect(m_dlgHelper, SIGNAL(colorSelected(QColor)), this, SLOT(setColor(QColor)));
    }

    return m_dlgHelper;
}

/*!
    \qmlproperty bool ColorDialog::visible

    This property holds whether the dialog is visible. By default this is
    false.

    \sa modality
*/

/*!
    \qmlproperty Qt::WindowModality ColorDialog::modality

    Whether the dialog should be shown modal with respect to the window
    containing the dialog's parent Item, modal with respect to the whole
    application, or non-modal.

    By default it is \l NonModal.

    Modality does not mean that there are any blocking calls to wait for the
    dialog to be accepted or rejected; it's only that the user will be
    prevented from interacting with the parent window and/or the application
    windows at the same time.

    On MacOS the color dialog is only allowed to be non-modal.
*/

/*!
    \qmlmethod void ColorDialog::open()

    Shows the dialog to the user. It is equivalent to setting \l visible to
    true.
*/

/*!
    \qmlmethod void ColorDialog::close()

    Closes the dialog.
*/

/*!
    \qmlproperty string ColorDialog::title

    The title of the dialog window.
*/

/*!
    \qmlproperty bool ColorDialog::showAlphaChannel

    Whether the dialog will provide a means of changing the opacity.

    By default, this property is true. This property must be set to the desired
    value before opening the dialog. Usually the alpha channel is represented
    by an additional slider control.
*/

/*!
    \qmlproperty color ColorDialog::color

    The color which the user selected.
*/

QT_END_NAMESPACE
