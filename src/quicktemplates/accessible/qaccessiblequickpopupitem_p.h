// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBLEQUICKPOPUPITEM_H
#define QACCESSIBLEQUICKPOPUPITEM_H

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

#include <QtQuick/private/qaccessiblequickitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickPopupItem;

class QAccessibleQuickPopupItem : public QAccessibleQuickItem
{
public:
    QAccessibleQuickPopupItem(QQuickPopupItem *popupItem);
    QAccessible::State state() const override;
};

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKPOPUPITEM_H
