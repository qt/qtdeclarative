// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfiledialogimpl_p.h"
#include "qquickfiledialogimpl_p_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick/private/qquickitemview_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupitem_p_p.h>
#include <QtQuickControls2Impl/private/qquickplatformtheme_p.h>
#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>

#include "qquickfiledialogdelegate_p.h"
#include "qquickfolderbreadcrumbbar_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCurrentFolder, "qt.quick.dialogs.quickfiledialogimpl.currentFolder")
Q_LOGGING_CATEGORY(lcSelectedFile, "qt.quick.dialogs.quickfiledialogimpl.selectedFile")
Q_LOGGING_CATEGORY(lcUpdateSelectedFile, "qt.quick.dialogs.quickfiledialogimpl.updateSelectedFile")
Q_LOGGING_CATEGORY(lcOptions, "qt.quick.dialogs.quickfiledialogimpl.options")
Q_LOGGING_CATEGORY(lcNameFilters, "qt.quick.dialogs.quickfiledialogimpl.namefilters")
Q_LOGGING_CATEGORY(lcAttachedNameFilters, "qt.quick.dialogs.quickfiledialogimplattached.namefilters")
Q_LOGGING_CATEGORY(lcAttachedCurrentIndex, "qt.quick.dialogs.quickfiledialogimplattached.currentIndex")

QQuickFileDialogImplPrivate::QQuickFileDialogImplPrivate()
{
}

void QQuickFileDialogImplPrivate::setNameFilters(const QStringList &filters)
{
    Q_Q(QQuickFileDialogImpl);
    if (filters == nameFilters)
        return;

    nameFilters = filters;
    emit q->nameFiltersChanged();
}

void QQuickFileDialogImplPrivate::updateEnabled()
{
    Q_Q(QQuickFileDialogImpl);
    QQuickFileDialogImplAttached *attached = attachedOrWarn();
    if (!attached)
        return;

    auto openButton = attached->buttonBox()->standardButton(QPlatformDialogHelper::Open);
    if (!openButton) {
        qmlWarning(q).nospace() << "Can't update Open button's enabled state because it wasn't found";
        return;
    }

    openButton->setEnabled(!selectedFile.isEmpty() && attached->breadcrumbBar()
        && !attached->breadcrumbBar()->textField()->isVisible());
}

