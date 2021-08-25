/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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

#include "qquickfiledialog_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlfile.h>

#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcFileDialog, "qt.quick.dialogs.filedialog")

/*!
    \qmltype FileDialog
    \inherits Dialog
//!     \instantiates QQuickFileDialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.2
    \brief A file dialog.

    The FileDialog type provides a QML API for file dialogs.

    \image qtquickdialogs-filedialog-gtk.png

    To show a file dialog, construct an instance of FileDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentFile
    or \l currentFiles properties can be used to determine the currently
    selected file(s) in the dialog. The \l selectedFile and \l selectedFiles
    properties are updated only after the final selection has been made by
    accepting the dialog.

    \code
    MenuItem {
        text: "Open..."
        onTriggered: fileDialog.open()
    }

    FileDialog {
        id: fileDialog
        currentFile: document.source
        folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
    }

    MyDocument {
        id: document
        source: fileDialog.file
    }
    \endcode

    \section2 Availability

    A native platform file dialog is currently available on the following platforms:

    \list
    \li iOS
    \li Linux (when running with the GTK+ platform theme)
    \li macOS
    \li Windows
    \endlist

    \include includes/fallback.qdocinc

    //! \sa FolderDialog, StandardPaths
*/

Q_DECLARE_LOGGING_CATEGORY(lcDialogs)

QQuickFileDialog::QQuickFileDialog(QObject *parent)
    : QQuickAbstractDialog(QPlatformTheme::FileDialog, parent),
      m_fileMode(OpenFile),
      m_options(QFileDialogOptions::create()),
      m_selectedNameFilter(nullptr)
{
    m_options->setFileMode(QFileDialogOptions::ExistingFile);
    m_options->setAcceptMode(QFileDialogOptions::AcceptOpen);
}

/*!
    \qmlproperty enumeration QtQuick.Dialogs::FileDialog::fileMode

    This property holds the mode of the dialog.

    Available values:
    \value FileDialog.OpenFile The dialog is used to select an existing file (default).
    \value FileDialog.OpenFiles The dialog is used to select multiple existing files.
    \value FileDialog.SaveFile The dialog is used to select any file. The file does not have to exist.
*/
QQuickFileDialog::FileMode QQuickFileDialog::fileMode() const
{
    return m_fileMode;
}

void QQuickFileDialog::setFileMode(FileMode mode)
{
    qCDebug(lcFileDialog) << "setFileMode called with" << mode;
    if (mode == m_fileMode)
        return;

    switch (mode) {
    case OpenFile:
        m_options->setFileMode(QFileDialogOptions::ExistingFile);
        m_options->setAcceptMode(QFileDialogOptions::AcceptOpen);
        break;
    case OpenFiles:
        m_options->setFileMode(QFileDialogOptions::ExistingFiles);
        m_options->setAcceptMode(QFileDialogOptions::AcceptOpen);
        break;
    case SaveFile:
        m_options->setFileMode(QFileDialogOptions::AnyFile);
        m_options->setAcceptMode(QFileDialogOptions::AcceptSave);
        break;
    default:
        break;
    }

    m_fileMode = mode;
    emit fileModeChanged();
}

/*!
    \qmlproperty url QtQuick.Dialogs::FileDialog::selectedFile
    \readonly

    This property holds the final accepted file.

    If there are multiple selected files, this property refers to the first
    file.

    Unlike the \l currentFile property, the \c selectedFile property is not
    updated while the user is selecting files in the dialog, but only after the
    final selection has been made. That is, when the user has clicked
    \uicontrol OK to accept a file. Alternatively, the
    \l {Dialog::}{accepted()} signal can be handled to get the final selection.

    \sa selectedFiles, currentFile, {Dialog::}{accepted()}, currentFolder
*/
QUrl QQuickFileDialog::selectedFile() const
{
    return addDefaultSuffix(m_selectedFiles.value(0));
}

/*!
    \qmlproperty list<url> QtQuick.Dialogs::FileDialog::selectedFiles

    This property holds the final accepted files.

    Unlike the \l currentFiles property, the \c selectedFiles property is not
    updated while the user is selecting files in the dialog, but only after the
    final selection has been made. That is, when the user has clicked
    \uicontrol OK to accept files. Alternatively, the \l {Dialog::}{accepted()}
    signal can be handled to get the final selection.

    \sa currentFiles, {Dialog::}{accepted()}, currentFolder
*/
QList<QUrl> QQuickFileDialog::selectedFiles() const
{
    return addDefaultSuffixes(m_selectedFiles);
}

