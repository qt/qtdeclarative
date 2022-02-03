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

#include "qquickdialogimplfactory_p.h"

#include <QtCore/qloggingcategory.h>

#include "qquickplatformfiledialog_p.h"
#include "qquickplatformfolderdialog_p.h"
#include "qquickplatformfontdialog_p.h"
#include "qquickplatformmessagedialog_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    Creates concrete QML-based dialogs.
*/

Q_LOGGING_CATEGORY(lcQuickDialogImplFactory, "qt.quick.dialogs.quickdialogimplfactory")

std::unique_ptr<QPlatformDialogHelper> QQuickDialogImplFactory::createPlatformDialogHelper(QQuickDialogType type, QObject *parent)
{
    std::unique_ptr<QPlatformDialogHelper> dialogHelper;
    switch (type) {
    case QQuickDialogType::FileDialog: {
        dialogHelper.reset(new QQuickPlatformFileDialog(parent));
        break;
    }
    case QQuickDialogType::FolderDialog: {
        dialogHelper.reset(new QQuickPlatformFolderDialog(parent));
        break;
    }
    case QQuickDialogType::FontDialog: {
        dialogHelper.reset(new QQuickPlatformFontDialog(parent));
        break;
    }
    case QQuickDialogType::MessageDialog: {
        dialogHelper.reset(new QQuickPlatformMessageDialog(parent));
        break;
    }
    default:
        break;
    }

    return dialogHelper;
}

QT_END_NAMESPACE
