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

#include <functional>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickItem;

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickGeneratorAnimationProvider
{
public:
    virtual ~QQuickGeneratorAnimationProvider();

    virtual QQuickAbstractAnimation *enterTimelineScope(QQuickItem *item,
                                                        const StructureNodeInfo &info) = 0;
    virtual void enterTimelineScope(const QString &scopeId, const QString &referenceId) = 0;
    virtual void exitTimelineScope() = 0;

    virtual void bindProperty(QObject *target, const QByteArray &propertyName,
                              const QQuickAnimatedProperty &property) = 0;

    virtual QQuickItem *createCustomItem(const QString &type) = 0;

    // Set by the generator. Calls which have to be made on the GUI thread (such as completing
    // QML objects or starting animations) must be passed to deferToGuiThread().
    using DeferFunction = std::function<void(std::function<void()>)>;
    void setDeferFunction(DeferFunction deferFunction) { m_deferFunction = std::move(deferFunction); }

protected:
    void deferToGuiThread(std::function<void()> call)
    {
        if (m_deferFunction)
            m_deferFunction(std::move(call));
        else
            call();
    }

private:
    DeferFunction m_deferFunction;
};

QT_END_NAMESPACE

#endif // QQUICKGENERATORANIMATIONPROVIDER_P_H
