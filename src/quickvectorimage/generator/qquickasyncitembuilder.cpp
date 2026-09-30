// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickasyncitembuilder_p.h"
#include "qquickitemgenerator_p.h"
#include "qquickvectorimagesource_p.h"
#include <private/qquickvectorimageplugin_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qthread.h>
#include <QtCore/qthreadpool.h>
#include <QtQuick/qquickitem.h>

#include <atomic>

QT_BEGIN_NAMESPACE

// State shared between the builder and the worker. The worker only posts to the receiver while
// holding the mutex, and the builder clears the receiver under the same mutex before it goes
// away, so the worker never posts to a deleted builder.
struct QQuickAsyncItemBuilderShared
{
    std::atomic<bool> cancelled = false;
    QMutex mutex;
    QObject *receiver = nullptr;
};

namespace {

QEvent::Type resultEventType()
{
    static const QEvent::Type type = QEvent::Type(QEvent::registerEventType());
    return type;
}

QEvent::Type initializeEventType()
{
    static const QEvent::Type type = QEvent::Type(QEvent::registerEventType());
    return type;
}

// Carries the generated tree from the worker to the builder as well as any additional calls to
// be executed on the main thread. If the event is never delivered (e.g. because the builder is
// cancelled or destroyed first), it disposes of the tree.
class QQuickAsyncItemBuilderResultEvent : public QEvent
{
public:
    QQuickAsyncItemBuilderResultEvent() : QEvent(resultEventType()) { }
    ~QQuickAsyncItemBuilderResultEvent() override
    {
        if (rootItem != nullptr)
            rootItem->deleteLater();
    }

    QQuickItem *rootItem = nullptr;
    QList<std::function<void()>> deferredCalls;
    QQuickVectorImageGenerator::ErrorState errorState = QQuickVectorImageGenerator::NoError;
};

// Runs on the global thread pool, which QCoreApplication waits for on destruction. A cancelled
// task finishes on its own and only shares QQuickAsyncItemBuilderShared with the builder.
class QQuickAsyncItemBuilderTask : public QRunnable
{
public:
    QQuickAsyncItemBuilderTask(const QQuickVectorImageSource &source,
                               QQuickVectorImageGenerator::GeneratorFlags flags,
                               QQmlContext *context,
                               QThread *targetThread,
                               std::vector<std::unique_ptr<QQuickVectorImagePluginGenerator>> pluginGenerators,
                               std::shared_ptr<QQuickAsyncItemBuilderShared> shared)
        : m_source(source)
        , m_flags(flags)
        , m_context(context)
        , m_targetThread(targetThread)
        , m_pluginGenerators(std::move(pluginGenerators))
        , m_shared(std::move(shared))
    {
    }

    void run() override
    {
        auto *result = new QQuickAsyncItemBuilderResultEvent;

        {
            // The context is only stored by the generator here. It is used by the deferred calls,
            // which are executed on the GUI thread.
            QQuickItemGenerator generator(m_source, m_flags, m_context);
            generator.setDeferGuiThreadCalls(true);
            generator.setCancellationFlag(&m_shared->cancelled);

            bool generatedWithPlugin = false;
            for (const auto &pluginGenerator : std::as_const(m_pluginGenerators)) {
                if (isCancelled())
                    break;
                if ((generatedWithPlugin = pluginGenerator->generate(&generator)))
                    break;
            }

            if (!generatedWithPlugin && !isCancelled())
                generator.generate();

            result->errorState = isCancelled() ? QQuickVectorImageGenerator::Cancelled
                                               : generator.errorState();

            if (result->errorState == QQuickVectorImageGenerator::NoError) {
                result->rootItem = generator.takeRootItem();
                result->deferredCalls = generator.takeDeferredCalls();
                if (result->rootItem != nullptr)
                    result->rootItem->moveToThread(m_targetThread);
            }
        }

        QMutexLocker locker(&m_shared->mutex);
        if (m_shared->receiver != nullptr) {
            QCoreApplication::postEvent(m_shared->receiver, result);
        } else {
            locker.unlock();
            delete result;
        }
    }

private:
    bool isCancelled() const { return m_shared->cancelled.load(std::memory_order_relaxed); }

    QQuickVectorImageSource m_source;
    QQuickVectorImageGenerator::GeneratorFlags m_flags;
    QQmlContext *m_context;
    QThread *m_targetThread;
    std::vector<std::unique_ptr<QQuickVectorImagePluginGenerator>> m_pluginGenerators;
    std::shared_ptr<QQuickAsyncItemBuilderShared> m_shared;
};

int initializationBudgetMs()
{
    static const int val = int(qEnvironmentVariableIntegerValue("QT_QUICKVECTORIMAGE_ASYNC_BUDGET_MS")
                                       .value_or(5));
    return val;
}

} // namespace

