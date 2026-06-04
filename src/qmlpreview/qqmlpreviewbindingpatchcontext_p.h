// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QQMLPREVIEWBINDINGPATCHCONTEXT_P_H
#define QQMLPREVIEWBINDINGPATCHCONTEXT_P_H

#include <private/qduplicatetracker_p.h>
#include <private/qqmlanybinding_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlpreviewdiff_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <unordered_map>

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

QT_BEGIN_NAMESPACE

namespace QQmlPreview {

struct CompositeLevel
{
    QQmlRefPointer<QV4::ExecutableCompilationUnit> oldCu;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> newCu;
    int objectIndex = -1;
    QString icName;

    QQmlRefPointer<QQmlContextData> context;
};

class BindingPatchContext
{
public:
    BindingPatchContext(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                        int objectIndex, const QString &prefix)
        : m_object(object), unit(unit), objectIndex(objectIndex), prefix(prefix + QLatin1Char('.'))
    {
    }

    BindingPatchContext(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                        int objectIndex)
        : m_object(object), unit(unit), objectIndex(objectIndex)
    {
    }

    void reset();
    void stashExternalState(
            const std::vector<CompositeLevel> &internalUnits,
            QDuplicateTracker<QObject *> *seenChildren);
    void refreshObjects();
    void restoreExternalState();

    BindingPatchContext *childContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                      const QV4::CompiledData::Binding *binding,
                                      QDuplicateTracker<QObject *> *seenChildren);
    BindingPatchContext *childContext(const QString &name, QObject *object,
                                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                      int objectIndex, QDuplicateTracker<QObject *> *seenChildren);
    BindingPatchContext *attachedContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                         const QV4::CompiledData::Binding *binding,
                                         QDuplicateTracker<QObject *> *seenChildren);

private:
    struct StoredBinding
    {
        QString propertyName;
        QQmlAnyBinding binding;
    };

    struct StoredValue
    {
        QString propertyName;
        QVariant value;
    };

    struct StoredSignalHandler
    {
        QString signature;
        std::unique_ptr<QQmlBoundSignal> handler;
    };

    void recordBindingValues(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                             int cuIndex, QHash<QString, QVariant> *constantValues,
                             QDuplicateTracker<QObject *> *seenChildren);
    void resetBinding(const QV4::CompiledData::Binding *binding, const QString &name,
                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit);
    void resetBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit, int cuIndex);

    QObject *m_object = nullptr;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> unit;
    int objectIndex;
    QString prefix;

    std::vector<StoredBinding> m_storedBindings;
    std::vector<StoredValue> m_storedValues;
    std::vector<StoredSignalHandler> m_storedSignalHandlers;
    std::unordered_map<QString, std::unique_ptr<BindingPatchContext>> m_children;
};

} // namespace QQmlPreview

QT_END_NAMESPACE

#endif
