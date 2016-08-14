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

#include "qquickplatformfiledialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>

#ifdef QT_WIDGETS_LIB
#include "widgets/qwidgetplatformfiledialog_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype FileDialog
    \inherits Dialog
    \instantiates QQuickPlatformFileDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native file dialog.

    The FileDialog type provides a QML API for native platform file dialogs.

    \image qtlabsplatform-filedialog-gtk.png

    To show a file dialog, construct an instance of FileDialog, set the
    desired properties, and call \l {Dialog::}{open()}. FileDialog emits
    the \l fileSelected() and \l filesSelected() signals when the user has
    selected file(s).

    \code
    MenuItem {
        text: "Open..."
        onTriggered: fileDialog.open()
    }

    FileDialog {
        id: fileDialog
        currentFile: document.source
        onFileSelected: document.source = file
    }
    \endcode

    \section2 Availability

    A native platform file dialog is currently available on the following platforms:

    \list
    \li iOS
    \li Linux (when running with the GTK+ platform theme)
    \li macOS
    \li Windows
    \li WinRT
    \endlist

    \input includes/widgets.qdocinc 1

    \labs

    \sa FolderDialog
*/

/*!
    \qmlsignal void Qt.labs.platform::FileDialog::fileSelected(url file)

    This signal is emitted when a \a file has been selected.

    \sa filesSelected(), currentFile
*/

/*!
    \qmlsignal void Qt.labs.platform::FileDialog::filesSelected(list<url> files)

    This signal is emitted when multiple \a files have been selected.

    \sa fileSelected(), currentFile
*/

Q_DECLARE_LOGGING_CATEGORY(qtLabsPlatformDialogs)

QQuickPlatformFileDialog::QQuickPlatformFileDialog(QObject *parent)
    : QQuickPlatformDialog(parent), m_fileMode(OpenFile), m_options(QFileDialogOptions::create())
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
QQuickPlatformFileDialog::FileMode QQuickPlatformFileDialog::fileMode() const
{
    return m_fileMode;
}

void QQuickPlatformFileDialog::setFileMode(FileMode mode)
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
    \qmlproperty url Qt.labs.platform::FileDialog::currentFile

    This property holds the currently selected file in the dialog.

    \sa fileSelected(), filesSelected()
*/
QUrl QQuickPlatformFileDialog::currentFile() const
{
    if (m_current.isEmpty()) {
        if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
            m_current = fileDialog->selectedFiles().value(0);
    }
    return addDefaultSuffix(m_current);
}

void QQuickPlatformFileDialog::setCurrentFile(const QUrl &file)
{
    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(handle()))
        fileDialog->selectFile(file);
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
QFileDialogOptions::FileDialogOptions QQuickPlatformFileDialog::options() const
{
    return m_options->options();
}

void QQuickPlatformFileDialog::setOptions(QFileDialogOptions::FileDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickPlatformFileDialog::resetOptions()
{
    setOptions(0);
}

/*!
    \qmlproperty list<string> Qt.labs.platform::FileDialog::nameFilters

    This property holds the filters that restrict the types of files that
    can be selected.

    \note \b{*.*} is not a portable filter, because the historical assumption
    that the file extension determines the file type is not consistent on every
    operating system. It is possible to have a file with no dot in its name (for
    example, \c Makefile). In a native Windows file dialog, \b{*.*} will match
    such files, while in other types of file dialogs it may not. So it is better
    to use \b{*} if you mean to select any file.
*/
QStringList QQuickPlatformFileDialog::nameFilters() const
{
    return m_options->nameFilters();
}

void QQuickPlatformFileDialog::setNameFilters(const QStringList &filters)
{
    if (filters == m_options->nameFilters())
        return;

    m_options->setNameFilters(filters);
    emit nameFiltersChanged();
}

void QQuickPlatformFileDialog::resetNameFilters()
{
    setNameFilters(QStringList());
}

/*!
    \qmlproperty string Qt.labs.platform::FileDialog::defaultSuffix

    This property holds a suffix that is added to selected files that have
    no suffix specified. The suffix is typically used to indicate the file
    type (e.g. "txt" indicates a text file).

    If the first character is a dot ('.'), it is removed.
*/
QString QQuickPlatformFileDialog::defaultSuffix() const
{
    return m_options->defaultSuffix();
}

void QQuickPlatformFileDialog::setDefaultSuffix(const QString &suffix)
{
    if (suffix == m_options->defaultSuffix())
        return;

    m_options->setDefaultSuffix(suffix);
    emit defaultSuffixChanged();
}

void QQuickPlatformFileDialog::resetDefaultSuffix()
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
QString QQuickPlatformFileDialog::acceptLabel() const
{
    return m_options->labelText(QFileDialogOptions::Accept);
}

void QQuickPlatformFileDialog::setAcceptLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Accept))
        return;

    m_options->setLabelText(QFileDialogOptions::Accept, label);
    emit acceptLabelChanged();
}

void QQuickPlatformFileDialog::resetAcceptLabel()
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
QString QQuickPlatformFileDialog::rejectLabel() const
{
    return m_options->labelText(QFileDialogOptions::Reject);
}

void QQuickPlatformFileDialog::setRejectLabel(const QString &label)
{
    if (label == m_options->labelText(QFileDialogOptions::Reject))
        return;

    m_options->setLabelText(QFileDialogOptions::Reject, label);
    emit rejectLabelChanged();
}

void QQuickPlatformFileDialog::resetRejectLabel()
{
    setRejectLabel(QString());
}

QPlatformDialogHelper *QQuickPlatformFileDialog::createHelper()
{
    QPlatformDialogHelper *dialog = QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(QPlatformTheme::FileDialog);
#ifdef QT_WIDGETS_LIB
    if (!dialog)
        dialog = new QWidgetPlatformFileDialog(this);
#endif
    qCDebug(qtLabsPlatformDialogs) << "FileDialog:" << dialog;

    if (QPlatformFileDialogHelper *fileDialog = qobject_cast<QPlatformFileDialogHelper *>(dialog)) {
        connect(fileDialog, &QPlatformFileDialogHelper::fileSelected, [this](const QUrl &file) {
            emit fileSelected(addDefaultSuffix(file));
        });
        connect(fileDialog, &QPlatformFileDialogHelper::filesSelected, [this](const QList<QUrl> &files) {
            emit filesSelected(addDefaultSuffixes(files));
        });
        connect(fileDialog, &QPlatformFileDialogHelper::currentChanged, [this](const QUrl &url) {
            if (m_current == url)
                return;
            m_current = url;
            emit currentFileChanged();
        });
        fileDialog->setOptions(m_options);
    }
    return dialog;
}

void QQuickPlatformFileDialog::applyOptions()
{
    m_options->setWindowTitle(title());
}

QUrl QQuickPlatformFileDialog::addDefaultSuffix(const QUrl &file) const
{
    QUrl url = file;
    const QString path = url.path();
    const QString suffix = m_options->defaultSuffix();
    if (!suffix.isEmpty() && !path.endsWith(QLatin1Char('/')) && path.lastIndexOf(QLatin1Char('.')) == -1)
        url.setPath(path + QLatin1Char('.') + suffix);
    return url;
}

QList<QUrl> QQuickPlatformFileDialog::addDefaultSuffixes(const QList<QUrl> &files) const
{
    QList<QUrl> urls;
    urls.reserve(files.size());
    for (const QUrl &file : files)
        urls += addDefaultSuffix(file);
    return urls;
}

QT_END_NAMESPACE
