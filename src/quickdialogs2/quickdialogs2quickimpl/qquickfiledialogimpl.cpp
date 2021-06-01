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

#include "qquickfiledialogimpl_p.h"
#include "qquickfiledialogimpl_p_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlfile.h>
#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupitem_p_p.h>
#include "qquickfiledialogdelegate_p.h"
#include "qquickfolderbreadcrumbbar_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCurrentFolder, "qt.quick.dialogs.quickfiledialogimpl.currentFolder")
Q_LOGGING_CATEGORY(lcOptions, "qt.quick.dialogs.quickfiledialogimpl.options")
Q_LOGGING_CATEGORY(lcNameFilters, "qt.quick.dialogs.quickfiledialogimpl.namefilters")
Q_LOGGING_CATEGORY(lcAttachedNameFilters, "qt.quick.dialogs.quickfiledialogimplattached.namefilters")

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

    openButton->setEnabled(!currentFile.isEmpty() && attached->breadcrumbBar()
        && !attached->breadcrumbBar()->textField()->isVisible());
}

/*!
    \internal

    Ensures that a file is always selected after a change in \c folder.

    \a oldFolderPath is the previous value of \c folder.
*/
void QQuickFileDialogImplPrivate::updateCurrentFile(const QString &oldFolderPath)
{
    Q_Q(QQuickFileDialogImpl);
    QQuickFileDialogImplAttached *attached = attachedOrWarn();
    if (!attached || !attached->fileDialogListView())
        return;

    QString newCurrentFilePath;
    int newCurrentFileIndex = 0;
    const QString newFolderPath = QQmlFile::urlToLocalFileOrQrc(currentFolder);
    if (!oldFolderPath.isEmpty() && !newFolderPath.isEmpty()) {
        // If the user went up a directory (or several), we should set
        // currentFile to be the directory that we were in (or
        // its closest ancestor that is a child of the new directory).
        // E.g. if oldFolderPath is /foo/bar/baz/abc/xyz, and newFolderPath is /foo/bar,
        // then we want to set currentFile to be /foo/bar/baz.
        const int indexOfFolder = oldFolderPath.indexOf(newFolderPath);
        if (indexOfFolder != -1) {
            // [folder]
            // [   oldFolderPath  ]
            // /foo/bar/baz/abc/xyz
            //         [rel...Paths]
            QStringList relativePaths = oldFolderPath.mid(indexOfFolder + newFolderPath.size()).split(QLatin1Char('/'), Qt::SkipEmptyParts);
            newCurrentFilePath = newFolderPath + QLatin1Char('/') + relativePaths.first();

            // Now find the index of that directory so that we can set the ListView's currentIndex to it.
            const QDir newFolderDir(newFolderPath);
            // Just to be safe...
            if (!newFolderDir.exists()) {
                qmlWarning(q) << "Directory" << newCurrentFilePath << "doesn't exist; can't get a file entry list for it";
                return;
            }

            const QFileInfoList dirs = newFolderDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::DirsFirst);
            const QFileInfo newCurrentFileInfo(newCurrentFilePath);
            // The directory can contain files, but since we put dirs first, that should never affect the indices.
            newCurrentFileIndex = dirs.indexOf(newCurrentFileInfo);
        }
    }

    if (newCurrentFilePath.isEmpty()) {
        // When entering into a directory that isn't a parent of the old one, the first
        // file delegate should be selected.
        // TODO: is there a cheaper way to do this? QDirIterator doesn't support sorting,
        // so we can't use that. QQuickFolderListModel uses threads to fetch its data,
        // so should be considered asynchronous. We might be able to use it, but it would
        // complicate the code even more...
        QDir newFolderDir(newFolderPath);
        if (newFolderDir.exists()) {
            const QFileInfoList files = newFolderDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst);
            if (!files.isEmpty())
                newCurrentFilePath = files.first().absoluteFilePath();
        }
    }

    if (!newCurrentFilePath.isEmpty()) {
        q->setCurrentFile(QUrl::fromLocalFile(newCurrentFilePath));
        attached->fileDialogListView()->setCurrentIndex(newCurrentFileIndex);
        if (QQuickItem *currentItem = attached->fileDialogListView()->currentItem())
            currentItem->forceActiveFocus();
    }
}

void QQuickFileDialogImplPrivate::handleAccept()
{
    // Let handleClick take care of calling accept().
}

void QQuickFileDialogImplPrivate::handleClick(QQuickAbstractButton *button)
{
    Q_Q(QQuickFileDialogImpl);
    if (buttonRole(button) == QPlatformDialogHelper::AcceptRole && currentFile.isValid()) {
        // The "Open" button was clicked, so we need to set the file to the current file, if any.
        const QFileInfo fileInfo(currentFile.toLocalFile());
        if (fileInfo.isDir()) {
            // If it's a directory, navigate to it.
            q->setCurrentFolder(currentFile);
            // Don't call accept(), because selecting a folder != accepting the dialog.
        } else {
            // Otherwise it's a file, so select it and close the dialog.
            q->setSelectedFile(currentFile);
            q->accept();
            QQuickDialogPrivate::handleClick(button);
            emit q->fileSelected(currentFile);
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

void QQuickFileDialogImpl::setCurrentFolder(const QUrl &currentFolder)
{
    qCDebug(lcCurrentFolder) << "setCurrentFolder called with" << currentFolder;
    Q_D(QQuickFileDialogImpl);
    if (currentFolder == d->currentFolder)
        return;

    const QString oldFolderPath = QQmlFile::urlToLocalFileOrQrc(d->currentFolder);

    d->currentFolder = currentFolder;
    // Since the directory changed, the old file can no longer be selected.
    setCurrentFile(QUrl());
    d->updateCurrentFile(oldFolderPath);
    emit currentFolderChanged(d->currentFolder);
}

QUrl QQuickFileDialogImpl::selectedFile() const
{
    Q_D(const QQuickFileDialogImpl);
    return d->selectedFile;
}

void QQuickFileDialogImpl::setSelectedFile(const QUrl &selectedFile)
{
    Q_D(QQuickFileDialogImpl);
    if (selectedFile == d->selectedFile)
        return;

    d->selectedFile = selectedFile;
    emit selectedFileChanged();
}

QUrl QQuickFileDialogImpl::currentFile() const
{
    Q_D(const QQuickFileDialogImpl);
    return d->currentFile;
}

void QQuickFileDialogImpl::setCurrentFile(const QUrl &currentFile)
{
    Q_D(QQuickFileDialogImpl);
    if (currentFile == d->currentFile)
        return;

    d->currentFile = currentFile;
    d->updateEnabled();
    emit currentFileChanged(d->currentFile);
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

    acceptButton->setText(!label.isEmpty()
        ? label : QQuickDialogButtonBoxPrivate::buttonText(QPlatformDialogHelper::Open));
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
        qmlAttachedPropertiesObject<QQuickFileDialogImpl>(q));
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

    fileDialogImpl->setCurrentFile(fileDialogDelegate->file());
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

QT_END_NAMESPACE