QQuickAsyncItemBuilder::QQuickAsyncItemBuilder(const QQuickVectorImageSource &source,
                                               QQuickVectorImageGenerator::GeneratorFlags flags,
                                               QQmlContext *context,
                                               QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_flags(flags)
    , m_context(context)
    , m_shared(std::make_shared<QQuickAsyncItemBuilderShared>())
{
}

QQuickAsyncItemBuilder::~QQuickAsyncItemBuilder()
{
    cancel();
    delete m_rootItem;
}

void QQuickAsyncItemBuilder::start(std::vector<std::unique_ptr<QQuickVectorImagePluginGenerator>> pluginGenerators)
{
    Q_ASSERT(m_state == State::Idle);
    m_state = State::Generating;

    {
        QMutexLocker locker(&m_shared->mutex);
        m_shared->receiver = this;
    }

    QThreadPool::globalInstance()->start(new QQuickAsyncItemBuilderTask(m_source,
                                                                        m_flags,
                                                                        m_context,
                                                                        thread(),
                                                                        std::move(pluginGenerators),
                                                                        m_shared));
}

// Stops generation without waiting for the worker thread. The worker notices the cancellation
// the next time it enters the generator and stops descending into the document; the partial tree
// is then destroyed on the worker. If the tree has already been handed over, it is destroyed here.
void QQuickAsyncItemBuilder::cancel()
{
    if (!isRunning())
        return;

    m_state = State::Cancelled;
    m_errorState = QQuickVectorImageGenerator::Cancelled;
    detach();

    m_deferredCalls.clear();
    delete m_rootItem;
    m_rootItem = nullptr;
}

void QQuickAsyncItemBuilder::detach()
{
    m_shared->cancelled = true;
    {
        QMutexLocker locker(&m_shared->mutex);
        m_shared->receiver = nullptr;
    }

    // Deletes a pending result event, and with it the tree it carries
    QCoreApplication::removePostedEvents(this, resultEventType());
    QCoreApplication::removePostedEvents(this, initializeEventType());
}

QQuickItem *QQuickAsyncItemBuilder::takeRootItem()
{
    return std::exchange(m_rootItem, nullptr);
}

bool QQuickAsyncItemBuilder::event(QEvent *e)
{
    if (e->type() == resultEventType()) {
        auto *result = static_cast<QQuickAsyncItemBuilderResultEvent *>(e);
        if (m_state != State::Generating)
            return true;

        if (result->errorState != QQuickVectorImageGenerator::NoError
            || result->rootItem == nullptr) {
            finish(result->errorState != QQuickVectorImageGenerator::NoError
                           ? result->errorState
                           : QQuickVectorImageGenerator::LoadFailed);
            return true;
        }

        Q_ASSERT(result->rootItem->thread() == thread());
        m_rootItem = std::exchange(result->rootItem, nullptr);
        m_deferredCalls = std::move(result->deferredCalls);
        m_nextDeferredCall = 0;
        m_state = State::Initializing;
        initializeItems();
        return true;
    }

    if (e->type() == initializeEventType()) {
        initializeItems();
        return true;
    }

    return QObject::event(e);
}

// Executes the deferred calls until they are done. Time slices in case certain deferred calls
// are expensive.
void QQuickAsyncItemBuilder::initializeItems()
{
    if (m_state != State::Initializing)
        return;

    QDeadlineTimer deadline(initializationBudgetMs(), Qt::PreciseTimer);
    while (m_nextDeferredCall < m_deferredCalls.size()) {
        // Release the call (and anything it has captured) once it has been executed
        std::function<void()> call = std::exchange(m_deferredCalls[m_nextDeferredCall++], {});
        call();

        if (m_state != State::Initializing) // Cancelled from within the call
            return;

        if (deadline.hasExpired() && m_nextDeferredCall < m_deferredCalls.size()) {
            QCoreApplication::postEvent(this, new QEvent(initializeEventType()));
            return;
        }
    }

    m_deferredCalls.clear();
    finish(QQuickVectorImageGenerator::NoError);
}

void QQuickAsyncItemBuilder::finish(QQuickVectorImageGenerator::ErrorState errorState)
{
    Q_ASSERT(isRunning());

    detach();
    m_errorState = errorState;
    if (errorState != QQuickVectorImageGenerator::NoError) {
        qCWarning(lcQuickVectorImage) << "QQuickAsyncItemBuilder: failed to generate" << m_source
                                      << "(errorState:" << errorState << ")";
        delete m_rootItem;
        m_rootItem = nullptr;
    }

    m_state = State::Finished;
    emit finished();
}

QT_END_NAMESPACE
