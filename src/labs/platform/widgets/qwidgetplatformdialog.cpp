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

#include "qwidgetplatformdialog_p.h"

#include <QtGui/qwindow.h>
#include <QtWidgets/qdialog.h>

QT_BEGIN_NAMESPACE

bool QWidgetPlatformDialog::show(QDialog *dialog, Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    dialog->setWindowFlags(flags);
    dialog->setWindowModality(modality);

    dialog->createWinId();
    QWindow *handle = dialog->windowHandle();
    Q_ASSERT(handle);
    handle->setTransientParent(const_cast<QWindow *>(parent));

    dialog->show();
    return true;
}

QT_END_NAMESPACE
