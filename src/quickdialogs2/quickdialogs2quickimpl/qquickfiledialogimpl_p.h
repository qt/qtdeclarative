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

#ifndef QQUICKFILEDIALOGIMPL_P_H
#define QQUICKFILEDIALOGIMPL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickComboBox;
class QQuickDialogButtonBox;

class QQuickFileDialogImplAttached;
class QQuickFileDialogImplAttachedPrivate;
class QQuickFileDialogImplPrivate;
class QQuickFileNameFilter;
class QQuickFolderBreadcrumbBar;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFileDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QUrl selectedFile READ selectedFile WRITE setSelectedFile NOTIFY selectedFileChanged FINAL)
    Q_PROPERTY(QStringList nameFilters READ nameFilters NOTIFY nameFiltersChanged FINAL)
    Q_PROPERTY(QQuickFileNameFilter *selectedNameFilter READ selectedNameFilter CONSTANT)
    QML_NAMED_ELEMENT(FileDialogImpl)
    QML_ATTACHED(QQuickFileDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 2)
    Q_MOC_INCLUDE(<QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>)
    Q_MOC_INCLUDE(<QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p.h>)

public:
    explicit QQuickFileDialogImpl(QObject *parent = nullptr);

    static QQuickFileDialogImplAttached *qmlAttachedProperties(QObject *object);

    enum class SetReason {
        // Either user interaction or e.g. a change in ListView's currentIndex after changing its model.
        External,
        // As a result of the user setting an initial selectedFile.
        Internal
    };

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &currentFolder, SetReason setReason = SetReason::External);

    QUrl selectedFile() const;
    void setSelectedFile(const QUrl &file);
    void setInitialCurrentFolderAndSelectedFile(const QUrl &file);

    QSharedPointer<QFileDialogOptions> options() const;
    void setOptions(const QSharedPointer<QFileDialogOptions> &options);

    QStringList nameFilters() const;
    void resetNameFilters();

    QQuickFileNameFilter *selectedNameFilter() const;

    void setAcceptLabel(const QString &label);
    void setRejectLabel(const QString &label);

public Q_SLOTS:
    void selectNameFilter(const QString &filter);

Q_SIGNALS:
    void currentFolderChanged(const QUrl &folderUrl);
    void selectedFileChanged(const QUrl &selectedFileUrl);
    void nameFiltersChanged();
    void fileSelected(const QUrl &fileUrl);
    void filterSelected(const QString &filter);

private:
    void componentComplete() override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data) override;

    Q_DISABLE_COPY(QQuickFileDialogImpl)
    Q_DECLARE_PRIVATE(QQuickFileDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFileDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialogButtonBox *buttonBox READ buttonBox WRITE setButtonBox NOTIFY buttonBoxChanged FINAL)
    Q_PROPERTY(QQuickComboBox *nameFiltersComboBox READ nameFiltersComboBox WRITE setNameFiltersComboBox NOTIFY nameFiltersComboBoxChanged)
    Q_PROPERTY(QQuickListView *fileDialogListView READ fileDialogListView WRITE setFileDialogListView NOTIFY fileDialogListViewChanged)
    Q_PROPERTY(QQuickFolderBreadcrumbBar *breadcrumbBar READ breadcrumbBar WRITE setBreadcrumbBar NOTIFY breadcrumbBarChanged)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickcombobox_p.h>)

public:
    explicit QQuickFileDialogImplAttached(QObject *parent = nullptr);

    QQuickDialogButtonBox *buttonBox() const;
    void setButtonBox(QQuickDialogButtonBox *buttonBox);

    QQuickComboBox *nameFiltersComboBox() const;
    void setNameFiltersComboBox(QQuickComboBox *nameFiltersComboBox);

    QString selectedNameFilter() const;
    void selectNameFilter(const QString &filter);

    QQuickListView *fileDialogListView() const;
    void setFileDialogListView(QQuickListView *fileDialogListView);

    QQuickFolderBreadcrumbBar *breadcrumbBar() const;
    void setBreadcrumbBar(QQuickFolderBreadcrumbBar *breadcrumbBar);

Q_SIGNALS:
    void buttonBoxChanged();
    void nameFiltersComboBoxChanged();
    void fileDialogListViewChanged();
    void breadcrumbBarChanged();

private:
    Q_DISABLE_COPY(QQuickFileDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickFileDialogImplAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFileDialogImpl)

#endif // QQUICKFILEDIALOGIMPL_P_H
