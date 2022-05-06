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

#include "qquickdialogimplfactory_p.h"

#include <QtCore/qloggingcategory.h>

#include "qquickplatformfiledialog_p.h"
#include "qquickplatformfontdialog_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    Creates concrete QML-based dialogs.
*/

Q_LOGGING_CATEGORY(lcQuickDialogImplFactory, "qt.quick.dialogs.quickdialogimplfactory")

QPlatformDialogHelper *QQuickDialogImplFactory::createPlatformDialogHelper(
    QPlatformTheme::DialogType type, QObject *parent)
{
    switch (type) {
    case QPlatformTheme::FileDialog: {
        auto dialog = new QQuickPlatformFileDialog(parent);
        // If the QML file failed to load, we need to handle it gracefully.
        if (!dialog->isValid()) {
            delete dialog;
            return nullptr;
        }

        return dialog;
    }
    case QPlatformTheme::FontDialog: {
        auto dialog = new QQuickPlatformFontDialog(parent);

        if (!dialog->isValid()) {
            delete dialog;
            return nullptr;
        }
        return dialog;
    }
    default:
        break;
    }

    return nullptr;
}

QT_END_NAMESPACE