/*!
    \internal

    Ensures that a file is always selected after a change in \c folder.

    \a oldFolderPath is the previous value of \c folder.
*/
void QQuickFileDialogImplPrivate::updateSelectedFile(const QString &oldFolderPath)
{
    Q_Q(QQuickFileDialogImpl);
    QQuickFileDialogImplAttached *attached = attachedOrWarn();
    if (!attached || !attached->fileDialogListView())
        return;

    qCDebug(lcUpdateSelectedFile) << "updateSelectedFile called with oldFolderPath" << oldFolderPath;

    QString newSelectedFilePath;
    int newSelectedFileIndex = -1;
    const QString newFolderPath = QQmlFile::urlToLocalFileOrQrc(currentFolder);
    if (!oldFolderPath.isEmpty() && !newFolderPath.isEmpty()) {
        // TODO: Add another platform theme hint for this behavior too, as e.g. macOS
        // doesn't do it this way.
        // If the user went up a directory (or several), we should set
        // selectedFile to be the directory that we were in (or
        // its closest ancestor that is a child of the new directory).
        // E.g. if oldFolderPath is /foo/bar/baz/abc/xyz, and newFolderPath is /foo/bar,
        // then we want to set selectedFile to be /foo/bar/baz.
        const int indexOfFolder = oldFolderPath.indexOf(newFolderPath);
        if (indexOfFolder != -1) {
            // [folder]
            // [   oldFolderPath  ]
            // /foo/bar/baz/abc/xyz
            //         [rel...Paths]
            QStringList relativePaths = oldFolderPath.mid(indexOfFolder + newFolderPath.size()).split(QLatin1Char('/'), Qt::SkipEmptyParts);
            newSelectedFilePath = newFolderPath + QLatin1Char('/') + relativePaths.first();

            // Now find the index of that directory so that we can set the ListView's currentIndex to it.
            const QDir newFolderDir(newFolderPath);
            // Just to be safe...
            if (!newFolderDir.exists()) {
                qmlWarning(q) << "Directory" << newSelectedFilePath << "doesn't exist; can't get a file entry list for it";
                return;
            }

            const QFileInfoList filesInNewDir = fileList(newFolderDir);
            const QFileInfo newSelectedFileInfo(newSelectedFilePath);
            newSelectedFileIndex = filesInNewDir.indexOf(newSelectedFileInfo);
        }
    }

    static const bool preselectFirstFile = []() {
        const QVariant envVar = qEnvironmentVariable("QT_QUICK_DIALOGS_PRESELECT_FIRST_FILE");
        if (envVar.isValid() && envVar.canConvert<bool>())
            return envVar.toBool();
        return QGuiApplicationPrivate::platformTheme()->themeHint(
            QPlatformTheme::PreselectFirstFileInDirectory).toBool();
    }();

    if (preselectFirstFile && newSelectedFilePath.isEmpty()) {
        // When entering into a directory that isn't a parent of the old one, the first
        // file delegate should be selected.
        // TODO: is there a cheaper way to do this? QDirIterator doesn't support sorting,
        // so we can't use that. QQuickFolderListModel uses threads to fetch its data,
        // so should be considered asynchronous. We might be able to use it, but it would
        // complicate the code even more...
        const QDir newFolderDir(newFolderPath);
        if (newFolderDir.exists()) {
            if (!cachedFileList.isEmpty()) {
                newSelectedFilePath = cachedFileList.first().absoluteFilePath();
                newSelectedFileIndex = 0;
            }
        }
    }

    const QUrl newSelectedFileUrl = QUrl::fromLocalFile(newSelectedFilePath);
    qCDebug(lcUpdateSelectedFile).nospace() << "updateSelectedFile is setting selectedFile to " << newSelectedFileUrl
        << ", newSelectedFileIndex is " << newSelectedFileIndex;
    q->setSelectedFile(newSelectedFileUrl);
    // If the index is -1, there are no files in the directory, and so fileDialogListView's
    // currentIndex will already be -1.
    if (newSelectedFileIndex != -1)
        tryUpdateFileDialogListViewCurrentIndex(newSelectedFileIndex);
}

QDir::SortFlags QQuickFileDialogImplPrivate::fileListSortFlags()
{
    QDir::SortFlags sortFlags = QDir::IgnoreCase;
    if (QQuickPlatformTheme::getThemeHint(QPlatformTheme::ShowDirectoriesFirst).toBool())
        sortFlags.setFlag(QDir::DirsFirst);
    return sortFlags;
}

QFileInfoList QQuickFileDialogImplPrivate::fileList(const QDir &dir)
{
    return dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, fileListSortFlags());
}

void QQuickFileDialogImplPrivate::setFileDialogListViewCurrentIndex(int newCurrentIndex)
{
    qCDebug(lcSelectedFile) << "setting fileDialogListView's currentIndex to" << newCurrentIndex;

    // We block signals from ListView because we don't want fileDialogListViewCurrentIndexChanged
    // to be called, as the file it gets from the delegate will not be up-to-date (but most
    // importantly because we already just set the selected file).
    QQuickFileDialogImplAttached *attached = attachedOrWarn();
    const QSignalBlocker blocker(attached->fileDialogListView());
    attached->fileDialogListView()->setCurrentIndex(newCurrentIndex);
    attached->fileDialogListView()->positionViewAtIndex(newCurrentIndex, QQuickListView::Center);
    if (QQuickItem *currentItem = attached->fileDialogListView()->currentItem())
        currentItem->forceActiveFocus();
}

