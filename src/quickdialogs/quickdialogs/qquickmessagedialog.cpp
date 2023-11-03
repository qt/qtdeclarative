// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmessagedialog_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype MessageDialog
    \inherits Dialog
//!     \instantiates QQuickMessageDialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.3
    \brief A message dialog.

    The MessageDialog type provides a QML API for message dialogs.

    \image qtquickdialogs-messagedialog-android.png

    A message dialog is used to inform the user, or ask the user a question.
    A message dialog displays a primary \l text to alert the user to a situation,
    an \l {informativeText}{informative text} to further explain the alert or to
    ask the user a question, and an optional \l {detailedText}{detailed text} to
    provide even more data if the user requests it. A message box can also display
    a configurable set of \l buttons for accepting a user response.

    To show a message dialog, construct an instance of MessageDialog, set the
    desired properties, and call \l {Dialog::}{open()}.

    \code
    MessageDialog {
        buttons: MessageDialog.Ok
        text: "The document has been modified."
    }
    \endcode

    The user must click the \uicontrol OK button to dismiss the message dialog.

    A more elaborate approach than just alerting the user to an event is to
    also ask the user what to do about it. Store the question in the
    \l {informativeText}{informative text} property, and specify the \l buttons
    property to the set of buttons you want as the set of user responses. The
    buttons are specified by combining values using the bitwise OR operator.

    \code
    MessageDialog {
        text: "The document has been modified."
        informativeText: "Do you want to save your changes?"
        buttons: MessageDialog.Ok | MessageDialog.Cancel

        onAccepted: document.save()
    }
    \endcode

    \image qtquickdialogs-messagedialog-informative-android.png


    \section2 Availability

    A native platform message dialog is currently available on the following platforms:

    \list
    \li Android
    \li iOS
    \li macOS
    \endlist

    \include includes/fallback.qdocinc
*/

/*!
    \qmlsignal QtQuick.Dialogs::MessageDialog::buttonClicked(QPlatformDialogHelper::StandardButton button, QPlatformDialogHelper::ButtonRole role)

    This signal is emitted when a \a button with the specified \a role is
    clicked.

    By giving this signal a handler, you can respond to any custom button being pressed.
    The \a button argument tells which button was clicked, while the \a role argument tells the functional role of that button.

    \code
    MessageDialog {
        id: dialog
        text: qsTr("The document has been modified.")
        informativeText: qsTr("Do you want to save your changes?")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onButtonClicked: function (button, role) {
            switch (button) {
            case MessageDialog.Ok:
                document.save()
                break;
            }
        }
    }
    \endcode

    \sa buttons
*/

QQuickMessageDialog::QQuickMessageDialog(QObject *parent)
    : QQuickAbstractDialog(QQuickDialogType::MessageDialog, parent),
      m_options(QMessageDialogOptions::create())
{
}

/*!
    \qmlproperty string QtQuick.Dialogs::MessageDialog::text

    This property holds the text to be displayed on the message dialog.

    \sa informativeText, detailedText
*/
QString QQuickMessageDialog::text() const
{
    return m_options->text();
}

void QQuickMessageDialog::setText(const QString &text)
{
    if (m_options->text() == text)
        return;

    m_options->setText(text);

    emit textChanged();
}

/*!
    \qmlproperty string QtQuick.Dialogs::MessageDialog::informativeText

    This property holds the informative text that provides a fuller description for the message.

    Informative text can be used to expand upon the \l text to give more information to the user.

    \sa text, detailedText
*/
QString QQuickMessageDialog::informativeText() const
{
    return m_options->informativeText();
}

void QQuickMessageDialog::setInformativeText(const QString &text)
{
    if (m_options->informativeText() == text)
        return;

    m_options->setInformativeText(text);

    emit informativeTextChanged();
}

/*!
    \qmlproperty string QtQuick.Dialogs::MessageDialog::detailedText

    This property holds the text to be displayed in the details area.

    \sa text, informativeText
*/
QString QQuickMessageDialog::detailedText() const
{
    return m_options->detailedText();
}

void QQuickMessageDialog::setDetailedText(const QString &text)
{
    if (m_options->detailedText() == text)
        return;

    m_options->setDetailedText(text);

    emit detailedTextChanged();
}

/*!
    \qmlproperty flags QtQuick.Dialogs::MessageDialog::buttons

    This property holds a combination of buttons that are used by the message dialog.
    The default value is \c MessageDialog.NoButton.

    Possible flags:
    \value MessageDialog.Ok An "OK" button defined with the \c AcceptRole.
    \value MessageDialog.Open An "Open" button defined with the \c AcceptRole.
    \value MessageDialog.Save A "Save" button defined with the \c AcceptRole.
    \value MessageDialog.Cancel A "Cancel" button defined with the \c RejectRole.
    \value MessageDialog.Close A "Close" button defined with the \c RejectRole.
    \value MessageDialog.Discard A "Discard" or "Don't Save" button, depending on the platform, defined with the \c DestructiveRole.
    \value MessageDialog.Apply An "Apply" button defined with the \c ApplyRole.
    \value MessageDialog.Reset A "Reset" button defined with the \c ResetRole.
    \value MessageDialog.RestoreDefaults A "Restore Defaults" button defined with the \c ResetRole.
    \value MessageDialog.Help A "Help" button defined with the \c HelpRole.
    \value MessageDialog.SaveAll A "Save All" button defined with the \c AcceptRole.
    \value MessageDialog.Yes A "Yes" button defined with the \c YesRole.
    \value MessageDialog.YesToAll A "Yes to All" button defined with the \c YesRole.
    \value MessageDialog.No A "No" button defined with the \c NoRole.
    \value MessageDialog.NoToAll A "No to All" button defined with the \c NoRole.
    \value MessageDialog.Abort An "Abort" button defined with the \c RejectRole.
    \value MessageDialog.Retry A "Retry" button defined with the \c AcceptRole.
    \value MessageDialog.Ignore An "Ignore" button defined with the \c AcceptRole.
    \value MessageDialog.NoButton The dialog has no buttons.

    \sa buttonClicked()
*/

QPlatformDialogHelper::StandardButtons QQuickMessageDialog::buttons() const
{
    return m_options->standardButtons();
}

void QQuickMessageDialog::setButtons(QPlatformDialogHelper::StandardButtons buttons)
{
    if (m_options->standardButtons() == buttons)
        return;

    m_options->setStandardButtons(buttons);

    emit buttonsChanged();
}

void QQuickMessageDialog::handleClick(QPlatformDialogHelper::StandardButton button,
                                      QPlatformDialogHelper::ButtonRole role)
{
    emit buttonClicked(button, role);
    close();
}

void QQuickMessageDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformMessageDialogHelper *messageDialog =
                qobject_cast<QPlatformMessageDialogHelper *>(dialog)) {
        connect(messageDialog, &QPlatformMessageDialogHelper::clicked, this,
                &QQuickMessageDialog::handleClick);
        messageDialog->setOptions(m_options);
    }
}

void QQuickMessageDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());

    if (QPlatformMessageDialogHelper *messageDialog =
                qobject_cast<QPlatformMessageDialogHelper *>(dialog))
        messageDialog->setOptions(m_options); // setOptions only assigns a member and isn't virtual
}

QT_END_NAMESPACE

#include "moc_qquickmessagedialog_p.cpp"