void QQuickFileDialog::setSelectedFiles(const QList<QUrl> &selectedFiles)
{
    if (m_selectedFiles == selectedFiles)
        return;

    bool firstChanged = m_selectedFiles.value(0) != selectedFiles.value(0);
    m_selectedFiles = selectedFiles;
    if (firstChanged)
        emit selectedFileChanged();
    emit selectedFilesChanged();
}

/*!
    \qmlproperty url QtQuick.Dialogs::FileDialog::currentFile

    This property holds the currently selected file in the dialog.

    Unlike the \l selectedFile property, the \c currentFile property is updated
    while the user is selecting files in the dialog, even before the final
    selection has been made.

    \sa selectedFile, currentFiles, currentFolder
*/
QUrl QQuickFileDialog::currentFile() const
{
    return currentFiles().value(0);
}

void QQuickFileDialog::setCurrentFile(const QUrl &file)
{
    setCurrentFiles(QList<QUrl>() << file);
}

/*!
    \qmlproperty list<url> QtQuick.Dialogs::FileDialog::currentFiles

    This property holds the currently selected files in the dialog.

    Unlike the \l selectedFiles property, the \c currentFiles property is
    updated while the user is selecting files in the dialog, even before the
    final selection has been made.

    \sa selectedFiles, currentFile, currentFolder
*/
QList<QUrl> QQuickFileDialog::currentFiles() const
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        return fileDialog->selectedFiles();
    return m_options->initiallySelectedFiles();
}

void QQuickFileDialog::setCurrentFiles(const QList<QUrl> &currentFiles)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle())) {
        for (const QUrl &file : currentFiles)
            fileDialog->selectFile(file);
    }
    m_options->setInitiallySelectedFiles(currentFiles);
}

/*!
    \qmlproperty url QtQuick.Dialogs::FileDialog::currentFolder

    This property holds the folder where files are selected. It can be set to
    control the initial directory that is shown when the dialog is opened.

\omit
    For selecting a folder, use FolderDialog instead.

    \sa FolderDialog
\endomit
*/
QUrl QQuickFileDialog::currentFolder() const
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        return fileDialog->directory();

    // If we're not using a native file dialog and the folder is invalid,
    // return the current directory.
    if (!m_options->initialDirectory().isValid())
        return QUrl::fromLocalFile(QDir::currentPath());

    return m_options->initialDirectory();
}

void QQuickFileDialog::setCurrentFolder(const QUrl &currentFolder)
{
    qCDebug(lcFileDialog) << "setCurrentFolder called with" << currentFolder;
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        fileDialog->setDirectory(currentFolder);
    m_options->setInitialDirectory(currentFolder);
}

/*!
    \qmlproperty flags QtQuick.Dialogs::FileDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value FileDialog.DontResolveSymlinks Don't resolve symlinks in the file dialog. By default symlinks are resolved.
    \value FileDialog.DontConfirmOverwrite Don't ask for confirmation if an existing file is selected. By default confirmation is requested.
    \value FileDialog.ReadOnly Indicates that the dialog doesn't allow creating directories.
    \value FileDialog.HideNameFilterDetails Indicates if the file name filter details are hidden or not.
*/
QFileDialogOptions::FileDialogOptions QQuickFileDialog::options() const
{
    return m_options->options();
}

