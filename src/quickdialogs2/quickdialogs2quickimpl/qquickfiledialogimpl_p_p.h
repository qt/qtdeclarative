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

#ifndef QQUICKFILEDIALOG_P_P_H
#define QQUICKFILEDIALOG_P_P_H

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

#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>

#include "qquickfiledialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickFileNameFilter;

class QQuickFileDialogImplPrivate : public QQuickDialogPrivate
{
    Q_DECLARE_PUBLIC(QQuickFileDialogImpl)

public:
    QQuickFileDialogImplPrivate();

    static QQuickFileDialogImplPrivate *get(QQuickFileDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickFileDialogImplAttached *attachedOrWarn();

    void setNameFilters(const QStringList &filters);

    void updateEnabled();
    void updateCurrentFile(const QString &oldFolderPath);

    void handleAccept() override;
    void handleClick(QQuickAbstractButton *button) override;

    QSharedPointer<QFileDialogOptions> options;
    QUrl currentFolder;
    QUrl selectedFile;
    QUrl currentFile;
    QStringList nameFilters;
    mutable QQuickFileNameFilter *selectedNameFilter = nullptr;
    QString acceptLabel;
    QString rejectLabel;
};

class QQuickFileDialogImplAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickFileDialogImplAttached)

    void nameFiltersComboBoxItemActivated(int index);
    void fileDialogListViewCurrentIndexChanged();

public:
    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickComboBox> nameFiltersComboBox;
    QPointer<QQuickListView> fileDialogListView;
    QPointer<QQuickFolderBreadcrumbBar> breadcrumbBar;
};

QT_END_NAMESPACE

#endif // QQUICKFILEDIALOG_P_P_H
