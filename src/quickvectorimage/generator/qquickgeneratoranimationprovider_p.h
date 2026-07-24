// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKGENERATORANIMATIONPROVIDER_P_H
#define QQUICKGENERATORANIMATIONPROVIDER_P_H

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

#include "qquickanimatedproperty_p.h"
#include "qquicknodeinfo_p.h"

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickItem;

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickGeneratorAnimationProvider
{
public:
    virtual ~QQuickGeneratorAnimationProvider();

    virtual QQuickAbstractAnimation *enterTimelineScope(QQuickItem *item,
                                                        const TimelineInfo &info) = 0;
    virtual void exitTimelineScope() = 0;

    virtual void bindProperty(QObject *target, const QByteArray &property,
                              const QQuickAnimatedProperty::PropertyAnimation &anim) = 0;

    virtual QQuickItem *createCustomItem(const QString &type) = 0;
};

QT_END_NAMESPACE

#endif // QQUICKGENERATORANIMATIONPROVIDER_P_H
