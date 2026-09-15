// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEQUICKTABLEVIEW_H
#define QACCESSIBLEQUICKTABLEVIEW_H

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

#include "qaccessiblequickflickable_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class QQuickTableView;

class Q_QUICK_EXPORT QAccessibleQuickTableView : public QAccessibleQuickFlickable
{
public:
    QAccessibleQuickTableView(QQuickTableView *tableView);

    QAccessibleInterface *focusChild() const override;
};

#endif // accessibility

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKTABLEVIEW_H
