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

#ifndef QWIDGETPLATFORMFONTDIALOG_P_H
#define QWIDGETPLATFORMFONTDIALOG_P_H

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

QT_BEGIN_NAMESPACE

class QFontDialog;

class QWidgetPlatformFontDialog : public QPlatformFontDialogHelper
{
    Q_OBJECT

public:
    explicit QWidgetPlatformFontDialog(QObject *parent = nullptr);
    ~QWidgetPlatformFontDialog();

    QFont currentFont() const override;
    void setCurrentFont(const QFont &font) override;

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

private:
    QScopedPointer<QFontDialog> m_dialog;
};

QT_END_NAMESPACE

#endif // QWIDGETPLATFORMFONTDIALOG_P_H