void QQuickFileDialog::setOptions(QFileDialogOptions::FileDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickFileDialog::resetOptions()
{
    setOptions({});
}

/*!
    \qmlproperty list<string> QtQuick.Dialogs::FileDialog::nameFilters

    This property holds the filters that restrict the types of files that
    can be selected.

    \code
    FileDialog {
        nameFilters: ["Text files (*.txt)", "HTML files (*.html *.htm)"]
    }
    \endcode

    Different platforms may restrict the files that can be selected in
    different ways. For example, macOS will disable file entries that do not
    match the filters, whereas Windows will hide them.

    \note \b{*.*} is not a portable filter, because the historical assumption
    that the file extension determines the file type is not consistent on every
    operating system. It is possible to have a file with no dot in its name (for
    example, \c Makefile). In a native Windows file dialog, \b{*.*} will match
    such files, while in other types of file dialogs it may not. So it is better
    to use \b{*} if you mean to select any file.

    \sa selectedNameFilter
*/
QStringList QQuickFileDialog::nameFilters() const
{
    return m_options->nameFilters();
}

void QQuickFileDialog::setNameFilters(const QStringList &filters)
{
    qCDebug(lcFileDialog).nospace() << "setNameFilters called with " << filters
        << " (old filters were: " << m_options->nameFilters() << ")";
    if (filters == m_options->nameFilters())
        return;

    m_options->setNameFilters(filters);
    if (m_selectedNameFilter) {
        int index = m_selectedNameFilter->index();
        if (index < 0 || index >= filters.count())
            index = 0;
        m_selectedNameFilter->update(filters.value(index));
    }
    emit nameFiltersChanged();
}

void QQuickFileDialog::resetNameFilters()
{
    setNameFilters(QStringList());
}

/*!
    \qmlproperty int QtQuick.Dialogs::FileDialog::selectedNameFilter.index
    \qmlproperty string QtQuick.Dialogs::FileDialog::selectedNameFilter.name
    \qmlproperty list<string> QtQuick.Dialogs::FileDialog::selectedNameFilter.extensions
    \qmlproperty list<string> QtQuick.Dialogs::FileDialog::selectedNameFilter.globs

    These properties hold the currently selected name filter.

    \table
    \header
        \li Name
        \li Description
    \row
        \li \b index : int
        \li This property determines which \l {nameFilters}{name filter} is selected.
            The specified filter is selected when the dialog is opened. The value is
            updated when the user selects another filter.
    \row
        \li [read-only] \b name : string
        \li This property holds the name of the selected filter. In the
            example below, the name of the first filter is \c {"Text files"}
            and the second is \c {"HTML files"}.
    \row
        \li [read-only] \b extensions : list<string>
        \li This property holds the list of extensions of the selected filter.
            In the example below, the list of extensions of the first filter is
            \c {["txt"]} and the second is \c {["html", "htm"]}.
    \row
        \li [read-only] \b globs : list<string>
        \li This property holds the list of globs of the selected filter.
            In the example below, the list of globs of the first filter is
            \c {["*.txt"]} and the second is \c {["*.html", "*.htm"]}.

            This property is useful in conjunction with \l {FolderListModel}'s
            \l {FolderListModel::}{nameFilters} property, for example.
    \endtable

    \code
    FileDialog {
        id: fileDialog
        selectedNameFilter.index: 1
        nameFilters: ["Text files (*.txt)", "HTML files (*.html *.htm)"]
    }

    MyDocument {
        id: document
        fileType: fileDialog.selectedNameFilter.extensions[0]
    }
    \endcode

    \sa nameFilters
*/
QQuickFileNameFilter *QQuickFileDialog::selectedNameFilter() const
{
    if (!m_selectedNameFilter) {
        QQuickFileDialog *that = const_cast<QQuickFileDialog *>(this);
        m_selectedNameFilter = new QQuickFileNameFilter(that);
        m_selectedNameFilter->setOptions(m_options);
    }
    return m_selectedNameFilter;
}

/*!
    \qmlproperty string QtQuick.Dialogs::FileDialog::defaultSuffix

    This property holds a suffix that is added to selected files that have
    no suffix specified. The suffix is typically used to indicate the file
    type (e.g. "txt" indicates a text file).

    If the first character is a dot ('.'), it is removed.
*/
QString QQuickFileDialog::defaultSuffix() const
{
    return m_options->defaultSuffix();
}

void QQuickFileDialog::setDefaultSuffix(const QString &suffix)
{
    if (suffix == m_options->defaultSuffix())
        return;

    m_options->setDefaultSuffix(suffix);
    emit defaultSuffixChanged();
}

void QQuickFileDialog::resetDefaultSuffix()
{
    setDefaultSuffix(QString());
}

/*!
    \qmlproperty string QtQuick.Dialogs::FileDialog::acceptLabel

    This property holds the label text shown on the button that accepts the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Open or \uicontrol Save depending on which
    \l fileMode the dialog is used in.

    The default value is an empty string.

    \sa rejectLabel
*/
QString QQuickFileDialog::acceptLabel() const
{
    return m_options->labelText(QFileDialogOptions::Accept);
}

void QQuickFileDialog::setAcceptLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Accept))
        return;

    m_options->setLabelText(QFileDialogOptions::Accept, label);
    emit acceptLabelChanged();
}

