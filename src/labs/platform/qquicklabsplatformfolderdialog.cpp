// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformfolderdialog_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype FolderDialog
    \inherits Dialog
//!     \instantiates QQuickLabsPlatformFolderDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native folder dialog.

    The FolderDialog type provides a QML API for native platform folder dialogs.

    \image qtlabsplatform-folderdialog-gtk.png

    To show a folder dialog, construct an instance of FolderDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentFolder
    property can be used to determine the currently selected folder in the
    dialog. The \l folder property is updated only after the final selection
    has been made by accepting the dialog.

    \code
    MenuItem {
        text: "Open..."
        onTriggered: folderDialog.open()
    }

    FolderDialog {
        id: folderDialog
        currentFolder: viewer.folder
        folder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
    }

    MyViewer {
        id: viewer
        folder: folderDialog.folder
    }
    \endcode

    \section2 Availability

    A native platform folder dialog is currently available on the following platforms:

    \list
    \li Android
    \li iOS
    \li Linux (when running with the GTK+ platform theme)
    \li macOS
    \li Windows
    \endlist

    \input includes/widgets.qdocinc 1

    \labs

    \sa FileDialog, StandardPaths
*/

QQuickLabsPlatformFolderDialog::QQuickLabsPlatformFolderDialog(QObject *parent)
    : QQuickLabsPlatformDialog(QPlatformTheme::FileDialog, parent),
      m_options(QFileDialogOptions::create())
{
    m_options->setFileMode(QFileDialogOptions::Directory);
    m_options->setAcceptMode(QFileDialogOptions::AcceptOpen);
}

/*!
    \qmlproperty url Qt.labs.platform::FolderDialog::folder

    This property holds the final accepted folder.

    Unlike the \l currentFolder property, the \c folder property is not updated
    while the user is selecting folders in the dialog, but only after the final
    selection has been made. That is, when the user has clicked \uicontrol OK
    to accept a folder. Alternatively, the \l {Dialog::}{accepted()} signal
    can be handled to get the final selection.

    \sa currentFolder, {Dialog::}{accepted()}
*/
QUrl QQuickLabsPlatformFolderDialog::folder() const
{
    return m_folder;
}

void QQuickLabsPlatformFolderDialog::setFolder(const QUrl &folder)
{
    if (m_folder == folder)
        return;

    m_folder = folder;
    emit folderChanged();
}

/*!
    \qmlproperty url Qt.labs.platform::FolderDialog::currentFolder

    This property holds the currently selected folder in the dialog.

    Unlike the \l folder property, the \c currentFolder property is updated
    while the user is selecting folders in the dialog, even before the final
    selection has been made.

    \sa folder
*/
QUrl QQuickLabsPlatformFolderDialog::currentFolder() const
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle())) {
        const QList<QUrl> selectedFiles = fileDialog->selectedFiles();
        if (!selectedFiles.isEmpty())
            return selectedFiles.first();
        return fileDialog->directory();
    }
    return m_options->initialDirectory();
}

void QQuickLabsPlatformFolderDialog::setCurrentFolder(const QUrl &folder)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        fileDialog->setDirectory(folder);
    m_options->setInitialDirectory(folder);
}

/*!
    \qmlproperty flags Qt.labs.platform::FolderDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value FolderDialog.ShowDirsOnly Only show directories in the folder dialog. By default both folders and directories are shown.
    \value FolderDialog.DontResolveSymlinks Don't resolve symlinks in the folder dialog. By default symlinks are resolved.
    \value FolderDialog.ReadOnly Indicates that the dialog doesn't allow creating directories.
*/
QFileDialogOptions::FileDialogOptions QQuickLabsPlatformFolderDialog::options() const
{
    return m_options->options();
}

void QQuickLabsPlatformFolderDialog::setOptions(QFileDialogOptions::FileDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickLabsPlatformFolderDialog::resetOptions()
{
    setOptions({});
}

/*!
    \qmlproperty string Qt.labs.platform::FolderDialog::acceptLabel

    This property holds the label text shown on the button that accepts the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Open.

    The default value is an empty string.

    \sa rejectLabel
*/
QString QQuickLabsPlatformFolderDialog::acceptLabel() const
{
    return m_options->labelText(QFileDialogOptions::Accept);
}

void QQuickLabsPlatformFolderDialog::setAcceptLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Accept))
        return;

    m_options->setLabelText(QFileDialogOptions::Accept, label);
    emit acceptLabelChanged();
}

void QQuickLabsPlatformFolderDialog::resetAcceptLabel()
{
    setAcceptLabel(QString());
}

/*!
    \qmlproperty string Qt.labs.platform::FolderDialog::rejectLabel

    This property holds the label text shown on the button that rejects the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Cancel.

    The default value is an empty string.

    \sa acceptLabel
*/
QString QQuickLabsPlatformFolderDialog::rejectLabel() const
{
    return m_options->labelText(QFileDialogOptions::Reject);
}

void QQuickLabsPlatformFolderDialog::setRejectLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Reject))
        return;

    m_options->setLabelText(QFileDialogOptions::Reject, label);
    emit rejectLabelChanged();
}

void QQuickLabsPlatformFolderDialog::resetRejectLabel()
{
    setRejectLabel(QString());
}

bool QQuickLabsPlatformFolderDialog::useNativeDialog() const
{
    return QQuickLabsPlatformDialog::useNativeDialog()
            && !m_options->testOption(QFileDialogOptions::DontUseNativeDialog);
}

void QQuickLabsPlatformFolderDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        connect(fileDialog, &QPlatformFileDialogHelper::currentChanged, this, &QQuickLabsPlatformFolderDialog::currentFolderChanged);
        fileDialog->setOptions(m_options);
    }
}

void QQuickLabsPlatformFolderDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog))
        fileDialog->setOptions(m_options);
}

void QQuickLabsPlatformFolderDialog::accept()
{
    setFolder(currentFolder());
    QQuickLabsPlatformDialog::accept();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformfolderdialog_p.cpp"
