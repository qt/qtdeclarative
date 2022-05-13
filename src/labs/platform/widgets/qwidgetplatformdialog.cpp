// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
