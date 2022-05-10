/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
#ifndef QTQUICKCONTROLS2FOREIGN_P_H
#define QTQUICKCONTROLS2FOREIGN_P_H

#include <QtQml/qqml.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquicksplitview_p.h>

QT_BEGIN_NAMESPACE

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

// These are necessary in order to use C++ types in a file where only QtQuick.Controls has been imported.
// Control types like Button don't need this done for them, as each style module provides a Button type,
// and QtQuick.Controls is a sort of alias for the active style import.

struct QQuickOverlayAttachedForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(Overlay)
    QML_FOREIGN(QQuickOverlay)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 3)
};

struct QQuickSplitHandleAttachedForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(SplitHandle)
    QML_FOREIGN(QQuickSplitHandleAttached)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 13)
};

QT_END_NAMESPACE

#endif // QTQUICKCONTROLS2FOREIGN_P_H
