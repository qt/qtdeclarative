/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKFOLDERDIALOGIMPL_P_H
#define QQUICKFOLDERDIALOGIMPL_P_H

//
//  W A R N I N G
//  -------------
//
// This folder is not part of the Qt API.  It exists purely as an
// implementation detail.  This header folder may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickDialogButtonBox;

class QQuickFolderDialogImplAttached;
class QQuickFolderDialogImplAttachedPrivate;
class QQuickFolderDialogImplPrivate;
class QQuickFolderBreadcrumbBar;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFolderDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QUrl selectedFolder READ selectedFolder WRITE setSelectedFolder NOTIFY selectedFolderChanged FINAL)
    QML_NAMED_ELEMENT(FolderDialogImpl)
    QML_ATTACHED(QQuickFolderDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickFolderDialogImpl(QObject *parent = nullptr);

    static QQuickFolderDialogImplAttached *qmlAttachedProperties(QObject *object);

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &folder);

    QUrl selectedFolder() const;
    void setSelectedFolder(const QUrl &selectedFolder);

    QSharedPointer<QFileDialogOptions> options() const;
    void setOptions(const QSharedPointer<QFileDialogOptions> &options);

    void setAcceptLabel(const QString &label);
    void setRejectLabel(const QString &label);

Q_SIGNALS:
    void currentFolderChanged(const QUrl &folderUrl);
    void selectedFolderChanged(const QUrl &folderUrl);
    void nameFiltersChanged();

private:
    void componentComplete() override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data) override;

    Q_DISABLE_COPY(QQuickFolderDialogImpl)
    Q_DECLARE_PRIVATE(QQuickFolderDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFolderDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickListView *folderDialogListView READ folderDialogListView WRITE setFolderDialogListView NOTIFY folderDialogListViewChanged)
    Q_PROPERTY(QQuickFolderBreadcrumbBar *breadcrumbBar READ breadcrumbBar WRITE setBreadcrumbBar NOTIFY breadcrumbBarChanged)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>)

public:
    explicit QQuickFolderDialogImplAttached(QObject *parent = nullptr);

    QQuickListView *folderDialogListView() const;
    void setFolderDialogListView(QQuickListView *folderDialogListView);

    QQuickFolderBreadcrumbBar *breadcrumbBar() const;
    void setBreadcrumbBar(QQuickFolderBreadcrumbBar *breadcrumbBar);

Q_SIGNALS:
    void folderDialogListViewChanged();
    void breadcrumbBarChanged();

private:
    Q_DISABLE_COPY(QQuickFolderDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickFolderDialogImplAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFolderDialogImpl)

#endif // QQUICKFOLDERDIALOGIMPL_P_H