/*!
    \internal

    Tries to set the currentIndex of fileDialogListView to \a newCurrentIndex and gives
    focus to the current item.
*/
void QQuickFileDialogImplPrivate::tryUpdateFileDialogListViewCurrentIndex(int newCurrentIndex)
{
    qCDebug(lcSelectedFile) << "tryUpdateFileDialogListViewCurrentIndex called with newCurrentIndex" << newCurrentIndex;
    QQuickFileDialogImplAttached *attached = attachedOrWarn();
    Q_ASSERT(attached);
    Q_ASSERT(attached->fileDialogListView());

    // We were likely trying to set an index for a file that the ListView hadn't loaded yet.
    // We need to wait until the ListView has loaded all expected items, but since we have no
    // efficient way of verifying that, we just check that the count is as expected.
    if (newCurrentIndex != -1 && newCurrentIndex >= attached->fileDialogListView()->count()) {
        qCDebug(lcSelectedFile) << "- trying to set currentIndex to" << newCurrentIndex
            << "but fileDialogListView only has" << attached->fileDialogListView()->count()
            << "items; setting pendingCurrentIndexToSet to" << newCurrentIndex;
        pendingCurrentIndexToSet = newCurrentIndex;
        QObjectPrivate::connect(attached->fileDialogListView(), &QQuickItemView::countChanged,
            this, &QQuickFileDialogImplPrivate::fileDialogListViewCountChanged, Qt::ConnectionType(Qt::DirectConnection | Qt::UniqueConnection));
        return;
    }

    setFileDialogListViewCurrentIndex(newCurrentIndex);
}

void QQuickFileDialogImplPrivate::fileDialogListViewCountChanged()
{
    QQuickFileDialogImplAttached *attached = attachedOrWarn();
    qCDebug(lcSelectedFile) << "fileDialogListView count changed to" << attached->fileDialogListView()->count();

    if (pendingCurrentIndexToSet != -1 && pendingCurrentIndexToSet < attached->fileDialogListView()->count()) {
        // The view now has all of the items we expect it to, so we can set
        // its currentIndex back to the selected file.
        qCDebug(lcSelectedFile) << "- ListView has expected count;"
            << "applying pending fileDialogListView currentIndex" << pendingCurrentIndexToSet;

        QObjectPrivate::disconnect(attached->fileDialogListView(), &QQuickItemView::countChanged,
            this, &QQuickFileDialogImplPrivate::fileDialogListViewCountChanged);
        setFileDialogListViewCurrentIndex(pendingCurrentIndexToSet);
        pendingCurrentIndexToSet = -1;
        qCDebug(lcSelectedFile) << "- reset pendingCurrentIndexToSet to -1";
    } else {
        qCDebug(lcSelectedFile) << "- ListView doesn't yet have expected count of" << cachedFileList.size();
    }
}

void QQuickFileDialogImplPrivate::handleAccept()
{
    // Let handleClick take care of calling accept().
}

void QQuickFileDialogImplPrivate::handleClick(QQuickAbstractButton *button)
{
    Q_Q(QQuickFileDialogImpl);
    if (buttonRole(button) == QPlatformDialogHelper::AcceptRole && selectedFile.isValid()) {
        // The "Open" button was clicked, so we need to set the file to the current file, if any.
        const QFileInfo fileInfo(selectedFile.toLocalFile());
        if (fileInfo.isDir()) {
            // If it's a directory, navigate to it.
            q->setCurrentFolder(selectedFile);
            // Don't call accept(), because selecting a folder != accepting the dialog.
        } else {
            // Otherwise it's a file, so select it and close the dialog.
            q->setSelectedFile(selectedFile);
            q->accept();
            QQuickDialogPrivate::handleClick(button);
            emit q->fileSelected(selectedFile);
        }
    }
}

QQuickFileDialogImpl::QQuickFileDialogImpl(QObject *parent)
    : QQuickDialog(*(new QQuickFileDialogImplPrivate), parent)
{
}

