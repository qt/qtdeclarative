// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfiledialogdelegate_p.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/QQmlFile>
#include <QtQml/qqmlexpression.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTemplates2/private/qquickitemdelegate_p_p.h>

#include "qquickfiledialogimpl_p.h"
#include "qquickfolderdialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickFileDialogDelegatePrivate : public QQuickItemDelegatePrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickFileDialogDelegate)

    void highlightFile();
    void chooseFile();

    bool acceptKeyClick(Qt::Key key) const override;

    QQuickDialog *dialog = nullptr;
    QQuickFileDialogImpl *fileDialog = nullptr;
    QQuickFolderDialogImpl *folderDialog = nullptr;
    QUrl file;
};

void QQuickFileDialogDelegatePrivate::highlightFile()
{
    Q_Q(QQuickFileDialogDelegate);
    QQuickListViewAttached *attached = static_cast<QQuickListViewAttached*>(
        qmlAttachedPropertiesObject<QQuickListView>(q));
    if (!attached)
        return;

    QQmlContext *delegateContext = qmlContext(q);
    if (!delegateContext)
        return;

    bool converted = false;
    const int index = q->property("index").toInt(&converted);
    if (converted) {
        attached->view()->setCurrentIndex(index);
        if (fileDialog)
            fileDialog->setSelectedFile(file);
        else if (folderDialog)
            folderDialog->setSelectedFolder(file);
    }
}

void QQuickFileDialogDelegatePrivate::chooseFile()
{
    const QFileInfo fileInfo(QQmlFile::urlToLocalFileOrQrc(file));
    if (fileInfo.isDir()) {
        // If it's a directory, navigate to it.
        if (fileDialog)
            fileDialog->setCurrentFolder(file);
        else
            folderDialog->setCurrentFolder(file);
    } else {
        Q_ASSERT(fileDialog);
        // Otherwise it's a file, so select it and close the dialog.
        fileDialog->setSelectedFile(file);
        fileDialog->accept();
    }
}

bool QQuickFileDialogDelegatePrivate::acceptKeyClick(Qt::Key key) const
{
    return key == Qt::Key_Return || key == Qt::Key_Enter;
}

QQuickFileDialogDelegate::QQuickFileDialogDelegate(QQuickItem *parent)
    : QQuickItemDelegate(*(new QQuickFileDialogDelegatePrivate), parent)
{
    Q_D(QQuickFileDialogDelegate);
    // Clicking and tabbing should result in it getting focus,
    // as e.g. Ubuntu and Windows both allow tabbing through file dialogs.
    setFocusPolicy(Qt::StrongFocus);
    setCheckable(true);
    QObjectPrivate::connect(this, &QQuickFileDialogDelegate::clicked,
        d, &QQuickFileDialogDelegatePrivate::highlightFile);
    QObjectPrivate::connect(this, &QQuickFileDialogDelegate::doubleClicked,
        d, &QQuickFileDialogDelegatePrivate::chooseFile);
}

QQuickDialog *QQuickFileDialogDelegate::dialog() const
{
    Q_D(const QQuickFileDialogDelegate);
    return d->dialog;
}

void QQuickFileDialogDelegate::setDialog(QQuickDialog *dialog)
{
    Q_D(QQuickFileDialogDelegate);
    if (dialog == d->dialog)
        return;

    d->dialog = dialog;
    d->fileDialog = qobject_cast<QQuickFileDialogImpl*>(dialog);
    d->folderDialog = qobject_cast<QQuickFolderDialogImpl*>(dialog);
    emit dialogChanged();
}

QUrl QQuickFileDialogDelegate::file() const
{
    Q_D(const QQuickFileDialogDelegate);
    return d->file;
}

void QQuickFileDialogDelegate::setFile(const QUrl &file)
{
    Q_D(QQuickFileDialogDelegate);
    QUrl adjustedFile = file;
#ifdef Q_OS_WIN32
    // Work around QTBUG-99105 (FolderListModel uses lowercase drive letter).
    QString path = adjustedFile.path();
    const int driveColonIndex = path.indexOf(QLatin1Char(':'));
    if (driveColonIndex == 2) {
        path.replace(1, 1, path.at(1).toUpper());
        adjustedFile.setPath(path);
    }
#endif
    if (adjustedFile == d->file)
        return;

    d->file = adjustedFile;
    emit fileChanged();
}

void QQuickFileDialogDelegate::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickFileDialogDelegate);
    // We need to respond to being triggered by enter being pressed,
    // but we can't use event->isAccepted() to check, because events are pre-accepted.
    auto connection = QObjectPrivate::connect(this, &QQuickFileDialogDelegate::clicked,
        d, &QQuickFileDialogDelegatePrivate::chooseFile);

    QQuickItemDelegate::keyReleaseEvent(event);

    disconnect(connection);
}

QT_END_NAMESPACE

#include "moc_qquickfiledialogdelegate_p.cpp"
