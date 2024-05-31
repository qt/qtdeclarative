// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljscompilerstats_p.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

using namespace Qt::StringLiterals;

std::unique_ptr<AotStats> QQmlJSAotCompilerStats::s_instance = std::make_unique<AotStats>();
QString QQmlJSAotCompilerStats::s_moduleId;
bool QQmlJSAotCompilerStats::s_recordAotStats = false;

bool QQmlJS::AotStatsEntry::operator<(const AotStatsEntry &other) const
{
    if (line == other.line)
        return column < other.column;
    return line < other.line;
}

void AotStats::insert(AotStats other)
{
    for (const auto &[moduleUri, moduleStats] : other.m_entries.asKeyValueRange()) {
        m_entries[moduleUri].insert(moduleStats);
    }
}

std::optional<QList<QString>> extractAotstatsFilesList(const QString &aotstatsListPath)
{
    QFile aotstatsListFile(aotstatsListPath);
    if (!aotstatsListFile.open(QIODevice::ReadOnly | QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug().noquote() << u"Could not open \"%1\" for reading"_s.arg(aotstatsListFile.fileName());
        return std::nullopt;
    }

    QStringList aotstatsFiles;
    QTextStream stream(&aotstatsListFile);
    while (!stream.atEnd())
        aotstatsFiles.append(stream.readLine());

    return aotstatsFiles;
}

std::optional<AotStats> AotStats::parseAotstatsFile(const QString &aotstatsPath)
{
    QFile file(aotstatsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug().noquote() << u"Could not open \"%1\""_s.arg(aotstatsPath);
        return std::nullopt;
    }

    return AotStats::fromJsonDocument(QJsonDocument::fromJson(file.readAll()));
}

std::optional<AotStats> AotStats::aggregateAotstatsList(const QString &aotstatsListPath)
{
    const auto aotstatsFiles = extractAotstatsFilesList(aotstatsListPath);
    if (!aotstatsFiles.has_value())
        return std::nullopt;

    AotStats aggregated;
    if (aotstatsFiles->empty())
        return aggregated;

    for (const auto &aotstatsFile : aotstatsFiles.value()) {
        auto parsed = parseAotstatsFile(aotstatsFile);
        if (!parsed.has_value())
            return std::nullopt;
        aggregated.insert(parsed.value());
    }

    return aggregated;
}

AotStats AotStats::fromJsonDocument(const QJsonDocument &document)
{
    QJsonArray modulesArray = document.array();

    QQmlJS::AotStats result;
    for (const auto &modulesArrayEntry : modulesArray) {
        const auto &moduleObject = modulesArrayEntry.toObject();
        QString moduleId = moduleObject[u"moduleId"_s].toString();
        const QJsonArray &filesArray = moduleObject[u"moduleFiles"_s].toArray();

        QHash<QString, QList<AotStatsEntry>> files;
        for (const auto &filesArrayEntry : filesArray) {
            const QJsonObject &fileObject = filesArrayEntry.toObject();
            QString filepath = fileObject[u"filepath"_s].toString();
            const QJsonArray &statsArray = fileObject[u"entries"_s].toArray();

            QList<AotStatsEntry> stats;
            for (const auto &statsArrayEntry : statsArray) {
                const auto &statsObject = statsArrayEntry.toObject();
                QQmlJS::AotStatsEntry stat;
                auto micros = statsObject[u"durationMicroseconds"_s].toInteger();
                stat.codegenDuration = std::chrono::microseconds(micros);
                stat.functionName = statsObject[u"functionName"_s].toString();
                stat.errorMessage = statsObject[u"errorMessage"_s].toString();
                stat.line = statsObject[u"line"_s].toInt();
                stat.column = statsObject[u"column"_s].toInt();
                stat.codegenSuccessful = statsObject[u"codegenSuccessfull"_s].toBool();
                stats.append(std::move(stat));
            }

            std::sort(stats.begin(), stats.end());
            files[filepath] = stats;
        }

        result.m_entries[moduleId] = files;
    }

    return result;
}

QJsonDocument AotStats::toJsonDocument() const
{
    QJsonArray modulesArray;
    for (auto it1 = m_entries.begin(); it1 != m_entries.end(); ++it1) {
        const QString moduleId = it1.key();
        const QHash<QString, QList<AotStatsEntry>> &files = it1.value();

        QJsonArray filesArray;
        for (auto it2 = files.begin(); it2 != files.end(); ++it2) {
            const QString &filename = it2.key();
            const QList<AotStatsEntry> &stats = it2.value();

            QJsonArray statsArray;
            for (const auto &stat : stats) {
                QJsonObject statObject;
                auto micros = static_cast<qint64>(stat.codegenDuration.count());
                statObject.insert(u"durationMicroseconds", micros);
                statObject.insert(u"functionName", stat.functionName);
                statObject.insert(u"errorMessage", stat.errorMessage);
                statObject.insert(u"line", stat.line);
                statObject.insert(u"column", stat.column);
                statObject.insert(u"codegenSuccessfull", stat.codegenSuccessful);
                statsArray.append(statObject);
            }

            QJsonObject o;
            o.insert(u"filepath"_s, filename);
            o.insert(u"entries"_s, statsArray);
            filesArray.append(o);
        }

        QJsonObject o;
        o.insert(u"moduleId"_s, moduleId);
        o.insert(u"moduleFiles"_s, filesArray);
        modulesArray.append(o);
    }

    return QJsonDocument(modulesArray);
}

void AotStats::addEntry(const QString &moduleId, const QString &filepath, AotStatsEntry entry)
{
    m_entries[moduleId][filepath].append(entry);
}

bool AotStats::saveToDisk(const QString &filepath) const
{
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug().noquote() << u"Could not open \"%1\""_s.arg(filepath);
        return false;
    }

    file.write(this->toJsonDocument().toJson(QJsonDocument::Indented));
    return true;
}

void QQmlJSAotCompilerStats::addEntry(QString filepath, QQmlJS::AotStatsEntry entry)
{
    auto *aotstats = QQmlJSAotCompilerStats::instance();
    aotstats->addEntry(s_moduleId, filepath, entry);
}

} // namespace QQmlJS

QT_END_NAMESPACE
