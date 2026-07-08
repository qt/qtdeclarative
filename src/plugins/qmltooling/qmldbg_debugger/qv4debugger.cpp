// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4debugger.h"
#include "qv4debugjob.h"
#include "qv4datacollector.h"

#include <private/qqmlcontext_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4function_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4script_p.h>
#include <private/qv4stackframe_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

QV4Debugger::BreakPoint::BreakPoint(const QString &fileName, int line)
    : fileName(fileName), lineNumber(line)
{}

inline size_t qHash(const QV4Debugger::BreakPoint &b, size_t seed = 0) noexcept
{
    return qHash(b.fileName, seed) ^ b.lineNumber;
}

inline bool operator==(const QV4Debugger::BreakPoint &a,
                       const QV4Debugger::BreakPoint &b)
{
    return a.lineNumber == b.lineNumber && a.fileName == b.fileName;
}

QV4Debugger::QV4Debugger(QV4::ExecutionEngine *engine)
    : m_engine(engine)
    , m_state(Running)
    , m_stepping(NotStepping)
    , m_pauseRequested(false)
    , m_haveBreakPoints(false)
    , m_breakOnThrow(false)
    , m_returnedValue(engine, QV4::Value::undefinedValue())
    , m_gatherSources(nullptr)
    , m_runningJob(nullptr)
    , m_collector(engine)
{
    static int debuggerId = qRegisterMetaType<QV4Debugger*>();
    static int pauseReasonId = qRegisterMetaType<QV4Debugger::PauseReason>();
    Q_UNUSED(debuggerId);
    Q_UNUSED(pauseReasonId);
    connect(this, &QV4Debugger::scheduleJob,
            this, &QV4Debugger::runJobUnpaused, Qt::QueuedConnection);
}

QV4::ExecutionEngine *QV4Debugger::engine() const
{
    return m_engine;
}

const QV4DataCollector *QV4Debugger::collector() const
{
    return &m_collector;
}

QV4DataCollector *QV4Debugger::collector()
{
    return &m_collector;
}

void QV4Debugger::pause()
{
    QMutexLocker locker(&m_lock);
    if (m_state == Paused)
        return;
    m_pauseRequested = true;
}

void QV4Debugger::resume(Speed speed)
{
    QMutexLocker locker(&m_lock);
    if (m_state != Paused)
        return;

    if (!m_returnedValue.isUndefined())
        m_returnedValue.set(m_engine, QV4::Encode::undefined());

    m_currentFrame = m_engine->currentStackFrame;
    m_stepping = speed;
    m_runningCondition.wakeAll();
}

QV4Debugger::State QV4Debugger::state() const
{
    return m_state;
}

void QV4Debugger::addBreakPoint(const QString &fileName, int lineNumber, const QString &condition)
{
    QMutexLocker locker(&m_lock);
    m_breakPoints.insert(BreakPoint(fileName.mid(fileName.lastIndexOf('/') + 1),
                                            lineNumber), {fileName, condition});
    m_haveBreakPoints = true;
    m_breakPointsChanged = true;
}

void QV4Debugger::removeBreakPoint(const QString &fileName, int lineNumber)
{
    QMutexLocker locker(&m_lock);
    m_breakPoints.remove(BreakPoint(fileName.mid(fileName.lastIndexOf('/') + 1),
                                            lineNumber));
    m_haveBreakPoints = !m_breakPoints.isEmpty();
    m_breakPointsChanged = true;
}

void QV4Debugger::setBreakOnThrow(bool onoff)
{
    QMutexLocker locker(&m_lock);

    m_breakOnThrow = onoff;
}

void QV4Debugger::clearPauseRequest()
{
    QMutexLocker locker(&m_lock);
    m_pauseRequested = false;
}

qsizetype QV4Debugger::executableLinesCacheSize()
{
    QMutexLocker locker(&m_lock);
    return m_executableLines.size();
}

QV4Debugger::ExecutionState QV4Debugger::currentExecutionState() const
{
    ExecutionState state;
    state.fileName = QUrl(getFunction()->sourceFile()).fileName();
    state.lineNumber = engine()->currentStackFrame->lineNumber();

    return state;
}

bool QV4Debugger::pauseAtNextOpportunity() const {
    // A change of breakpoints has to reach maybeBreakAtInstruction() even when
    // none are left, so that the cache of executable lines gets pruned.
    return m_pauseRequested || m_haveBreakPoints || m_breakPointsChanged || m_gatherSources
            || m_stepping >= StepOver;
}

QList<QV4::StackFrame> QV4Debugger::stackTrace(int frameLimit) const
{
    return m_engine->stackTrace(frameLimit);
}

