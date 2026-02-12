// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprocessscheduler_p.h"
#include <QtQmlDom/private/qqmldom_utils_p.h>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

Q_STATIC_LOGGING_CATEGORY(schedulerLog, "qt.languageserver.qprocessscheduler")

using namespace Qt::StringLiterals;

/*!
  \internal
  \class QProcessScheduler

  \brief Runs multiple processes sequentially via a QProcess, and signals once they are done.

  QProcessScheduler runs multiple programs sequentially through a QProcess, and emits the \c done
  signal once all programs finished. Use schedule(programs, id) to add new programs to be run. The
  id is used to signal their completion. Duplicate programs that already are in the queue are not
  re-added to the queue.
 */
QProcessScheduler::QProcessScheduler()
{
    QObject::connect(&m_process, &QProcess::finished, this, &QProcessScheduler::processNext);
    QObject::connect(&m_process, &QProcess::errorOccurred, this,
                     &QProcessScheduler::onErrorOccurred);
}
QProcessScheduler::~QProcessScheduler()
{
    QObject::disconnect(&m_process, nullptr);
    m_process.kill();
    m_process.waitForFinished();
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized") // use of std::variant
void QProcessScheduler::schedule(const QList<Command> &list, const Id &id)
{
    for (const auto &x : list) {
        const QueueElement queueElement{ x };
        if (!m_queue.contains(queueElement))
            m_queue.enqueue(queueElement);
    }
    m_queue.enqueue(id);
    if (!m_isRunning)
        processNext();
}
QT_WARNING_POP

void QProcessScheduler::processNext()
{
    m_isRunning = m_queue.size() > 0;
    if (!m_isRunning)
        return;

    std::visit(qOverloadedVisitor{ [this](const Id &id) {
                                      emit done(id);
                                      processNext();
                                  },
                                   [this](const Command &command) {
                                       m_process.setProgram(command.program);
                                       m_process.setArguments(command.arguments);
                                       m_process.setProcessEnvironment(command.customEnvironment);
                                       m_process.start();
                                   }

               },
               m_queue.dequeue());
}

void QProcessScheduler::onErrorOccurred(QProcess::ProcessError error)
{
    qCDebug(schedulerLog) << "Process" << m_process.program() << m_process.arguments().join(" "_L1)
                          << "had an error:" << error;
    processNext();
}

} // namespace QmlLsp

QT_END_NAMESPACE
