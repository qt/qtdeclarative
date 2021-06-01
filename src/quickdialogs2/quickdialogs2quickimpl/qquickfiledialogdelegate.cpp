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

#include "qquickfiledialogdelegate_p.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/QQmlFile>
#include <QtQml/qqmlexpression.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTemplates2/private/qquickitemdelegate_p_p.h>

#include "qquickfiledialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickFileDialogDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickFileDialogDelegate)

public:
    void highlightFile();
    void chooseFile();

    bool acceptKeyClick(Qt::Key key) const override;

    QQuickFileDialogImpl *fileDialog = nullptr;
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
        fileDialog->setCurrentFile(file);
    }
}

void QQuickFileDialogDelegatePrivate::chooseFile()
{
    const QFileInfo fileInfo(QQmlFile::urlToLocalFileOrQrc(file));
    if (fileInfo.isDir()) {
        // If it's a directory, navigate to it.
        fileDialog->setCurrentFolder(file);
    } else {
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

QQuickFileDialogImpl *QQuickFileDialogDelegate::fileDialog() const
{
    Q_D(const QQuickFileDialogDelegate);
    return d->fileDialog;
}

void QQuickFileDialogDelegate::setFileDialog(QQuickFileDialogImpl *fileDialog)
{
    Q_D(QQuickFileDialogDelegate);
    if (fileDialog == d->fileDialog)
        return;

    d->fileDialog = fileDialog;
    emit fileDialogChanged();
}

QUrl QQuickFileDialogDelegate::file() const
{
    Q_D(const QQuickFileDialogDelegate);
    return d->file;
}

void QQuickFileDialogDelegate::setFile(const QUrl &file)
{
    Q_D(QQuickFileDialogDelegate);
    if (file == d->file)
        return;

    d->file = file;
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
