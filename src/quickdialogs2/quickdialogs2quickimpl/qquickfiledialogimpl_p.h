/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
    Q_PROPERTY(QUrl currentFile READ currentFile WRITE setCurrentFile NOTIFY currentFileChanged FINAL)
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

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &currentFolder);

    QUrl selectedFile() const;
    void setSelectedFile(const QUrl &file);

    QUrl currentFile() const;
    void setCurrentFile(const QUrl &currentFile);

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
    void selectedFileChanged();
    void currentFileChanged(const QUrl &currentFileUrl);
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
