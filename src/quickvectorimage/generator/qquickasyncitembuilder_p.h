// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKASYNCITEMBUILDER_P_H
#define QQUICKASYNCITEMBUILDER_P_H

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

#include <QtQuickVectorImageGenerator/qtquickvectorimagegeneratorexports.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimagesource_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qobject.h>

#include <functional>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQuickItem;
class QQuickVectorImagePluginGenerator;
struct QQuickAsyncItemBuilderShared;

// Builds the item tree for a vector image on a worker thread using QQuickItemGenerator. Calls
// which have to be made on the GUI thread are deferred by the generator. When the tree is done,
// it is moved to the GUI thread, where the deferred calls are executed in time-limited slices,
// so that input and rendering are not starved.
class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickAsyncItemBuilder : public QObject
{
    Q_OBJECT
public:
    enum class State { Idle, Generating, Initializing, Finished, Cancelled };

    QQuickAsyncItemBuilder(const QQuickVectorImageSource &source,
                           QQuickVectorImageGenerator::GeneratorFlags flags,
                           QQmlContext *context,
                           QObject *parent = nullptr);
    ~QQuickAsyncItemBuilder() override;

    // Plugin generators must be created on the GUI thread. They are tried before the default
    // generator, the same way as in the synchronous case.
    void start(std::vector<std::unique_ptr<QQuickVectorImagePluginGenerator>> pluginGenerators);
    void cancel();

    State state() const { return m_state; }
    bool isRunning() const { return m_state == State::Generating || m_state == State::Initializing; }
    QQuickVectorImageGenerator::ErrorState errorState() const { return m_errorState; }
    QQuickItem *takeRootItem();

Q_SIGNALS:
    // Emitted when generation has completed, successfully or not. Not emitted when cancelled.
    void finished();

protected:
    bool event(QEvent *e) override;

private:
    void initializeItems();
    void finish(QQuickVectorImageGenerator::ErrorState errorState);
    void detach();

    QQuickVectorImageSource m_source;
    QQuickVectorImageGenerator::GeneratorFlags m_flags;
    QQmlContext *m_context = nullptr;
    std::shared_ptr<QQuickAsyncItemBuilderShared> m_shared;
    QQuickItem *m_rootItem = nullptr;
    QList<std::function<void()>> m_deferredCalls;
    qsizetype m_nextDeferredCall = 0;
    State m_state = State::Idle;
    QQuickVectorImageGenerator::ErrorState m_errorState = QQuickVectorImageGenerator::NoError;
};

QT_END_NAMESPACE

#endif // QQUICKASYNCITEMBUILDER_P_H
