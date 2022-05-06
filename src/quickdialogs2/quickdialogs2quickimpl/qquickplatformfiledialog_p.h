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

#ifndef QQUICKPLATFORMFILEDIALOG_P_H
#define QQUICKPLATFORMFILEDIALOG_P_H

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

#include <QtGui/qpa/qplatformdialoghelper.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickFileDialogImpl;
class QWindow;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickPlatformFileDialog : public QPlatformFileDialogHelper
{
    Q_OBJECT

public:
    explicit QQuickPlatformFileDialog(QObject *parent);
    ~QQuickPlatformFileDialog() = default;

    bool isValid() const;
    bool defaultNameFilterDisables() const override;
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &file) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;

    void exec() override;
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void hide() override;

    QQuickFileDialogImpl *dialog() const;

private:
    QQuickFileDialogImpl *m_dialog = nullptr;
    QString m_pendingNameFilter;
};

QT_END_NAMESPACE

#endif // QQUICKPLATFORMFILEDIALOG_P_H
