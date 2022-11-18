// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickplatformfiledialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qwindow.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupanchors_p.h>

#include "qquickfiledialogimpl_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuickPlatformFileDialog, "qt.quick.dialogs.quickplatformfiledialog")

/*!
    \class QQuickPlatformFileDialog
    \internal

    An interface that QQuickFileDialog can use to access the non-native Qt Quick FileDialog.

    Both this and the native implementations are created in QQuickAbstractDialog::create().
*/
QQuickPlatformFileDialog::QQuickPlatformFileDialog(QObject *parent)
{
    qCDebug(lcQuickPlatformFileDialog) << "creating non-native Qt Quick FileDialog with parent" << parent;

    // Set a parent so that we get deleted if we can't be shown for whatever reason.
    // Our eventual parent should be the window, though.
    setParent(parent);

    auto qmlContext = ::qmlContext(parent);
    if (!qmlContext) {
        qmlWarning(parent) << "No QQmlContext for QQuickPlatformFileDialog; can't create non-native FileDialog implementation";
        return;
    }

    const auto dialogQmlUrl = QUrl(QStringLiteral("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/qml/FileDialog.qml"));
    QQmlComponent fileDialogComponent(qmlContext->engine(), dialogQmlUrl, parent);
    if (!fileDialogComponent.isReady()) {
        qmlWarning(parent) << "Failed to load non-native FileDialog implementation:\n" << fileDialogComponent.errorString();
        return;
    }
    m_dialog = qobject_cast<QQuickFileDialogImpl*>(fileDialogComponent.create());
    if (!m_dialog) {
        qmlWarning(parent) << "Failed to create an instance of the non-native FileDialog:\n" << fileDialogComponent.errorString();
        return;
    }
    // Give it a parent until it's parented to the window in show().
    m_dialog->setParent(this);

    connect(m_dialog, &QQuickDialog::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog, &QQuickDialog::rejected, this, &QPlatformDialogHelper::reject);

    connect(m_dialog, &QQuickFileDialogImpl::fileSelected, this, &QQuickPlatformFileDialog::fileSelected);
    // TODO: add support for multiple file selection (QTBUG-92585)
//    connect(m_dialog, &QQuickFileDialogImpl::filesSelected, [this](const QList<QString> &files) {
//        QList<QUrl> urls;
//        urls.reserve(files.count());
//        for (const QString &file : files)
//            urls += QUrl::fromLocalFile(file);
//        emit filesSelected(urls);
//    });
    connect(m_dialog, &QQuickFileDialogImpl::selectedFileChanged, this, &QQuickPlatformFileDialog::currentChanged);
    connect(m_dialog, &QQuickFileDialogImpl::currentFolderChanged, this, &QQuickPlatformFileDialog::directoryEntered);
    connect(m_dialog, &QQuickFileDialogImpl::filterSelected, this, &QQuickPlatformFileDialog::filterSelected);
}

bool QQuickPlatformFileDialog::isValid() const
{
    return m_dialog;
}

bool QQuickPlatformFileDialog::defaultNameFilterDisables() const
{
    return false;
}

void QQuickPlatformFileDialog::setDirectory(const QUrl &directory)
{
    if (!m_dialog)
        return;

    m_dialog->setCurrentFolder(directory);
}

QUrl QQuickPlatformFileDialog::directory() const
{
    if (!m_dialog)
        return {};

    return m_dialog->currentFolder();
}

void QQuickPlatformFileDialog::selectFile(const QUrl &file)
{
    if (!m_dialog)
        return;

    if (m_dialog->isVisible()) {
        qWarning() << "Cannot set an initial selectedFile while FileDialog is open";
        return;
    }

    // Since we're only called once each time the FileDialog is shown,
    // we call setInitialCurrentFolderAndSelectedFile here, which will ensure that
    // the first currentIndex change (to 0, as a result of the ListView's model changing
    // as a result of the FolderListModel directory change) is effectively
    // ignored and the correct index for the initial selectedFile is maintained.
    m_dialog->setInitialCurrentFolderAndSelectedFile(file);
}

// TODO: support for multiple selected files
QList<QUrl> QQuickPlatformFileDialog::selectedFiles() const
{
    if (m_dialog->selectedFile().isEmpty())
        return {};

    return { m_dialog->selectedFile() };
}

void QQuickPlatformFileDialog::setFilter()
{
}

void QQuickPlatformFileDialog::selectNameFilter(const QString &filter)
{
    // There is a bit of a problem with order here - QQuickFileDialog::onShow()
    // is called before our show(), but it needs to set the selected filter
    // (which we can't do in our show() because we don't know about QQuickFileDialog).
    // So, delay setting the filter until we're shown. This shouldn't be an issue
    // in practice, since it doesn't make sense for the filter to programmatically
    // change while the dialog is visible.
    m_pendingNameFilter = filter;
}

QString QQuickPlatformFileDialog::selectedNameFilter() const
{
    return m_dialog->selectedNameFilter()->name();
}

void QQuickPlatformFileDialog::exec()
{
    qCWarning(lcQuickPlatformFileDialog) << "exec() is not supported for the Qt Quick FileDialog fallback";
}

/*!
    \internal

    This is called after QQuickFileDialog::onShow().
    Both are called in QQuickAbstractDialog::open().
*/
bool QQuickPlatformFileDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    qCDebug(lcQuickPlatformFileDialog) << "show called with flags" << flags <<
        "modality" << modality << "parent" << parent;
    if (!m_dialog)
        return false;

    if (!parent)
        return false;

    auto quickWindow = qobject_cast<QQuickWindow*>(parent);
    if (!quickWindow) {
        qmlInfo(this->parent()) << "Parent window (" << parent << ") of non-native dialog is not a QQuickWindow";
        return false;
    }
    m_dialog->setParent(parent);
    m_dialog->resetParentItem();

    auto popupPrivate = QQuickPopupPrivate::get(m_dialog);
    popupPrivate->getAnchors()->setCenterIn(m_dialog->parentItem());

    QSharedPointer<QFileDialogOptions> options = QPlatformFileDialogHelper::options();
    m_dialog->setTitle(options->windowTitle());
    m_dialog->setOptions(options);
    m_dialog->selectNameFilter(m_pendingNameFilter);
    m_pendingNameFilter.clear();
    m_dialog->setAcceptLabel(options->isLabelExplicitlySet(QFileDialogOptions::Accept)
        ? options->labelText(QFileDialogOptions::Accept) : QString());
    m_dialog->setRejectLabel(options->isLabelExplicitlySet(QFileDialogOptions::Reject)
        ? options->labelText(QFileDialogOptions::Reject) : QString());

    if (options->initiallySelectedFiles().isEmpty()) {
        if (m_dialog->currentFolder().isEmpty()) {
            // The user didn't set an initial selectedFile nor currentFolder, so we'll set it to the working directory.
            qCDebug(lcQuickPlatformFileDialog) << "- calling setCurrentFolder(QDir()) on quick dialog" << parent;
            m_dialog->setCurrentFolder(QUrl::fromLocalFile(QDir().absolutePath()));
        }
    }

    m_dialog->open();
    return true;
}

void QQuickPlatformFileDialog::hide()
{
    if (!m_dialog)
        return;

    m_dialog->close();
}

QQuickFileDialogImpl *QQuickPlatformFileDialog::dialog() const
{
    return m_dialog;
}

QT_END_NAMESPACE

#include "moc_qquickplatformfiledialog_p.cpp"
