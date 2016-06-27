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

#include "qquickplatformfontdialog_p.h"

#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>

#ifdef QT_WIDGETS_LIB
#include "widgets/qwidgetplatformfontdialog_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype FontDialog
    \inherits Dialog
    \instantiates QQuickPlatformFontDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native font dialog.

    The FontDialog type provides a QML API for native platform font dialogs.

    \image qtlabsplatform-fontdialog-gtk.png

    To show a font dialog, construct an instance of FontDialog, set the
    desired properties, and call \l {Dialog::}{open()}. FontDialog emits
    the \l fontSelected() signal when the user has selected a font.

    \code
    MenuItem {
        text: "Font"
        onTriggered: fontDialog.open()
    }

    FontDialog {
        id: fontDialog
        currentFont.family: "Sans"
        onFontSelected: document.font = font
    }
    \endcode

    \section2 Availability

    A native platform font dialog is currently available on the following platforms:

    \list
    \li macOS
    \li Linux (when running with the GTK+ platform theme)
    \endlist

    \input includes/widgets.qdocinc 1

    \labs
*/

/*!
    \qmlsignal void Qt.labs.platform::FontDialog::fontSelected(font font)

    This signal is emitted just after the user has clicked \uicontrol OK to select a \a font.

    \sa currentFont
*/

QQuickPlatformFontDialog::QQuickPlatformFontDialog(QObject *parent)
    : QQuickPlatformDialog(parent), m_options(QFontDialogOptions::create())
{
    QPlatformDialogHelper *dialog = QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(QPlatformTheme::FontDialog);
#ifdef QT_WIDGETS_LIB
    if (!dialog)
        dialog = new QWidgetPlatformFontDialog(this);
#endif
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(dialog)) {
        connect(fontDialog, &QPlatformFontDialogHelper::currentFontChanged, this, &QQuickPlatformFontDialog::currentFontChanged);
        connect(fontDialog, &QPlatformFontDialogHelper::fontSelected, this, &QQuickPlatformFontDialog::fontSelected);
        fontDialog->setOptions(m_options);
    }
    setHandle(dialog);
}

/*!
    \qmlproperty font Qt.labs.platform::FontDialog::currentFont

    This property holds the currently selected font in the dialog.

    \sa fontSelected()
*/
QFont QQuickPlatformFontDialog::currentFont() const
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(handle()))
        return fontDialog->currentFont();
    return QFont();
}

void QQuickPlatformFontDialog::setCurrentFont(const QFont &font)
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(handle()))
        fontDialog->setCurrentFont(font);
}

/*!
    \qmlproperty flags Qt.labs.platform::FontDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value FontDialog.ScalableFonts Show scalable fonts.
    \value FontDialog.NonScalableFonts Show non-scalable fonts.
    \value FontDialog.MonospacedFonts Show monospaced fonts.
    \value FontDialog.ProportionalFonts Show proportional fonts.
    \value FontDialog.NoButtons Don't display \uicontrol OK and \uicontrol Cancel buttons (useful for "live dialogs").
*/
QFontDialogOptions::FontDialogOptions QQuickPlatformFontDialog::options() const
{
    return m_options->options();
}

void QQuickPlatformFontDialog::setOptions(QFontDialogOptions::FontDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickPlatformFontDialog::applyOptions()
{
    m_options->setWindowTitle(title());
}

QT_END_NAMESPACE
