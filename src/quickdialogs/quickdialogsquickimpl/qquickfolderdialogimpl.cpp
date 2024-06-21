// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfolderdialogimpl_p.h"
#include "qquickfolderdialogimpl_p_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>

#include "qquickfiledialogdelegate_p.h"
#include "qquickfolderbreadcrumbbar_p.h"

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcFolderDialogCurrentFolder, "qt.quick.dialogs.quickfolderdialogimpl.currentFolder")
Q_STATIC_LOGGING_CATEGORY(lcFolderDialogSelectedFolder, "qt.quick.dialogs.quickfolderdialogimpl.selectedFolder")
Q_STATIC_LOGGING_CATEGORY(lcFolderDialogOptions, "qt.quick.dialogs.quickfolderdialogimpl.options")

QQuickFolderDialogImplPrivate::QQuickFolderDialogImplPrivate()
{
}

void QQuickFolderDialogImplPrivate::updateEnabled()
{
    Q_Q(QQuickFolderDialogImpl);
    if (!buttonBox)
        return;

    QQuickFolderDialogImplAttached *attached = attachedOrWarn();
    if (!attached)
        return;

    auto openButton = buttonBox->standardButton(QPlatformDialogHelper::Open);
    if (!openButton) {
        qmlWarning(q).nospace() << "Can't update Open button's enabled state because it wasn't found";
        return;
    }

    openButton->setEnabled(!selectedFolder.isEmpty() && attached->breadcrumbBar()
        && !attached->breadcrumbBar()->textField()->isVisible());
}

/*!
    \internal

    Ensures that a folder is always selected after a change in \c currentFolder.

    \a oldFolderPath is the previous value of \c currentFolder.
*/
void QQuickFolderDialogImplPrivate::updateSelectedFolder(const QString &oldFolderPath)
{
    Q_Q(QQuickFolderDialogImpl);
    QQuickFolderDialogImplAttached *attached = attachedOrWarn();
    if (!attached || !attached->folderDialogListView())
        return;

    QString newSelectedFolderPath;
    int newSelectedFolderIndex = 0;
    const QString newFolderPath = QQmlFile::urlToLocalFileOrQrc(currentFolder);
    if (!oldFolderPath.isEmpty() && !newFolderPath.isEmpty()) {
        // If the user went up a directory (or several), we should set
        // selectedFolder to be the directory that we were in (or
        // its closest ancestor that is a child of the new directory).
        // E.g. if oldFolderPath is /foo/bar/baz/abc/xyz, and newFolderPath is /foo/bar,
        // then we want to set selectedFolder to be /foo/bar/baz.
        const int indexOfFolder = oldFolderPath.indexOf(newFolderPath);
        if (indexOfFolder != -1) {
            // [folder]
            // [   oldFolderPath  ]
            // /foo/bar/baz/abc/xyz
            //         [rel...Paths]
            QStringList relativePaths = oldFolderPath.mid(indexOfFolder + newFolderPath.size()).split(QLatin1Char('/'), Qt::SkipEmptyParts);
            newSelectedFolderPath = newFolderPath + QLatin1Char('/') + relativePaths.first();

            // Now find the index of that directory so that we can set the ListView's currentIndex to it.
            const QDir newFolderDir(newFolderPath);
            // Just to be safe...
            if (!newFolderDir.exists()) {
                qmlWarning(q) << "Directory" << newSelectedFolderPath << "doesn't exist; can't get a file entry list for it";
                return;
            }

            const QFileInfoList dirs = newFolderDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::DirsFirst);
            const QFileInfo newSelectedFileInfo(newSelectedFolderPath);
            // The directory can contain files, but since we put dirs first, that should never affect the indices.
            newSelectedFolderIndex = dirs.indexOf(newSelectedFileInfo);
        }
    }

    if (newSelectedFolderPath.isEmpty()) {
        // When entering into a directory that isn't a parent of the old one, the first
        // file delegate should be selected.
        // TODO: is there a cheaper way to do this? QDirIterator doesn't support sorting,
        // so we can't use that. QQuickFolderListModel uses threads to fetch its data,
        // so should be considered asynchronous. We might be able to use it, but it would
        // complicate the code even more...
        QDir newFolderDir(newFolderPath);
        if (newFolderDir.exists()) {
            const QFileInfoList files = newFolderDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::DirsFirst);
            if (!files.isEmpty())
                newSelectedFolderPath = files.first().absoluteFilePath();
        }
    }

    const bool folderSelected = !newSelectedFolderPath.isEmpty();
    q->setSelectedFolder(folderSelected ? QUrl::fromLocalFile(newSelectedFolderPath) : QUrl());
    {
        // Set the appropriate currentIndex for the selected folder. We block signals from ListView
        // because we don't want folderDialogListViewCurrentIndexChanged to be called, as the file
        // it gets from the delegate will not be up-to-date (but most importantly because we already
        // just set the selected folder).
        QSignalBlocker blocker(attached->folderDialogListView());
        attached->folderDialogListView()->setCurrentIndex(folderSelected ? newSelectedFolderIndex : -1);
    }
    if (folderSelected) {
        if (QQuickItem *currentItem = attached->folderDialogListView()->currentItem())
            currentItem->forceActiveFocus();
    }
}