QQuickFileDialogImplAttached *QQuickFileDialogImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickFileDialogImplAttached(object);
}

QUrl QQuickFileDialogImpl::currentFolder() const
{
    Q_D(const QQuickFileDialogImpl);
    return d->currentFolder;
}

void QQuickFileDialogImpl::setCurrentFolder(const QUrl &currentFolder, SetReason setReason)
{
    Q_D(QQuickFileDialogImpl);
    qCDebug(lcCurrentFolder).nospace() << "setCurrentFolder called with " << currentFolder
        << " (old currentFolder is " << d->currentFolder << ")";

    // As we would otherwise get the file list from scratch in a couple of places,
    // just get it once and cache it.
    // We need to cache it before the equality check, otherwise opening the dialog
    // several times in the same directory wouldn't update the cache.
    if (!currentFolder.isEmpty())
        d->cachedFileList = d->fileList(QQmlFile::urlToLocalFileOrQrc(currentFolder));
    else
        d->cachedFileList.clear();
    qCDebug(lcCurrentFolder) << "- cachedFileList size is now " << d->cachedFileList.size();

    if (currentFolder == d->currentFolder)
        return;

    const QString oldFolderPath = QQmlFile::urlToLocalFileOrQrc(d->currentFolder);

    d->currentFolder = currentFolder;
    // Don't update the selectedFile if it's an Internal set, as that
    // means that the user just set selectedFile, and we're being called as a result of that.
    if (setReason == SetReason::External) {
        // Since the directory changed, the old file can no longer be selected.
        d->updateSelectedFile(oldFolderPath);
    }
    emit currentFolderChanged(d->currentFolder);
}

QUrl QQuickFileDialogImpl::selectedFile() const
{
    Q_D(const QQuickFileDialogImpl);
    return d->selectedFile;
}

/*!
    \internal

    This is mostly called as a result of user interaction, but is also
    called (indirectly) by QQuickFileDialog::onShow when the user set an initial
    selectedFile.
*/
void QQuickFileDialogImpl::setSelectedFile(const QUrl &selectedFile)
{
    qCDebug(lcSelectedFile) << "setSelectedFile called with" << selectedFile;
    Q_D(QQuickFileDialogImpl);
    if (selectedFile == d->selectedFile)
        return;

    d->selectedFile = selectedFile;
    d->updateEnabled();
    emit selectedFileChanged(d->selectedFile);
}

/*!
    \internal

    Called when showing the FileDialog each time, so long as
    QFileDialogOptions::initiallySelectedFiles is not empty.
*/
void QQuickFileDialogImpl::setInitialCurrentFolderAndSelectedFile(const QUrl &file)
{
    Q_D(QQuickFileDialogImpl);
    const QUrl fileDirUrl = QUrl::fromLocalFile(QFileInfo(file.toLocalFile()).dir().absolutePath());
    const bool currentFolderChanged = d->currentFolder != fileDirUrl;
    qCDebug(lcSelectedFile) << "setting initial currentFolder to" << fileDirUrl << "and selectedFile to" << file;
    setCurrentFolder(fileDirUrl, QQuickFileDialogImpl::SetReason::Internal);
    setSelectedFile(file);
    d->setCurrentIndexToInitiallySelectedFile = true;

    // If the currentFolder didn't change, the FolderListModel won't change and
    // neither will the ListView. This means that setFileDialogListViewCurrentIndex
    // will never get called and the currentIndex will not reflect selectedFile.
    // We need to account for that here.
    if (!currentFolderChanged) {
        const QFileInfo newSelectedFileInfo(d->selectedFile.toLocalFile());
        const int indexOfSelectedFileInFileDialogListView = d->cachedFileList.indexOf(newSelectedFileInfo);
        d->tryUpdateFileDialogListViewCurrentIndex(indexOfSelectedFileInFileDialogListView);
    }
}

QSharedPointer<QFileDialogOptions> QQuickFileDialogImpl::options() const
{
    Q_D(const QQuickFileDialogImpl);
    return d->options;
}

