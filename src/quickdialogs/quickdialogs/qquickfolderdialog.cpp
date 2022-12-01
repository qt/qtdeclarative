// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfolderdialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlfile.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcDialogs)

/*!
    \qmltype FolderDialog
    \inherits Dialog
//!     \instantiates QQuickFolderDialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.3
    \brief A native folder dialog.

    The FolderDialog type provides a QML API for native platform folder dialogs.

    \image qtquickdialogs-folderdialog-gtk.png

    To show a folder dialog, construct an instance of FolderDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentFolder
    property can be used to determine the folder that is currently being
    displayed in the dialog. The \l selectedFolder property can be used to
    determine the last folder that was selected in the dialog.

    \code
    MenuItem {
        text: qsTr("Open...")
        onTriggered: folderDialog.open()
    }

    FolderDialog {
        id: folderDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        selectedFolder: viewer.folder
    }

    MyViewer {
        id: viewer
        folder: folderDialog.selectedFolder
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

    \include includes/fallback.qdocinc

    \sa FileDialog, {QtCore::}{StandardPaths}
*/

QQuickFolderDialog::QQuickFolderDialog(QObject *parent)
    : QQuickAbstractDialog(QQuickDialogType::FolderDialog, parent),
      m_options(QFileDialogOptions::create())
{
    m_options->setFileMode(QFileDialogOptions::Directory);
    m_options->setAcceptMode(QFileDialogOptions::AcceptOpen);
    m_options->setInitialDirectory(QUrl::fromLocalFile(QDir::currentPath()));
}

/*!
    \qmlproperty url QtQuick.Dialogs::FolderDialog::currentFolder

    This property holds the folder that is currently being displayed in the dialog.

    \sa selectedFolder
*/
QUrl QQuickFolderDialog::currentFolder() const
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        return fileDialog->directory();
    return m_options->initialDirectory();
}

void QQuickFolderDialog::setCurrentFolder(const QUrl &folder)
{
    if (folder == m_options->initialDirectory())
        return;

    m_options->setInitialDirectory(folder);
    emit currentFolderChanged();
}

/*!
    \qmlproperty url QtQuick.Dialogs::FolderDialog::selectedFolder

    This property holds the last folder that was selected in the dialog.

    The value of this property is updated each time the user selects a folder
    in the dialog, and when the dialog is accepted. Alternatively, the
    \l {Dialog::}{accepted()} signal can be handled to get the final selection.

    \sa currentFolder, {Dialog::}{accepted()}
*/
QUrl QQuickFolderDialog::selectedFolder() const
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle())) {
        const QList<QUrl> selectedFiles = fileDialog->selectedFiles();
        if (!selectedFiles.isEmpty())
            return selectedFiles.first();
    }
    return QUrl();
}

void QQuickFolderDialog::setSelectedFolder(const QUrl &folder)
{
    if (folder == selectedFolder())
        return;

    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle())) {
        fileDialog->selectFile(folder);
        emit selectedFolderChanged();
    }
}

/*!
    \qmlproperty flags QtQuick.Dialogs::FolderDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value FolderDialog.DontResolveSymlinks Don't resolve symlinks in the folder dialog. By default symlinks are resolved.
    \value FolderDialog.ReadOnly Indicates that the dialog doesn't allow creating directories.
    \value FolderDialog.DontUseNativeDialog Forces the dialog to use a non-native quick implementation.
*/
QFileDialogOptions::FileDialogOptions QQuickFolderDialog::options() const
{
    return m_options->options();
}

void QQuickFolderDialog::setOptions(QFileDialogOptions::FileDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickFolderDialog::resetOptions()
{
    setOptions({});
}

/*!
    \qmlproperty string QtQuick.Dialogs::FolderDialog::acceptLabel

    This property holds the label text shown on the button that accepts the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Open.

    The default value is an empty string.

    \sa rejectLabel
*/
QString QQuickFolderDialog::acceptLabel() const
{
    return m_options->labelText(QFileDialogOptions::Accept);
}

void QQuickFolderDialog::setAcceptLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Accept))
        return;

    m_options->setLabelText(QFileDialogOptions::Accept, label);
    emit acceptLabelChanged();
}

void QQuickFolderDialog::resetAcceptLabel()
{
    setAcceptLabel(QString());
}

/*!
    \qmlproperty string QtQuick.Dialogs::FolderDialog::rejectLabel

    This property holds the label text shown on the button that rejects the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Cancel.

    The default value is an empty string.

    \sa acceptLabel
*/
QString QQuickFolderDialog::rejectLabel() const
{
    return m_options->labelText(QFileDialogOptions::Reject);
}

void QQuickFolderDialog::setRejectLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Reject))
        return;

    m_options->setLabelText(QFileDialogOptions::Reject, label);
    emit rejectLabelChanged();
}

void QQuickFolderDialog::resetRejectLabel()
{
    setRejectLabel(QString());
}

bool QQuickFolderDialog::useNativeDialog() const
{
    if (!QQuickAbstractDialog::useNativeDialog())
        return false;

    if (m_options->testOption(QFileDialogOptions::DontUseNativeDialog)) {
        qCDebug(lcDialogs) << "  - the FolderDialog was told not to use a native dialog; not using native dialog";
        return false;
    }

    return true;
}

void QQuickFolderDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        connect(fileDialog, &QPlatformFileDialogHelper::directoryEntered, this, &QQuickFolderDialog::currentFolderChanged);
        connect(fileDialog, &QPlatformFileDialogHelper::currentChanged, this, &QQuickFolderDialog::selectedFolderChanged);
        fileDialog->setOptions(m_options);
    }
}

void QQuickFolderDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        fileDialog->setOptions(m_options);

        const QUrl initialDir = m_options->initialDirectory();
        // If it's not valid, we shouldn't set it.
        if (m_firstShow && initialDir.isValid() && QDir(QQmlFile::urlToLocalFileOrQrc(initialDir)).exists())
            fileDialog->setDirectory(m_options->initialDirectory());
    }
    QQuickAbstractDialog::onShow(dialog);
}

QT_END_NAMESPACE

#include "moc_qquickfolderdialog_p.cpp"
