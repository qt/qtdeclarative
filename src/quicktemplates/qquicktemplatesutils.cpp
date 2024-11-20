// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktemplatesutils_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>

QT_BEGIN_NAMESPACE

namespace QQuickTemplatesUtils {

/*!
    \internal

    Returns \c true if \a item is a \c QQuickControl or similar interactive
    type that should be a QQuickControl-subclass but cannot due to existing
    inheritance; e.g. \c QQuickTextField or \c QQuickTextArea.

    \c QQuickPopup is not considered a controls type for this function due to
    the original use cases that resulted in this being factored out.

    \c QQuickLabel is not interactive.
*/
bool isInteractiveControlType(const QQuickItem *item)
{
    return qobject_cast<const QQuickControl *>(item)
        || qobject_cast<const QQuickTextField *>(item)
        || qobject_cast<const QQuickTextArea *>(item);
}

} // namespace QQuickTemplatesUtils

QT_END_NAMESPACE