void QQuickFolderDialogImplPrivate::handleAccept()
{
    // Let handleClick take care of calling accept().
}

void QQuickFolderDialogImplPrivate::handleClick(QQuickAbstractButton *button)
{
    Q_Q(QQuickFolderDialogImpl);
    if (buttonRole(button) == QPlatformDialogHelper::AcceptRole && selectedFolder.isValid()) {
        q->setSelectedFolder(selectedFolder);
        q->accept();
    }
}

/*!
    \class QQuickFolderDialogImpl
    \internal

    An interface that QQuickFolderDialog can use to access the non-native Qt Quick FolderDialog.

    Both this and the native implementations are created in QQuickAbstractDialog::create().
*/

QQuickFolderDialogImpl::QQuickFolderDialogImpl(QObject *parent)
    : QQuickDialog(*(new QQuickFolderDialogImplPrivate), parent)
{
}

QQuickFolderDialogImplAttached *QQuickFolderDialogImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickFolderDialogImplAttached(object);
}

QUrl QQuickFolderDialogImpl::currentFolder() const
{
    Q_D(const QQuickFolderDialogImpl);
    return d->currentFolder;
}

void QQuickFolderDialogImpl::setCurrentFolder(const QUrl &currentFolder)
{
    qCDebug(lcFolderDialogCurrentFolder) << "setCurrentFolder called with" << currentFolder;
    Q_D(QQuickFolderDialogImpl);
    if (currentFolder == d->currentFolder)
        return;

    const QString oldFolderPath = QQmlFile::urlToLocalFileOrQrc(d->currentFolder);

    d->currentFolder = currentFolder;
    d->updateSelectedFolder(oldFolderPath);
    emit currentFolderChanged(d->currentFolder);
}

QUrl QQuickFolderDialogImpl::selectedFolder() const
{
    Q_D(const QQuickFolderDialogImpl);
    return d->selectedFolder;
}

void QQuickFolderDialogImpl::setSelectedFolder(const QUrl &selectedFolder)
{
    Q_D(QQuickFolderDialogImpl);
    qCDebug(lcFolderDialogSelectedFolder).nospace() << "setSelectedFolder called with selectedFolder "
        << selectedFolder << " (d->selectedFolder is " << d->selectedFolder << ")";
    if (selectedFolder == d->selectedFolder)
        return;

    d->selectedFolder = selectedFolder;
    d->updateEnabled();
    emit selectedFolderChanged(selectedFolder);
}

QSharedPointer<QFileDialogOptions> QQuickFolderDialogImpl::options() const
{
    Q_D(const QQuickFolderDialogImpl);
    return d->options;
}

void QQuickFolderDialogImpl::setOptions(const QSharedPointer<QFileDialogOptions> &options)
{
    qCDebug(lcFolderDialogOptions).nospace() << "setOptions called with:"
        << " acceptMode=" << options->acceptMode()
        << " fileMode=" << options->fileMode()
        << " initialDirectory=" << options->initialDirectory();

    Q_D(QQuickFolderDialogImpl);
    d->options = options;
}

/*!
    \internal

    These allow QQuickPlatformFileDialog::show() to set custom labels on the
    dialog buttons without having to know about/go through QQuickFolderDialogImplAttached
    and QQuickDialogButtonBox.
*/
void QQuickFolderDialogImpl::setAcceptLabel(const QString &label)
{
    Q_D(QQuickFolderDialogImpl);
    d->acceptLabel = label;
    QQuickFolderDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    auto acceptButton = d->buttonBox->standardButton(QPlatformDialogHelper::Open);
    if (!acceptButton) {
        qmlWarning(this).nospace() << "Can't set accept label to " << label
            << "; failed to find Open button in DialogButtonBox of " << this;
        return;
    }

    acceptButton->setText(!label.isEmpty()
        ? label : QQuickDialogButtonBoxPrivate::buttonText(QPlatformDialogHelper::Open));
}

void QQuickFolderDialogImpl::setRejectLabel(const QString &label)
{
    Q_D(QQuickFolderDialogImpl);
    d->rejectLabel = label;
    if (!d->buttonBox)
        return;

    auto rejectButton = d->buttonBox->standardButton(QPlatformDialogHelper::Cancel);
    if (!rejectButton) {
        qmlWarning(this).nospace() << "Can't set reject label to " << label
            << "; failed to find Open button in DialogButtonBox of " << this;
        return;
    }

    rejectButton->setText(!label.isEmpty()
        ? label : QQuickDialogButtonBoxPrivate::buttonText(QPlatformDialogHelper::Cancel));
}