void QQuickFileDialogImpl::setOptions(const QSharedPointer<QFileDialogOptions> &options)
{
    qCDebug(lcOptions).nospace() << "setOptions called with:"
        << " acceptMode=" << options->acceptMode()
        << " fileMode=" << options->fileMode()
        << " initialDirectory=" << options->initialDirectory()
        << " nameFilters=" << options->nameFilters()
        << " initiallySelectedNameFilter=" << options->initiallySelectedNameFilter();

    Q_D(QQuickFileDialogImpl);
    d->options = options;

    if (d->options) {
        d->selectedNameFilter->setOptions(options);
        d->setNameFilters(options->nameFilters());

        if (auto attached = d->attachedOrWarn()) {
            const bool isSaveMode = d->options->fileMode() == QFileDialogOptions::AnyFile;
            attached->fileNameLabel()->setVisible(isSaveMode);
            attached->fileNameTextField()->setVisible(isSaveMode);
        }
    }
}

/*!
    \internal

    The list of user-facing strings describing the available file filters.
*/
QStringList QQuickFileDialogImpl::nameFilters() const
{
    Q_D(const QQuickFileDialogImpl);
    return d->options ? d->options->nameFilters() : QStringList();
}

void QQuickFileDialogImpl::resetNameFilters()
{
    Q_D(QQuickFileDialogImpl);
    d->setNameFilters(QStringList());
}

QQuickFileNameFilter *QQuickFileDialogImpl::selectedNameFilter() const
{
    Q_D(const QQuickFileDialogImpl);
    if (!d->selectedNameFilter) {
        QQuickFileDialogImpl *that = const_cast<QQuickFileDialogImpl *>(this);
        d->selectedNameFilter = new QQuickFileNameFilter(that);
        if (d->options)
            d->selectedNameFilter->setOptions(d->options);
    }
    return d->selectedNameFilter;
}

/*!
    \internal

    These allow QQuickPlatformFileDialog::show() to set custom labels on the
    dialog buttons without having to know about/go through QQuickFileDialogImplAttached
    and QQuickDialogButtonBox.
*/
void QQuickFileDialogImpl::setAcceptLabel(const QString &label)
{
    Q_D(QQuickFileDialogImpl);
    d->acceptLabel = label;
    QQuickFileDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    auto acceptButton = attached->buttonBox()->standardButton(QPlatformDialogHelper::Open);
    if (!acceptButton) {
        qmlWarning(this).nospace() << "Can't set accept label to " << label
            << "; failed to find Open button in DialogButtonBox of " << this;
        return;
    }

    auto buttonType = d->options->acceptMode() == QFileDialogOptions::AcceptSave
        ? QPlatformDialogHelper::Save : QPlatformDialogHelper::Open;
    acceptButton->setText(!label.isEmpty()
        ? label : QQuickDialogButtonBoxPrivate::buttonText(buttonType));
}

void QQuickFileDialogImpl::setRejectLabel(const QString &label)
{
    Q_D(QQuickFileDialogImpl);
    d->rejectLabel = label;
    QQuickFileDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    auto rejectButton = attached->buttonBox()->standardButton(QPlatformDialogHelper::Cancel);
    if (!rejectButton) {
        qmlWarning(this).nospace() << "Can't set reject label to " << label
            << "; failed to find Open button in DialogButtonBox of " << this;
        return;
    }

    rejectButton->setText(!label.isEmpty()
        ? label : QQuickDialogButtonBoxPrivate::buttonText(QPlatformDialogHelper::Cancel));
}

void QQuickFileDialogImpl::selectNameFilter(const QString &filter)
{
    qCDebug(lcNameFilters) << "selectNameFilter called with" << filter;
    Q_D(QQuickFileDialogImpl);
    d->selectedNameFilter->update(filter);
    emit filterSelected(filter);
}

