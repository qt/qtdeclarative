// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcolordialog_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ColorDialog
    \inherits Dialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.4
    \brief A color dialog.

    The ColorDialog type provides a QML API for color dialogs.

    \image qtquickdialogs-colordialog-gtk.png

    To show a color dialog, construct an instance of ColorDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l selectedColor
    property can be used to determine the initially selected color in the
    dialog.

    \code
    MenuItem {
        text: qsTr("Color")
        onTriggered: colorDialog.open()
    }

    ColorDialog {
        id: colorDialog
        selectedColor: document.color
        onAccepted: document.color = selectedColor
    }

    MyDocument {
        id: document
    }
    \endcode

    \section2 Availability

    A native platform color dialog is currently available on the following platforms:

    \list
    \li iOS
    \li Linux (when running with the GTK+ platform theme)
    \li macOS
    \endlist

    \include includes/fallback.qdocinc
*/

QQuickColorDialog::QQuickColorDialog(QObject *parent)
    : QQuickAbstractDialog(QQuickDialogType::ColorDialog, parent),
      m_options(QColorDialogOptions::create()),
      m_selectedColor(QColorConstants::White)
{
}

/*!
    \qmlproperty color QtQuick.Dialogs::ColorDialog::selectedColor

    This property holds the currently selected color in the dialog.

    The \l {Dialog::}{accepted()} signal can be handled to get the final selection.
    When the user has clicked \uicontrol Open to accept a color, a signal handler
    for the \l {Dialog::}{accepted()} signal can query the selectedColor property to
    get the final color that was selected by the user.

    \sa {Dialog::}{accepted()}
*/
QColor QQuickColorDialog::selectedColor() const
{
    return m_selectedColor;
}

void QQuickColorDialog::setSelectedColor(const QColor &color)
{
    if (color == m_selectedColor)
        return;

    m_selectedColor = color;

    emit selectedColorChanged();
}

/*!
    \qmlproperty flags QtQuick.Dialogs::ColorDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value ColorDialog.ShowAlphaChannel Show a slider and additional input fields for the alpha value.
    \value ColorDialog.NoButtons Don't display \uicontrol Open and \uicontrol Cancel buttons (useful
   for "live dialogs").
    \value ColorDialog.DontUseNativeDialog Forces the dialog to use a non-native quick implementation.
*/

QColorDialogOptions::ColorDialogOptions QQuickColorDialog::options() const
{
    return m_options->options();
}

void QQuickColorDialog::setOptions(QColorDialogOptions::ColorDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickColorDialog::resetOptions()
{
    setOptions({});
}

bool QQuickColorDialog::useNativeDialog() const
{
    return QQuickAbstractDialog::useNativeDialog()
            && !(m_options->testOption(QColorDialogOptions::DontUseNativeDialog));
}

void QQuickColorDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (auto colorDialog = qobject_cast<QPlatformColorDialogHelper *>(dialog)) {
        connect(colorDialog, &QPlatformColorDialogHelper::currentColorChanged, this,
                [this, colorDialog]() { setSelectedColor(colorDialog->currentColor()); });
        colorDialog->setOptions(m_options);
    }
}

void QQuickColorDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (auto colorDialog = qobject_cast<QPlatformColorDialogHelper *>(dialog)) {
        colorDialog->setOptions(m_options);
        colorDialog->setCurrentColor(m_selectedColor);
    }

    QQuickAbstractDialog::onShow(dialog);
}

QT_END_NAMESPACE