void QQuickFolderDialogImpl::componentComplete()
{
    Q_D(QQuickFolderDialogImpl);
    QQuickDialog::componentComplete();

    // Find the right-most button and set its key navigation so that
    // tab moves focus to the breadcrumb bar's up button. I tried
    // doing this via KeyNavigation on the DialogButtonBox in QML,
    // but it didn't work (probably because it's not the right item).
    QQuickFolderDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    Q_ASSERT(d->buttonBox);
    const int buttonCount = d->buttonBox->count();
    if (buttonCount == 0)
        return;

    QQuickAbstractButton *rightMostButton = qobject_cast<QQuickAbstractButton *>(
        d->buttonBox->itemAt(buttonCount - 1));
    if (!rightMostButton) {
        qmlWarning(this) << "Can't find right-most button in DialogButtonBox";
        return;
    }

    auto keyNavigationAttached = QQuickKeyNavigationAttached::qmlAttachedProperties(rightMostButton);
    if (!keyNavigationAttached) {
        qmlWarning(this) << "Can't create attached KeyNavigation object on" << QDebug::toString(rightMostButton);
        return;
    }

    keyNavigationAttached->setTab(attached->breadcrumbBar()->upButton());
}

void QQuickFolderDialogImpl::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_D(QQuickFolderDialogImpl);
    QQuickDialog::itemChange(change, data);

    if (change != QQuickItem::ItemVisibleHasChanged || !isComponentComplete() || !data.boolValue)
        return;

    QQuickFolderDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    attached->folderDialogListView()->forceActiveFocus();
    d->updateEnabled();
}

QQuickFolderDialogImplAttached *QQuickFolderDialogImplPrivate::attachedOrWarn()
{
    Q_Q(QQuickFolderDialogImpl);
    QQuickFolderDialogImplAttached *attached = static_cast<QQuickFolderDialogImplAttached*>(
        qmlAttachedPropertiesObject<QQuickFolderDialogImpl>(q));
    if (!attached)
        qmlWarning(q) << "Expected FileDialogImpl attached object to be present on" << this;
    return attached;
}

void QQuickFolderDialogImplAttachedPrivate::folderDialogListViewCurrentIndexChanged()
{
    auto folderDialogImpl = qobject_cast<QQuickFolderDialogImpl*>(parent);
    if (!folderDialogImpl)
        return;

    auto folderDialogDelegate = qobject_cast<QQuickFileDialogDelegate*>(folderDialogListView->currentItem());
    if (!folderDialogDelegate)
        return;

    folderDialogImpl->setSelectedFolder(folderDialogDelegate->file());
}

QQuickFolderDialogImplAttached::QQuickFolderDialogImplAttached(QObject *parent)
    : QObject(*(new QQuickFolderDialogImplAttachedPrivate), parent)
{
    if (!qobject_cast<QQuickFolderDialogImpl*>(parent)) {
        qmlWarning(this) << "FolderDialogImpl attached properties should only be "
            << "accessed through the root FileDialogImpl instance";
    }
}

QQuickListView *QQuickFolderDialogImplAttached::folderDialogListView() const
{
    Q_D(const QQuickFolderDialogImplAttached);
    return d->folderDialogListView;
}

void QQuickFolderDialogImplAttached::setFolderDialogListView(QQuickListView *folderDialogListView)
{
    Q_D(QQuickFolderDialogImplAttached);
    if (folderDialogListView == d->folderDialogListView)
        return;

    d->folderDialogListView = folderDialogListView;

    QObjectPrivate::connect(d->folderDialogListView, &QQuickListView::currentIndexChanged,
        d, &QQuickFolderDialogImplAttachedPrivate::folderDialogListViewCurrentIndexChanged);

    emit folderDialogListViewChanged();
}

QQuickFolderBreadcrumbBar *QQuickFolderDialogImplAttached::breadcrumbBar() const
{
    Q_D(const QQuickFolderDialogImplAttached);
    return d->breadcrumbBar;
}

void QQuickFolderDialogImplAttached::setBreadcrumbBar(QQuickFolderBreadcrumbBar *breadcrumbBar)
{
    Q_D(QQuickFolderDialogImplAttached);
    if (breadcrumbBar == d->breadcrumbBar)
        return;

    d->breadcrumbBar = breadcrumbBar;
    emit breadcrumbBarChanged();
}

QT_END_NAMESPACE

#include "moc_qquickfolderdialogimpl_p.cpp"