void QV4Debugger::maybeBreakAtInstruction()
{
    if (m_runningJob) // do not re-enter when we're doing a job for the debugger.
        return;

    QMutexLocker locker(&m_lock);

    if (m_gatherSources) {
        m_gatherSources->run();
        delete m_gatherSources;
        m_gatherSources = nullptr;
    }

    if (m_breakPointsChanged) {
        m_breakPointsChanged = false;
        pruneExecutableLines();
    }

    switch (m_stepping) {
    case StepOver:
        if (m_currentFrame != m_engine->currentStackFrame)
            break;
        Q_FALLTHROUGH();
    case StepIn:
        pauseAndWait(Step);
        return;
    case StepOut:
    case NotStepping:
        break;
    }

    if (m_pauseRequested) { // Serve debugging requests from the agent
        m_pauseRequested = false;
        pauseAndWait(PauseRequest);
    } else if (m_haveBreakPoints) {
        if (QV4::Function *f = getFunction()) {
            // lineNumber will be negative for Ret instructions, so those won't match
            const int lineNumber = engine()->currentStackFrame->lineNumber();
            if (reallyHitTheBreakPoint(f, f->sourceFile(), lineNumber))
                pauseAndWait(BreakPointHit);
        }
    }
}

void QV4Debugger::enteringFunction()
{
    if (m_runningJob)
        return;
    QMutexLocker locker(&m_lock);

    if (m_stepping == StepIn)
        m_currentFrame = m_engine->currentStackFrame;
}

void QV4Debugger::leavingFunction(const QV4::ReturnedValue &retVal)
{
    if (m_runningJob)
        return;
    Q_UNUSED(retVal); // TODO

    QMutexLocker locker(&m_lock);

    if (m_stepping != NotStepping && m_currentFrame == m_engine->currentStackFrame) {
        m_currentFrame = m_currentFrame->parentFrame();
        m_stepping = StepOver;
        m_returnedValue.set(m_engine, retVal);
    }
}

void QV4Debugger::aboutToThrow()
{
    if (!m_breakOnThrow)
        return;

    if (m_runningJob) // do not re-enter when we're doing a job for the debugger.
        return;

    QMutexLocker locker(&m_lock);
    pauseAndWait(Throwing);
}

QV4::Function *QV4Debugger::getFunction() const
{
    if (m_engine->currentStackFrame)
        return m_engine->currentStackFrame->v4Function;
    else
        return m_engine->globalCode;
}

void QV4Debugger::runJobUnpaused()
{
    QMutexLocker locker(&m_lock);
    if (m_runningJob)
        m_runningJob->run();
    m_jobIsRunning.wakeAll();
}

void QV4Debugger::pauseAndWait(PauseReason reason)
{
    if (m_runningJob)
        return;

    m_state = Paused;
    emit debuggerPaused(this, reason);

    while (true) {
        m_runningCondition.wait(&m_lock);
        if (m_runningJob) {
            m_runningJob->run();
            m_jobIsRunning.wakeAll();
        } else {
            break;
        }
    }

    m_state = Running;
}

/*!
    \internal

    Collects the executable lines of every function in \a unit, i.e. of the
    whole source file. Resolution has to be file-global: a breakpoint on a
    comment inside a nested function must not also match the outer function's
    first line after the comment.
*/
QV4Debugger::ExecutableLines QV4Debugger::collectExecutableLines(
        const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit)
{
    ExecutableLines result;
    const QV4::CompiledData::Unit *data = unit ? unit->unitData() : nullptr;
    if (!data)
        return result;
    for (uint f = 0, fn = data->functionTableSize; f < fn; ++f) {
        const QV4::CompiledData::Function *cf = data->functionAt(f);
        const int start = cf->location.line();
        result.functionStarts.append(start);
        const QV4::CompiledData::CodeOffsetToLineAndStatement *table
                = cf->lineAndStatementNumberTable();
        for (uint i = 0, n = cf->nLineAndStatementNumbers; i < n; ++i) {
            const int line = table[i].line;
            if (line > 0) // debug instructions get negative line numbers
                result.lines.append({line, start});
        }
    }
    std::sort(result.lines.begin(), result.lines.end());
    result.lines.erase(std::unique(result.lines.begin(), result.lines.end()),
                       result.lines.end());
    std::sort(result.functionStarts.begin(), result.functionStarts.end());
    return result;
}

int QV4Debugger::firstLineAtOrAfter(const ExecutableLines &executable, int requestedLine)
{
    // Lines are sorted by line, then by the start of their function, so the
    // first one at or after the request is from the earliest starting function.
    const auto it = std::lower_bound(executable.lines.cbegin(), executable.lines.cend(),
                                     ExecutableLine{requestedLine, 0});
    if (it == executable.lines.cend())
        return -1;
    // Only move within code that starts at or before the requested line, and
    // not past the start of another function or binding. A function starting
    // later runs at an unrelated time, and in QML the requested line may be a
    // declaration without code, such as an id or a literal binding. A unit may
    // also cover only a fragment of its file (a binding compiled from a
    // string, a chunk passed to QJSEngine::evaluate() with a line offset), and
    // a line before its code belongs to some other unit.
    if (it->functionStart > requestedLine)
        return -1;
    const QList<int> &starts = executable.functionStarts;
    const auto start = std::upper_bound(starts.cbegin(), starts.cend(), requestedLine);
    if (start != starts.cend() && *start < it->line)
        return -1;
    return it->line;
}

