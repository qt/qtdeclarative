// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformfiledialog_p.h"

#if QT_DEPRECATED_SINCE(6, 9)

#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \qmltype FileDialog
    \inherits Dialog
//!     \nativetype QQuickLabsPlatformFileDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \deprecated [6.9] Use QtQuick.Dialogs::FileDialog instead.
    \brief A native file dialog.

    The FileDialog type provides a QML API for native platform file dialogs.

    \image {qtlabsplatform-filedialog-gtk.png} {A native file dialog}

    To show a file dialog, construct an instance of FileDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentFile
    or \l currentFiles properties can be used to determine the currently
    selected file(s) in the dialog. The \l file and \l files properties
    are updated only after the final selection has been made by accepting
    the dialog.

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
    \li Android
    \li iOS
    \li Linux (when running with the GTK+ platform theme)
    \li macOS
    \li Windows
    \endlist

    \input includes/widgets.qdocinc 1

    \labs

    \sa QtQuick.Dialogs::FileDialog, FolderDialog, StandardPaths
*/

QQuickLabsPlatformFileDialog::QQuickLabsPlatformFileDialog(QObject *parent)
    : QQuickLabsPlatformDialog(QPlatformTheme::FileDialog, parent),
      m_fileMode(OpenFile),
      m_options(QFileDialogOptions::create()),
      m_selectedNameFilter(nullptr)
{
    m_options->setFileMode(QFileDialogOptions::ExistingFile);
    m_options->setAcceptMode(QFileDialogOptions::AcceptOpen);
}

/*!
    \qmlproperty enumeration Qt.labs.platform::FileDialog::fileMode

    This property holds the mode of the dialog.

    Available values:
    \value FileDialog.OpenFile The dialog is used to select an existing file (default).
    \value FileDialog.OpenFiles The dialog is used to select multiple existing files.
    \value FileDialog.SaveFile The dialog is used to select any file. The file does not have to exist.
*/
QQuickLabsPlatformFileDialog::FileMode QQuickLabsPlatformFileDialog::fileMode() const
{
    return m_fileMode;
}

void QQuickLabsPlatformFileDialog::setFileMode(FileMode mode)
{
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
    \qmlproperty url Qt.labs.platform::FileDialog::file

    This property holds the final accepted file.

    Unlike the \l currentFile property, the \c file property is not updated
    while the user is selecting files in the dialog, but only after the final
    selection has been made. That is, when the user has clicked \uicontrol OK
    to accept a file. Alternatively, the \l {Dialog::}{accepted()} signal
    can be handled to get the final selection.

    \sa currentFile, {Dialog::}{accepted()}
*/
QUrl QQuickLabsPlatformFileDialog::file() const
{
    return addDefaultSuffix(m_files.value(0));
}

void QQuickLabsPlatformFileDialog::setFile(const QUrl &file)
{
    setFiles(QList<QUrl>() << file);
}

/*!
    \qmlproperty list<url> Qt.labs.platform::FileDialog::files

    This property holds the final accepted files.

    Unlike the \l currentFiles property, the \c files property is not updated
    while the user is selecting files in the dialog, but only after the final
    selection has been made. That is, when the user has clicked \uicontrol OK
    to accept files. Alternatively, the \l {Dialog::}{accepted()} signal
    can be handled to get the final selection.

    \sa currentFiles, {Dialog::}{accepted()}
*/
QList<QUrl> QQuickLabsPlatformFileDialog::files() const
{
    return addDefaultSuffixes(m_files);
}

void QQuickLabsPlatformFileDialog::setFiles(const QList<QUrl> &files)
{
    if (m_files == files)
        return;

    bool firstChanged = m_files.value(0) != files.value(0);
    m_files = files;
    if (firstChanged)
        emit fileChanged();
    emit filesChanged();
}

/*!
    \qmlproperty url Qt.labs.platform::FileDialog::currentFile

    This property holds the currently selected file in the dialog.

    Unlike the \l file property, the \c currentFile property is updated
    while the user is selecting files in the dialog, even before the final
    selection has been made.

    \sa file, currentFiles
*/
QUrl QQuickLabsPlatformFileDialog::currentFile() const
{
    return currentFiles().value(0);
}

void QQuickLabsPlatformFileDialog::setCurrentFile(const QUrl &file)
{
    setCurrentFiles(QList<QUrl>() << file);
}

/*!
    \qmlproperty list<url> Qt.labs.platform::FileDialog::currentFiles

    This property holds the currently selected files in the dialog.

    Unlike the \l files property, the \c currentFiles property is updated
    while the user is selecting files in the dialog, even before the final
    selection has been made.

    \sa files, currentFile
*/
QList<QUrl> QQuickLabsPlatformFileDialog::currentFiles() const
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        return fileDialog->selectedFiles();
    return m_options->initiallySelectedFiles();
}

