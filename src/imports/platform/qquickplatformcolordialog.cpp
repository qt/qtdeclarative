/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickplatformcolordialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>

#ifdef QT_WIDGETS_LIB
#include "widgets/qwidgetplatformcolordialog_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype ColorDialog
    \inherits Dialog
    \instantiates QQuickPlatformColorDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native color dialog.

    The ColorDialog type provides a QML API for native platform color dialogs.

    \image qtlabsplatform-colordialog-gtk.png

    To show a color dialog, construct an instance of ColorDialog, set the
    desired properties, and call \l {Dialog::}{open()}. ColorDialog emits
    the \l colorSelected() signal when the user has selected a color.

    \code
    MenuItem {
        text: "Color"
        onTriggered: colorDialog.open()
    }

    ColorDialog {
        id: colorDialog
        currentColor: "black"
        onColorSelected: document.brush = color
    }
    \endcode

    \section2 Availability

    A native platform color dialog is currently available on the following platforms:

    \list
    \li macOS
    \li Linux (when running with the GTK+ platform theme)
    \endlist

    \input includes/widgets.qdocinc 1

    \labs
*/

/*!
    \qmlsignal void Qt.labs.platform::ColorDialog::colorSelected(color color)

    This signal is emitted just after the user has clicked \uicontrol OK to select a \a color.

    \sa currentColor
*/

Q_DECLARE_LOGGING_CATEGORY(qtLabsPlatformDialogs)

QQuickPlatformColorDialog::QQuickPlatformColorDialog(QObject *parent)
    : QQuickPlatformDialog(parent), m_options(QColorDialogOptions::create())
{
    QPlatformDialogHelper *dialog = QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(QPlatformTheme::ColorDialog);
#ifdef QT_WIDGETS_LIB
    if (!dialog)
        dialog = new QWidgetPlatformColorDialog(this);
#endif
    qCDebug(qtLabsPlatformDialogs) << "ColorDialog:" << dialog;

    if (QPlatformColorDialogHelper *colorDialog = qobject_cast<QPlatformColorDialogHelper *>(dialog)) {
        connect(colorDialog, &QPlatformColorDialogHelper::currentColorChanged, this, &QQuickPlatformColorDialog::currentColorChanged);
        connect(colorDialog, &QPlatformColorDialogHelper::colorSelected, this, &QQuickPlatformColorDialog::colorSelected);
        colorDialog->setOptions(m_options);
    }
    setHandle(dialog);
}

/*!
    \qmlproperty color Qt.labs.platform::ColorDialog::currentColor

    This property holds the currently selected color in the dialog.

    \sa colorSelected()
*/
QColor QQuickPlatformColorDialog::currentColor() const
{
    if (QPlatformColorDialogHelper *colorDialog = qobject_cast<QPlatformColorDialogHelper *>(handle()))
        return colorDialog->currentColor();
    return QColor();
}

void QQuickPlatformColorDialog::setCurrentColor(const QColor &color)
{
    if (QPlatformColorDialogHelper *colorDialog = qobject_cast<QPlatformColorDialogHelper *>(handle()))
        colorDialog->setCurrentColor(color);
}

/*!
    \qmlproperty flags Qt.labs.platform::ColorDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value ColorDialog.ShowAlphaChannel Allow the user to select the alpha component of a color.
    \value ColorDialog.NoButtons Don't display \uicontrol OK and \uicontrol Cancel buttons (useful for "live dialogs").
*/
QColorDialogOptions::ColorDialogOptions QQuickPlatformColorDialog::options() const
{
    return m_options->options();
}

void QQuickPlatformColorDialog::setOptions(QColorDialogOptions::ColorDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickPlatformColorDialog::applyOptions()
{
    m_options->setWindowTitle(title());
}

QT_END_NAMESPACE