QString QQuickFileDialogImpl::fileName() const
{
    return selectedFile().fileName();
}
void QQuickFileDialogImpl::setFileName(const QString &fileName)
{
    const QString previous = selectedFile().fileName();
    if (previous == fileName)
        return;

    setSelectedFile(QUrl(currentFolder().path() + u'/' + fileName));
}

void QQuickFileDialogImpl::componentComplete()
{
    Q_D(QQuickFileDialogImpl);
    QQuickDialog::componentComplete();

    // Find the right-most button and set its key navigation so that
    // tab moves focus to the breadcrumb bar's up button. I tried
    // doing this via KeyNavigation on the DialogButtonBox in QML,
    // but it didn't work (probably because it's not the right item).
    QQuickFileDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    const int buttonCount = attached->buttonBox()->count();
    if (buttonCount == 0)
        return;

    QQuickAbstractButton *rightMostButton = qobject_cast<QQuickAbstractButton *>(
        attached->buttonBox()->itemAt(buttonCount - 1));
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

void QQuickFileDialogImpl::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_D(QQuickFileDialogImpl);
    QQuickDialog::itemChange(change, data);

    if (change != QQuickItem::ItemVisibleHasChanged || !isComponentComplete() || !data.boolValue)
        return;

    QQuickFileDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    attached->fileDialogListView()->forceActiveFocus();
    d->updateEnabled();
}

QQuickFileDialogImplAttached *QQuickFileDialogImplPrivate::attachedOrWarn()
{
    Q_Q(QQuickFileDialogImpl);
    QQuickFileDialogImplAttached *attached = static_cast<QQuickFileDialogImplAttached*>(
        qmlAttachedPropertiesObject<QQuickFileDialogImpl>(q, false));
    if (!attached)
        qmlWarning(q) << "Expected FileDialogImpl attached object to be present on" << this;
    return attached;
}

void QQuickFileDialogImplAttachedPrivate::nameFiltersComboBoxItemActivated(int index)
{
    qCDebug(lcAttachedNameFilters) << "nameFiltersComboBoxItemActivated called with" << index;
    auto fileDialogImpl = qobject_cast<QQuickFileDialogImpl*>(parent);
    if (!fileDialogImpl)
        return;

    fileDialogImpl->selectNameFilter(nameFiltersComboBox->textAt(index));
}

void QQuickFileDialogImplAttachedPrivate::fileDialogListViewCurrentIndexChanged()
{
    auto fileDialogImpl = qobject_cast<QQuickFileDialogImpl*>(parent);
    if (!fileDialogImpl)
        return;

    auto fileDialogDelegate = qobject_cast<QQuickFileDialogDelegate*>(fileDialogListView->currentItem());
    if (!fileDialogDelegate)
        return;

    const QQuickItemViewPrivate::MovementReason moveReason = QQuickItemViewPrivate::get(fileDialogListView)->moveReason;
    qCDebug(lcAttachedCurrentIndex).nospace() << "fileDialogListView currentIndex changed to " << fileDialogListView->currentIndex()
        << " with moveReason " << moveReason
        << "; the file at that index is " << fileDialogDelegate->file();

    // Only update selectedFile if the currentIndex changed as a result of user interaction;
    // things like model changes (i.e. QQuickItemViewPrivate::applyModelChanges() calling
    // QQuickItemViewPrivate::updateCurrent as a result of us changing the directory on the FolderListModel)
    // shouldn't cause the selectedFile to change.
    auto fileDialogImplPrivate = QQuickFileDialogImplPrivate::get(fileDialogImpl);
    if (moveReason != QQuickItemViewPrivate::Other) {
        fileDialogImpl->setSelectedFile(fileDialogDelegate->file());
    } else if (fileDialogImplPrivate->setCurrentIndexToInitiallySelectedFile) {
        // When setting selectedFile before opening the FileDialog,
        // we need to ensure that the currentIndex is correct, because the initial change
        // in directory will cause the underyling FolderListModel to change its folder property,
        // which in turn resets the fileDialogListView's currentIndex to 0.
        const QFileInfo newSelectedFileInfo(fileDialogImplPrivate->selectedFile.toLocalFile());
        const int indexOfSelectedFileInFileDialogListView = fileDialogImplPrivate->cachedFileList.indexOf(newSelectedFileInfo);
        fileDialogImplPrivate->tryUpdateFileDialogListViewCurrentIndex(indexOfSelectedFileInFileDialogListView);
        fileDialogImplPrivate->setCurrentIndexToInitiallySelectedFile = false;
    }
}