void QV4Debugger::pruneExecutableLines()
{
    // Drop the units of files without breakpoints, so that the cache does not
    // keep them from being trimmed. This runs on the engine thread, which is
    // the only one allowed to release a unit.
    m_executableLines.removeIf([this](const UnitLines &cached) {
        const QString file = QUrl(cached.unit->fileName()).fileName();
        for (auto it = m_breakPoints.keyBegin(), end = m_breakPoints.keyEnd(); it != end; ++it) {
            if (it->fileName == file)
                return false;
        }
        return true;
    });
}

/*!
    \internal

    Returns the executable lines of the compilation unit \a function belongs
    to, cached for the most recent units.
*/
const QV4Debugger::ExecutableLines &QV4Debugger::executableLines(QV4::Function *function)
{
    // Enough for execution moving back and forth between the files of a scene
    // that all have breakpoints, few enough for a linear search.
    constexpr qsizetype MaxCachedUnits = 8;

    const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit
            = function->executableCompilationUnit()->baseCompilationUnit();
    for (qsizetype i = 0, n = m_executableLines.size(); i < n; ++i) {
        if (m_executableLines.at(i).unit == unit) {
            if (i > 0)
                m_executableLines.move(i, 0);
            return m_executableLines.first().lines;
        }
    }
    if (m_executableLines.size() == MaxCachedUnits)
        m_executableLines.removeLast();
    m_executableLines.prepend({unit, collectExecutableLines(unit)});
    return m_executableLines.first().lines;
}

/*!
    \internal

    While paused, returns the line a breakpoint requested on \a requestedLine
    resolves to in the unit currently being executed, or -1. Lets the agent
    attribute a hit that landed on a shifted line back to the breakpoint the
    user placed on a non-executable line.
*/
int QV4Debugger::resolvedBreakpointLine(int requestedLine)
{
    QMutexLocker locker(&m_lock);
    QV4::Function *f = getFunction();
    if (!f)
        return -1;
    // This runs on the agent's thread, so it must not change the cache: dropping
    // the reference to an evicted unit could destroy that unit here.
    const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit
            = f->executableCompilationUnit()->baseCompilationUnit();
    for (const UnitLines &cached : std::as_const(m_executableLines)) {
        if (cached.unit == unit)
            return firstLineAtOrAfter(cached.lines, requestedLine);
    }
    return firstLineAtOrAfter(collectExecutableLines(unit), requestedLine);
}

bool QV4Debugger::reallyHitTheBreakPoint(QV4::Function *function, const QString &filename,
                                         int linenr)
{
    const QString file = QUrl(filename).fileName();

    // Besides a breakpoint exactly on this line, a breakpoint requested on a
    // non-executable line (e.g. a comment or blank line) counts here if this is
    // the first executable line at or after it. Any of them can stop execution,
    // so all of them have to be checked.
    QStringList conditions;
    for (auto bp = m_breakPoints.constBegin(), end = m_breakPoints.constEnd(); bp != end; ++bp) {
        if (bp.key().fileName != file || bp.key().lineNumber > linenr)
            continue;
        // Only relocate in a unit the client's target identifies: a same-named
        // file elsewhere has unrelated executable lines.
        if (bp.key().lineNumber != linenr
                && (!filename.endsWith(bp.value().target)
                    || firstLineAtOrAfter(executableLines(function), bp.key().lineNumber)
                            != linenr)) {
            continue;
        }
        if (bp.value().condition.isEmpty())
            return true;
        conditions.append(bp.value().condition);
    }

    for (const QString &condition : std::as_const(conditions)) {
        Q_ASSERT(m_runningJob == nullptr);
        EvalJob evilJob(m_engine, condition);
        m_runningJob = &evilJob;
        m_runningJob->run();
        m_runningJob = nullptr;
        if (evilJob.resultAsBoolean())
            return true;
    }
    return false;
}

void QV4Debugger::runInEngine(QV4DebugJob *job)
{
    QMutexLocker locker(&m_lock);
    runInEngine_havingLock(job);
}

void QV4Debugger::runInEngine_havingLock(QV4DebugJob *job)
{
    Q_ASSERT(job);
    Q_ASSERT(m_runningJob == nullptr);

    m_runningJob = job;
    if (state() == Paused)
        m_runningCondition.wakeAll();
    else
        emit scheduleJob();
    m_jobIsRunning.wait(&m_lock);
    m_runningJob = nullptr;
}

QT_END_NAMESPACE

#include "moc_qv4debugger.cpp"
