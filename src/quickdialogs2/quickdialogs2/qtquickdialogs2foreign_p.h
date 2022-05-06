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

#ifndef QTQUICKDIALOGS2FOREIGN_P_H
#define QTQUICKDIALOGS2FOREIGN_P_H

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
#include <QtQml/qqml.h>
#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>

QT_BEGIN_NAMESPACE

struct QPlatformDialogHelperForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QPlatformDialogHelper)
    QML_ADDED_IN_VERSION(6, 2)
};

struct QQuickFileNameFilterQuickDialogs2Foreign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickFileNameFilter)
    QML_ADDED_IN_VERSION(6, 2)
};

QT_END_NAMESPACE

#endif // QTQUICKDIALOGS2FOREIGN_P_H
