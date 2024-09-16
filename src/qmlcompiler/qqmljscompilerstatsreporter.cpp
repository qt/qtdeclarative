// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljscompilerstatsreporter_p.h"

#include <QFileInfo>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

using namespace Qt::StringLiterals;

AotStatsReporter::AotStatsReporter(const AotStats &aotstats) : m_aotstats(aotstats)
{
    for (const auto &[moduleUri, fileEntries] : aotstats.entries().asKeyValueRange()) {
        for (const auto &[filepath, statsEntries] : fileEntries.asKeyValueRange()) {
            for (const auto &entry : statsEntries) {
                m_fileCounters[moduleUri][filepath].codegens += 1;
                if (entry.codegenSuccessful) {
                    m_fileCounters[moduleUri][filepath].successes += 1;
                    m_successDurations.append(entry.codegenDuration);
                }
            }
            m_moduleCounters[moduleUri].codegens += m_fileCounters[moduleUri][filepath].codegens;
            m_moduleCounters[moduleUri].successes += m_fileCounters[moduleUri][filepath].successes;
        }
        m_totalCounters.codegens += m_moduleCounters[moduleUri].codegens;
        m_totalCounters.successes += m_moduleCounters[moduleUri].successes;
    }
}

void AotStatsReporter::formatDetailedStats(QTextStream &s) const
{
    s << "############ AOT COMPILATION STATS ############\n";
    for (const auto &[moduleUri, fileStats] : m_aotstats.entries().asKeyValueRange()) {
        s << u"Module %1:\n"_s.arg(moduleUri);
        if (fileStats.empty()) {
            s << "No attempts at compiling a binding or function\n";
            continue;
        }

        for (const auto &[filename, entries] : fileStats.asKeyValueRange()) {
            s << u"--File %1\n"_s.arg(filename);
            if (entries.empty()) {
                s << "  No attempts at compiling a binding or function\n";
                continue;
            }

            int successes = m_fileCounters[moduleUri][filename].successes;
            s << "  " << formatSuccessRate(entries.size(), successes) << "\n";

            for (const auto &stat : entries) {
                s << u"    %1: [%2:%3:%4]\n"_s.arg(stat.functionName)
                                .arg(QFileInfo(filename).fileName())
                                .arg(stat.line)
                                .arg(stat.column);
                s << u"      result: "_s << (stat.codegenSuccessful
                                                     ? u"Success\n"_s
                                                     : u"Error: "_s + stat.errorMessage + u'\n');
                s << u"      duration: %1us\n"_s.arg(stat.codegenDuration.count());
            }
            s << "\n";
        }
    }
}

void AotStatsReporter::formatSummary(QTextStream &s) const
{
    s << "############ AOT COMPILATION STATS SUMMARY ############\n";
    if (m_totalCounters.codegens == 0) {
        s << "No attempted compilations to Cpp for bindings or functions.\n";
        return;
    }

    for (const auto &moduleUri : m_aotstats.entries().keys()) {
        const auto &counters = m_moduleCounters[moduleUri];
        s << u"Module %1: "_s.arg(moduleUri)
          << formatSuccessRate(counters.codegens, counters.successes) << "\n";
    }

    s << "Total results: " << formatSuccessRate(m_totalCounters.codegens, m_totalCounters.successes);
    s << "\n";

    if (m_totalCounters.successes != 0) {
        auto totalDuration = std::accumulate(m_successDurations.cbegin(), m_successDurations.cend(),
                                             std::chrono::microseconds(0));
        const auto averageDuration = totalDuration.count() / m_totalCounters.successes;
        s << u"Successful codegens took an average of %1us\n"_s.arg(averageDuration);
    }
}

QString AotStatsReporter::format() const
{
    QString output;
    QTextStream s(&output);

    formatDetailedStats(s);
    formatSummary(s);

    return output;
}

QString AotStatsReporter::formatSuccessRate(int codegens, int successes) const
{
    if (codegens == 0)
        return u"No attempted compilations"_s;

    return u"%1 of %2 (%3%4) bindings or functions compiled to Cpp successfully"_s
            .arg(successes)
            .arg(codegens)
            .arg(double(successes) / codegens * 100, 0, 'g', 4)
            .arg(u"%"_s);
}

} // namespace QQmlJS

QT_END_NAMESPACE