void QQuickFileDialog::resetAcceptLabel()
{
    setAcceptLabel(QString());
}

/*!
    \qmlproperty string QtQuick.Dialogs::FileDialog::rejectLabel

    This property holds the label text shown on the button that rejects the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Cancel.

    The default value is an empty string.

    \sa acceptLabel
*/
QString QQuickFileDialog::rejectLabel() const
{
    return m_options->labelText(QFileDialogOptions::Reject);
}

void QQuickFileDialog::setRejectLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Reject))
        return;

    m_options->setLabelText(QFileDialogOptions::Reject, label);
    emit rejectLabelChanged();
}

void QQuickFileDialog::resetRejectLabel()
{
    setRejectLabel(QString());
}

bool QQuickFileDialog::useNativeDialog() const
{
    if (!QQuickAbstractDialog::useNativeDialog())
        return false;

    if (m_options->testOption(QFileDialogOptions::DontUseNativeDialog)) {
        qCDebug(lcDialogs) << "  - the FileDialog was told not to use a native dialog; not using native dialog";
        return false;
    }

    return true;
}

void QQuickFileDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        connect(fileDialog, &QPlatformFileDialogHelper::currentChanged, this, &QQuickFileDialog::currentFileChanged);
        connect(fileDialog, &QPlatformFileDialogHelper::currentChanged, this, &QQuickFileDialog::currentFilesChanged);
        connect(fileDialog, &QPlatformFileDialogHelper::directoryEntered, this, &QQuickFileDialog::currentFolderChanged);
        fileDialog->setOptions(m_options);
    }
}

void QQuickFileDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        // Ensure that a name filter is always selected.
        int index = selectedNameFilter()->index();
        if (index == -1)
            index = 0;
        const QString filter = m_options->nameFilters().value(index);
        m_options->setInitiallySelectedNameFilter(filter);

        fileDialog->setOptions(m_options); // setOptions only assigns a member and isn't virtual

        connect(fileDialog, &QPlatformFileDialogHelper::filterSelected, m_selectedNameFilter, &QQuickFileNameFilter::update);
        fileDialog->selectNameFilter(filter);

        const QUrl initialDir = m_options->initialDirectory();
        // If it's not valid, or it's a file and not a directory, we shouldn't set it.
        if (m_firstShow && initialDir.isValid() && QDir(QQmlFile::urlToLocalFileOrQrc(initialDir)).exists())
            fileDialog->setDirectory(m_options->initialDirectory());
    }
    if (m_firstShow)
        m_firstShow = false;
}

void QQuickFileDialog::onHide(QPlatformDialogHelper *dialog)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        if (m_selectedNameFilter)
            disconnect(fileDialog, &QPlatformFileDialogHelper::filterSelected, m_selectedNameFilter, &QQuickFileNameFilter::update);
    }
}

void QQuickFileDialog::accept()
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle())) {
        // Take the currently selected files and make them the final set of files.
        setSelectedFiles(fileDialog->selectedFiles());
    }
    QQuickAbstractDialog::accept();
}

QUrl QQuickFileDialog::addDefaultSuffix(const QUrl &file) const
{
    QUrl url = file;
    const QString path = url.path();
    const QString suffix = m_options->defaultSuffix();
    if (!suffix.isEmpty() && !path.endsWith(QLatin1Char('/')) && path.lastIndexOf(QLatin1Char('.')) == -1)
        url.setPath(path + QLatin1Char('.') + suffix);
    return url;
}

QList<QUrl> QQuickFileDialog::addDefaultSuffixes(const QList<QUrl> &files) const
{
    QList<QUrl> urls;
    urls.reserve(files.size());
    for (const QUrl &file : files)
        urls += addDefaultSuffix(file);
    return urls;
}

QT_END_NAMESPACE
