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
    connect(m_dialog, &QQuickFileDialogImpl::currentFileChanged, this, &QQuickPlatformFileDialog::currentChanged);
    connect(m_dialog, &QQuickFileDialogImpl::currentFolderChanged, this, &QQuickPlatformFileDialog::directoryEntered);
    connect(m_dialog, &QQuickFileDialogImpl::filterSelected, this, &QQuickPlatformFileDialog::filterSelected);

    // We would do this in QQuickFileDialogImpl, but we need to ensure that folderChanged()
    // is connected to directoryEntered() before setting it to ensure that the QQuickFileDialog is notified.
    if (m_dialog->currentFolder().isEmpty())
        m_dialog->setCurrentFolder(QUrl::fromLocalFile(QDir().absolutePath()));
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

    m_dialog->setSelectedFile(file);
}

QList<QUrl> QQuickPlatformFileDialog::selectedFiles() const
{
    // TODO: support for multiple selected files
    return { m_dialog->currentFile() };
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