void QQuickLabsPlatformFileDialog::setCurrentFiles(const QList<QUrl> &files)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle())) {
        for (const QUrl &file : files)
            fileDialog->selectFile(file);
    }
    m_options->setInitiallySelectedFiles(files);
}

/*!
    \qmlproperty url Qt.labs.platform::FileDialog::folder

    This property holds the folder where files are selected.
    For selecting a folder, use FolderDialog instead.

    \sa FolderDialog
*/
QUrl QQuickLabsPlatformFileDialog::folder() const
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        return fileDialog->directory();
    return m_options->initialDirectory();
}

void QQuickLabsPlatformFileDialog::setFolder(const QUrl &folder)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        fileDialog->setDirectory(folder);
    m_options->setInitialDirectory(folder);
}

/*!
    \qmlproperty flags Qt.labs.platform::FileDialog::options

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
QFileDialogOptions::FileDialogOptions QQuickLabsPlatformFileDialog::options() const
{
    return m_options->options();
}

void QQuickLabsPlatformFileDialog::setOptions(QFileDialogOptions::FileDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickLabsPlatformFileDialog::resetOptions()
{
    setOptions({});
}

/*!
    \qmlproperty list<string> Qt.labs.platform::FileDialog::nameFilters

    This property holds the filters that restrict the types of files that
    can be selected.

    \code
    FileDialog {
        nameFilters: ["Text files (*.txt)", "HTML files (*.html *.htm)"]
    }
    \endcode

    \note \b{*.*} is not a portable filter, because the historical assumption
    that the file extension determines the file type is not consistent on every
    operating system. It is possible to have a file with no dot in its name (for
    example, \c Makefile). In a native Windows file dialog, \b{*.*} will match
    such files, while in other types of file dialogs it may not. So it is better
    to use \b{*} if you mean to select any file.

    \sa selectedNameFilter
*/
QStringList QQuickLabsPlatformFileDialog::nameFilters() const
{
    return m_options->nameFilters();
}

void QQuickLabsPlatformFileDialog::setNameFilters(const QStringList &filters)
{
    if (filters == m_options->nameFilters())
        return;

    m_options->setNameFilters(filters);
    if (m_selectedNameFilter) {
        int index = m_selectedNameFilter->index();
        if (index < 0 || index >= filters.size())
            index = 0;
        m_selectedNameFilter->update(filters.value(index));
    }
    emit nameFiltersChanged();
}

void QQuickLabsPlatformFileDialog::resetNameFilters()
{
    setNameFilters(QStringList());
}

/*!
    \qmlproperty int Qt.labs.platform::FileDialog::selectedNameFilter.index
    \qmlproperty string Qt.labs.platform::FileDialog::selectedNameFilter.name
    \qmlproperty list<string> Qt.labs.platform::FileDialog::selectedNameFilter.extensions

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
QQuickLabsPlatformFileNameFilter *QQuickLabsPlatformFileDialog::selectedNameFilter() const
{
    if (!m_selectedNameFilter) {
        QQuickLabsPlatformFileDialog *that = const_cast<QQuickLabsPlatformFileDialog *>(this);
        m_selectedNameFilter = new QQuickLabsPlatformFileNameFilter(that);
        m_selectedNameFilter->setOptions(m_options);
    }
    return m_selectedNameFilter;
}

/*!
    \qmlproperty string Qt.labs.platform::FileDialog::defaultSuffix

    This property holds a suffix that is added to selected files that have
    no suffix specified. The suffix is typically used to indicate the file
    type (e.g. "txt" indicates a text file).

    If the first character is a dot ('.'), it is removed.
*/
QString QQuickLabsPlatformFileDialog::defaultSuffix() const
{
    return m_options->defaultSuffix();
}