void QQuickFileDialogImplAttachedPrivate::fileNameEditedByUser()
{
    if (!buttonBox)
        return;
    auto openButton = buttonBox->standardButton(QPlatformDialogHelper::Open);
    if (!openButton || !fileNameTextField)
        return;
    openButton->setEnabled(!fileNameTextField->text().isEmpty());
}

void QQuickFileDialogImplAttachedPrivate::fileNameEditingByUserFinished()
{
    auto fileDialogImpl = qobject_cast<QQuickFileDialogImpl *>(parent);
    if (!fileDialogImpl)
        return;

    fileDialogImpl->setFileName(fileNameTextField->text());
}

QQuickFileDialogImplAttached::QQuickFileDialogImplAttached(QObject *parent)
    : QObject(*(new QQuickFileDialogImplAttachedPrivate), parent)
{
    if (!qobject_cast<QQuickFileDialogImpl*>(parent)) {
        qmlWarning(this) << "FileDialogImpl attached properties should only be "
            << "accessed through the root FileDialogImpl instance";
    }
}

QQuickDialogButtonBox *QQuickFileDialogImplAttached::buttonBox() const
{
    Q_D(const QQuickFileDialogImplAttached);
    return d->buttonBox;
}

void QQuickFileDialogImplAttached::setButtonBox(QQuickDialogButtonBox *buttonBox)
{
    Q_D(QQuickFileDialogImplAttached);
    if (buttonBox == d->buttonBox)
        return;

    if (d->buttonBox) {
        QQuickFileDialogImpl *fileDialogImpl = qobject_cast<QQuickFileDialogImpl*>(parent());
        if (fileDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(fileDialogImpl);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::accepted,
                dialogPrivate, &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::rejected,
                dialogPrivate, &QQuickDialogPrivate::handleReject);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::clicked,
                dialogPrivate, &QQuickDialogPrivate::handleClick);
        }
    }

    d->buttonBox = buttonBox;

    if (buttonBox) {
        QQuickFileDialogImpl *fileDialogImpl = qobject_cast<QQuickFileDialogImpl*>(parent());
        if (fileDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(fileDialogImpl);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::accepted,
                dialogPrivate, &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::rejected,
                dialogPrivate, &QQuickDialogPrivate::handleReject);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::clicked,
                dialogPrivate, &QQuickDialogPrivate::handleClick);
        }
    }

    emit buttonBoxChanged();
}

QQuickComboBox *QQuickFileDialogImplAttached::nameFiltersComboBox() const
{
    Q_D(const QQuickFileDialogImplAttached);
    return d->nameFiltersComboBox;
}

void QQuickFileDialogImplAttached::setNameFiltersComboBox(QQuickComboBox *nameFiltersComboBox)
{
    Q_D(QQuickFileDialogImplAttached);
    if (nameFiltersComboBox == d->nameFiltersComboBox)
        return;

    d->nameFiltersComboBox = nameFiltersComboBox;

    QObjectPrivate::connect(d->nameFiltersComboBox, &QQuickComboBox::activated,
        d, &QQuickFileDialogImplAttachedPrivate::nameFiltersComboBoxItemActivated);

    emit nameFiltersComboBoxChanged();
}

QString QQuickFileDialogImplAttached::selectedNameFilter() const
{
    Q_D(const QQuickFileDialogImplAttached);
    return d->nameFiltersComboBox ? d->nameFiltersComboBox->currentText() : QString();
}

