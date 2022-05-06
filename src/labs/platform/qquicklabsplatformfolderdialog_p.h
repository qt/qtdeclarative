/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#ifndef QQUICKLABSPLATFORMFOLDERDIALOG_P_H
#define QQUICKLABSPLATFORMFOLDERDIALOG_P_H

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

#include "qquicklabsplatformdialog_p.h"
#include <QtCore/qurl.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformFolderDialog : public QQuickLabsPlatformDialog
{
    Q_OBJECT
    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged FINAL)
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QFileDialogOptions::FileDialogOptions options READ options WRITE setOptions RESET resetOptions NOTIFY optionsChanged FINAL)
    Q_PROPERTY(QString acceptLabel READ acceptLabel WRITE setAcceptLabel RESET resetAcceptLabel NOTIFY acceptLabelChanged FINAL)
    Q_PROPERTY(QString rejectLabel READ rejectLabel WRITE setRejectLabel RESET resetRejectLabel NOTIFY rejectLabelChanged FINAL)
    Q_FLAGS(QFileDialogOptions::FileDialogOptions)

public:
    explicit QQuickLabsPlatformFolderDialog(QObject *parent = nullptr);

    QUrl folder() const;
    void setFolder(const QUrl &folder);

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &folder);

    QFileDialogOptions::FileDialogOptions options() const;
    void setOptions(QFileDialogOptions::FileDialogOptions options);
    void resetOptions();

    QString acceptLabel() const;
    void setAcceptLabel(const QString &label);
    void resetAcceptLabel();

    QString rejectLabel() const;
    void setRejectLabel(const QString &label);
    void resetRejectLabel();

Q_SIGNALS:
    void folderChanged();
    void currentFolderChanged();
    void optionsChanged();
    void acceptLabelChanged();
    void rejectLabelChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QUrl m_folder;
    QSharedPointer<QFileDialogOptions> m_options;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickLabsPlatformFolderDialog)

#endif // QQUICKLABSPLATFORMFOLDERDIALOG_P_H