void QQuickLabsPlatformFileDialog::setDefaultSuffix(const QString &suffix)
{
    if (suffix == m_options->defaultSuffix())
        return;

    m_options->setDefaultSuffix(suffix);
    emit defaultSuffixChanged();
}

void QQuickLabsPlatformFileDialog::resetDefaultSuffix()
{
    setDefaultSuffix(QString());
}

/*!
    \qmlproperty string Qt.labs.platform::FileDialog::acceptLabel

    This property holds the label text shown on the button that accepts the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Open or \uicontrol Save depending on which
    \l fileMode the dialog is used in.

    The default value is an empty string.

    \sa rejectLabel
*/
QString QQuickLabsPlatformFileDialog::acceptLabel() const
{
    return m_options->labelText(QFileDialogOptions::Accept);
}

void QQuickLabsPlatformFileDialog::setAcceptLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Accept))
        return;

    m_options->setLabelText(QFileDialogOptions::Accept, label);
    emit acceptLabelChanged();
}

void QQuickLabsPlatformFileDialog::resetAcceptLabel()
{
    setAcceptLabel(QString());
}

/*!
    \qmlproperty string Qt.labs.platform::FileDialog::rejectLabel

    This property holds the label text shown on the button that rejects the dialog.

    When set to an empty string, the default label of the underlying platform is used.
    The default label is typically \uicontrol Cancel.

    The default value is an empty string.

    \sa acceptLabel
*/
QString QQuickLabsPlatformFileDialog::rejectLabel() const
{
    return m_options->labelText(QFileDialogOptions::Reject);
}

void QQuickLabsPlatformFileDialog::setRejectLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Reject))
        return;

    m_options->setLabelText(QFileDialogOptions::Reject, label);
    emit rejectLabelChanged();
}

void QQuickLabsPlatformFileDialog::resetRejectLabel()
{
    setRejectLabel(QString());
}

bool QQuickLabsPlatformFileDialog::useNativeDialog() const
{
    return QQuickLabsPlatformDialog::useNativeDialog()
            && !m_options->testOption(QFileDialogOptions::DontUseNativeDialog);
}

void QQuickLabsPlatformFileDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        // TODO: emit currentFileChanged only when the first entry in currentFiles changes
        connect(fileDialog, &QPlatformFileDialogHelper::currentChanged, this, &QQuickLabsPlatformFileDialog::currentFileChanged);
        connect(fileDialog, &QPlatformFileDialogHelper::currentChanged, this, &QQuickLabsPlatformFileDialog::currentFilesChanged);
        connect(fileDialog, &QPlatformFileDialogHelper::directoryEntered, this, &QQuickLabsPlatformFileDialog::folderChanged);
        fileDialog->setOptions(m_options);
    }
}

void QQuickLabsPlatformFileDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        fileDialog->setOptions(m_options); // setOptions only assigns a member and isn't virtual
        if (m_firstShow && m_options->initialDirectory().isValid())
            fileDialog->setDirectory(m_options->initialDirectory());
        if (m_selectedNameFilter) {
            const int index = m_selectedNameFilter->index();
            const QString filter = m_options->nameFilters().value(index);
            m_options->setInitiallySelectedNameFilter(filter);
            fileDialog->selectNameFilter(filter);
            connect(fileDialog, &QPlatformFileDialogHelper::filterSelected, m_selectedNameFilter, &QQuickLabsPlatformFileNameFilter::update);
        }
    }
    if (m_firstShow)
        m_firstShow = false;
}