void QQuickFileDialogImplAttached::selectNameFilter(const QString &filter)
{
    Q_D(QQuickFileDialogImplAttached);
    qCDebug(lcAttachedNameFilters) << "selectNameFilter called with" << filter;
    if (!d->nameFiltersComboBox)
        return;

    const int indexInComboBox = d->nameFiltersComboBox->find(filter);
    if (indexInComboBox == -1)
        return;

    qCDebug(lcAttachedNameFilters) << "setting ComboBox's currentIndex to" << indexInComboBox;
    d->nameFiltersComboBox->setCurrentIndex(indexInComboBox);
}

QQuickListView *QQuickFileDialogImplAttached::fileDialogListView() const
{
    Q_D(const QQuickFileDialogImplAttached);
    return d->fileDialogListView;
}

void QQuickFileDialogImplAttached::setFileDialogListView(QQuickListView *fileDialogListView)
{
    Q_D(QQuickFileDialogImplAttached);
    if (fileDialogListView == d->fileDialogListView)
        return;

    d->fileDialogListView = fileDialogListView;

    QObjectPrivate::connect(d->fileDialogListView, &QQuickListView::currentIndexChanged,
        d, &QQuickFileDialogImplAttachedPrivate::fileDialogListViewCurrentIndexChanged);

    emit fileDialogListViewChanged();
}

QQuickFolderBreadcrumbBar *QQuickFileDialogImplAttached::breadcrumbBar() const
{
    Q_D(const QQuickFileDialogImplAttached);
    return d->breadcrumbBar;
}

void QQuickFileDialogImplAttached::setBreadcrumbBar(QQuickFolderBreadcrumbBar *breadcrumbBar)
{
    Q_D(QQuickFileDialogImplAttached);
    if (breadcrumbBar == d->breadcrumbBar)
        return;

    d->breadcrumbBar = breadcrumbBar;
    emit breadcrumbBarChanged();
}

QQuickLabel *QQuickFileDialogImplAttached::fileNameLabel() const
{
    Q_D(const QQuickFileDialogImplAttached);
    return d->fileNameLabel;
}

void QQuickFileDialogImplAttached::setFileNameLabel(QQuickLabel *fileNameLabel)
{
    Q_D(QQuickFileDialogImplAttached);
    if (fileNameLabel == d->fileNameLabel)
        return;

    d->fileNameLabel = fileNameLabel;

    emit fileNameLabelChanged();
}

QQuickTextField *QQuickFileDialogImplAttached::fileNameTextField() const
{
    Q_D(const QQuickFileDialogImplAttached);
    return d->fileNameTextField;
}

void QQuickFileDialogImplAttached::setFileNameTextField(QQuickTextField *fileNameTextField)
{
    Q_D(QQuickFileDialogImplAttached);
    if (fileNameTextField == d->fileNameTextField)
        return;

    if (d->fileNameTextField) {
        QObjectPrivate::disconnect(d->fileNameTextField, &QQuickTextField::editingFinished,
            d, &QQuickFileDialogImplAttachedPrivate::fileNameEditingByUserFinished);
        QObjectPrivate::disconnect(d->fileNameTextField, &QQuickTextField::textEdited,
            d, &QQuickFileDialogImplAttachedPrivate::fileNameEditedByUser);
    }

    d->fileNameTextField = fileNameTextField;

    if (d->fileNameTextField) {
        QObjectPrivate::connect(d->fileNameTextField, &QQuickTextField::editingFinished,
            d, &QQuickFileDialogImplAttachedPrivate::fileNameEditingByUserFinished);
        QObjectPrivate::connect(d->fileNameTextField, &QQuickTextField::textEdited,
            d, &QQuickFileDialogImplAttachedPrivate::fileNameEditedByUser);
    }
    emit fileNameTextFieldChanged();
}

QT_END_NAMESPACE

#include "moc_qquickfiledialogimpl_p.cpp"
