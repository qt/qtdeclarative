// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformcolordialog_p.h"

#if QT_DEPRECATED_SINCE(6, 9)

QT_BEGIN_NAMESPACE

/*!
    \qmltype ColorDialog
    \inherits Dialog
//! \nativetype QQuickLabsPlatformColorDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \deprecated [6.9] Use QtQuick.Dialogs::ColorDialog instead.
    \brief A native color dialog.

    The ColorDialog type provides a QML API for native platform color dialogs.

    \image {qtlabsplatform-colordialog-gtk.png} {A native color dialog}

    To show a color dialog, construct an instance of ColorDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentColor
    property can be used to determine the currently selected color in the
    dialog. The \l color property is updated only after the final selection
    has been made by accepting the dialog.

    \code
    MenuItem {
        text: "Color"
        onTriggered: colorDialog.open()
    }

    ColorDialog {
        id: colorDialog
        currentColor: document.color
    }

    MyDocument {
        id: document
        color: colorDialog.color
    }
    \endcode

    \section2 Availability

    A native platform color dialog is currently available on the following platforms:

    \list
    \li iOS
    \li Linux (when running with the GTK+ platform theme)
    \li macOS
    \endlist

    \input includes/widgets.qdocinc 1

    \labs

    \sa QtQuick.Dialogs::ColorDialog
*/

QQuickLabsPlatformColorDialog::QQuickLabsPlatformColorDialog(QObject *parent)
    : QQuickLabsPlatformDialog(QPlatformTheme::ColorDialog, parent),
      m_options(QColorDialogOptions::create())
{
}

/*!
    \qmlproperty color Qt.labs.platform::ColorDialog::color

    This property holds the final accepted color.

    Unlike the \l currentColor property, the \c color property is not updated
    while the user is selecting colors in the dialog, but only after the final
    selection has been made. That is, when the user has clicked \uicontrol OK
    to accept a color. Alternatively, the \l {Dialog::}{accepted()} signal
    can be handled to get the final selection.

    \sa currentColor, {Dialog::}{accepted()}
*/
QColor QQuickLabsPlatformColorDialog::color() const
{
    return m_color;
}

void QQuickLabsPlatformColorDialog::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;
    setCurrentColor(color);
    emit colorChanged();
}

/*!
    \qmlproperty color Qt.labs.platform::ColorDialog::currentColor

    This property holds the currently selected color in the dialog.

    Unlike the \l color property, the \c currentColor property is updated
    while the user is selecting colors in the dialog, even before the final
    selection has been made.

    \sa color
*/
QColor QQuickLabsPlatformColorDialog::currentColor() const
{
    if (QPlatformColorDialogHelper *colorDialog = qobject_cast<QPlatformColorDialogHelper *>(handle()))
        return colorDialog->currentColor();
    return m_currentColor;
}

void QQuickLabsPlatformColorDialog::setCurrentColor(const QColor &color)
{
    if (QPlatformColorDialogHelper *colorDialog = qobject_cast<QPlatformColorDialogHelper *>(handle()))
        colorDialog->setCurrentColor(color);
    m_currentColor = color;
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
QColorDialogOptions::ColorDialogOptions QQuickLabsPlatformColorDialog::options() const
{
    return m_options->options();
}

void QQuickLabsPlatformColorDialog::setOptions(QColorDialogOptions::ColorDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

bool QQuickLabsPlatformColorDialog::useNativeDialog() const
{
    return QQuickLabsPlatformDialog::useNativeDialog()
            && !m_options->testOption(QColorDialogOptions::DontUseNativeDialog);
}

void QQuickLabsPlatformColorDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformColorDialogHelper *colorDialog = qobject_cast<QPlatformColorDialogHelper *>(dialog)) {
        connect(colorDialog, &QPlatformColorDialogHelper::currentColorChanged, this, &QQuickLabsPlatformColorDialog::currentColorChanged);
        colorDialog->setOptions(m_options);
        colorDialog->setCurrentColor(m_currentColor);
    }
}

void QQuickLabsPlatformColorDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformColorDialogHelper *colorDialog = qobject_cast<QPlatformColorDialogHelper *>(dialog))
        colorDialog->setOptions(m_options);
}

void QQuickLabsPlatformColorDialog::accept()
{
    setColor(currentColor());
    QQuickLabsPlatformDialog::accept();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformcolordialog_p.cpp"

#endif // QT_DEPRECATED_SINCE(6, 9)