void QQuickLabsPlatformFileDialog::onHide(QPlatformDialogHelper *dialog)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        if (m_selectedNameFilter)
            disconnect(fileDialog, &QPlatformFileDialogHelper::filterSelected, m_selectedNameFilter, &QQuickLabsPlatformFileNameFilter::update);
    }
}

void QQuickLabsPlatformFileDialog::accept()
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        setFiles(fileDialog->selectedFiles());
    QQuickLabsPlatformDialog::accept();
}

QUrl QQuickLabsPlatformFileDialog::addDefaultSuffix(const QUrl &file) const
{
    QUrl url = file;
    const QString path = url.path();
    const QString suffix = m_options->defaultSuffix();
    // Urls with "content" scheme do not require suffixes. Such schemes are
    // used on Android.
    const bool isContentScheme = url.scheme() == u"content"_s;
    if (!isContentScheme && !suffix.isEmpty() && !path.endsWith(QLatin1Char('/'))
        && path.lastIndexOf(QLatin1Char('.')) == -1) {
        url.setPath(path + QLatin1Char('.') + suffix);
    }
    return url;
}

QList<QUrl> QQuickLabsPlatformFileDialog::addDefaultSuffixes(const QList<QUrl> &files) const
{
    QList<QUrl> urls;
    urls.reserve(files.size());
    for (const QUrl &file : files)
        urls += addDefaultSuffix(file);
    return urls;
}

QQuickLabsPlatformFileNameFilter::QQuickLabsPlatformFileNameFilter(QObject *parent)
    : QObject(parent), m_index(-1)
{
}

int QQuickLabsPlatformFileNameFilter::index() const
{
    return m_index;
}

void QQuickLabsPlatformFileNameFilter::setIndex(int index)
{
    if (m_index == index)
        return;

    m_index = index;
    emit indexChanged(index);
}

QString QQuickLabsPlatformFileNameFilter::name() const
{
    return m_name;
}

QStringList QQuickLabsPlatformFileNameFilter::extensions() const
{
    return m_extensions;
}

QSharedPointer<QFileDialogOptions> QQuickLabsPlatformFileNameFilter::options() const
{
    return m_options;
}

void QQuickLabsPlatformFileNameFilter::setOptions(const QSharedPointer<QFileDialogOptions> &options)
{
    m_options = options;
}

static QString extractName(const QString &filter)
{
    return filter.left(filter.indexOf(QLatin1Char('(')) - 1);
}

static QString extractExtension(QStringView filter)
{
    return filter.mid(filter.indexOf(QLatin1Char('.')) + 1).toString();
}

static QStringList extractExtensions(QStringView filter)
{
    QStringList extensions;
    const int from = filter.indexOf(QLatin1Char('('));
    const int to = filter.lastIndexOf(QLatin1Char(')')) - 1;
    if (from >= 0 && from < to) {
        const QStringView ref = filter.mid(from + 1, to - from);
        const QList<QStringView> exts = ref.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QStringView &ref : exts)
            extensions += extractExtension(ref);
    }

    return extensions;
}

void QQuickLabsPlatformFileNameFilter::update(const QString &filter)
{
    const QStringList filters = nameFilters();

    const int oldIndex = m_index;
    const QString oldName = m_name;
    const QStringList oldExtensions = m_extensions;

    m_index = filters.indexOf(filter);
    m_name = extractName(filter);
    m_extensions = extractExtensions(filter);

    if (oldIndex != m_index)
        emit indexChanged(m_index);
    if (oldName != m_name)
        emit nameChanged(m_name);
    if (oldExtensions != m_extensions)
        emit extensionsChanged(m_extensions);
}

QStringList QQuickLabsPlatformFileNameFilter::nameFilters() const
{
    return m_options ? m_options->nameFilters() : QStringList();
}

QString QQuickLabsPlatformFileNameFilter::nameFilter(int index) const
{
    return m_options ? m_options->nameFilters().value(index) : QString();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformfiledialog_p.cpp"

#endif // QT_DEPRECATED_SINCE(6, 9)
