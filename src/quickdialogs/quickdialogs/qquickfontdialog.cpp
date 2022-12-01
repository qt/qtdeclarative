// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfontdialog_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype FontDialog
    \inherits Dialog
//!     \instantiates QQuickFontDialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.2
    \brief A font dialog.

    The FontDialog type provides a QML API for font dialogs.

    \image qtquickdialogs-fontdialog-gtk.png

    To show a font dialog, construct an instance of FontDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentFont
    property can be used to determine the currently selected font in the
    dialog. The \l selectedFont property is updated only after the final selection
    has been made by accepting the dialog.

    \code
    MenuItem {
        text: "Font"
        onTriggered: fontDialog.open()
    }

    FontDialog {
        id: fontDialog
        currentFont.family: document.font
    }

    MyDocument {
        id: document
        font: fontDialog.selectedFont
    }
    \endcode

    \section2 Availability

    A native platform font dialog is currently available on the following platforms:

    \list
    \li iOS
    \li Linux (when running with the GTK+ platform theme)
    \li macOS
    \endlist

    \include includes/fallback.qdocinc
*/

Q_LOGGING_CATEGORY(lcFontDialog, "qt.quick.dialogs.fontdialog")

QQuickFontDialog::QQuickFontDialog(QObject *parent)
    : QQuickAbstractDialog(QQuickDialogType::FontDialog, parent),
      m_options(QFontDialogOptions::create())
{
}

/*!
    \qmlproperty font QtQuick.Dialogs::FontDialog::currentFont
    \deprecated [6.3] Use \l selectedFont instead.

    This property holds the currently selected font in the dialog.

    The \c currentFont property is updated while the user is selecting
    fonts in the dialog, even before the final selection has been made.

    \sa selectedFont
*/

QFont QQuickFontDialog::currentFont() const
{
    return selectedFont();
}

void QQuickFontDialog::setCurrentFont(const QFont &font)
{
    setSelectedFont(font);
}

/*!
    \qmlproperty font QtQuick.Dialogs::FontDialog::selectedFont

    This property holds the currently selected font in the dialog.

    The \c selectedFont property is updated while the user is selecting
    fonts in the dialog, even before the final selection has been made.

    The \l {Dialog::}{accepted()} signal can be handled to get the final selection.
    When the user has clicked \uicontrol Open to accept a font, a signal handler
    for the \l {Dialog::}{accepted()} signal can query the selectedFont property to
    get the final font that was selected by the user.

    \sa currentFont, {Dialog::}{accepted()}
*/

QFont QQuickFontDialog::selectedFont() const
{
    return m_selectedFont;
}

void QQuickFontDialog::setSelectedFont(const QFont &font)
{
    if (font == m_selectedFont)
        return;

    m_selectedFont = font;

    emit selectedFontChanged();
    emit currentFontChanged();
}

/*!
    \qmlproperty flags QtQuick.Dialogs::FontDialog::options

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
    \value FontDialog.NoButtons Don't display \uicontrol Open and \uicontrol Cancel buttons (useful
   for "live dialogs").
    \value FontDialog.DontUseNativeDialog Forces the dialog to use a non-native quick implementation.
*/

QFontDialogOptions::FontDialogOptions QQuickFontDialog::options() const
{
    return m_options->options();
}

void QQuickFontDialog::setOptions(QFontDialogOptions::FontDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickFontDialog::resetOptions()
{
    setOptions({});
}

bool QQuickFontDialog::useNativeDialog() const
{
    return QQuickAbstractDialog::useNativeDialog()
            && !(m_options->testOption(QFontDialogOptions::DontUseNativeDialog));
}

void QQuickFontDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(dialog)) {
        connect(fontDialog, &QPlatformFontDialogHelper::currentFontChanged, this,
                [this, fontDialog]() { setSelectedFont(fontDialog->currentFont()); });
        connect(this, &QQuickFontDialog::selectedFontChanged, this,
                [this, fontDialog]() { fontDialog->setCurrentFont(m_selectedFont); });
        fontDialog->setOptions(m_options);
    }
}

void QQuickFontDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(dialog)) {
        fontDialog->setOptions(m_options); // setOptions only assigns a member and isn't virtual
        fontDialog->setCurrentFont(m_selectedFont);
    }

    QQuickAbstractDialog::onShow(dialog);
}

QT_END_NAMESPACE

#include "moc_qquickfontdialog_p.cpp"
