// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljscontextproperties_p.h"

#include <private/qtqmlglobal_p.h>

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qdirlisting.h>
#include <QtCore/qfile.h>

#if QT_CONFIG(qmlcontextpropertydump)
#  include <QtCore/qsettings.h>
#endif

#if QT_CONFIG(process)
#  include <QtCore/qprocess.h>
#endif

QT_BEGIN_NAMESPACE

namespace QQmlJS {

using namespace Qt::StringLiterals;

// There are many ways to set context properties without triggering the regexp in s_pattern,
// but its supposed to catch most context properties set via "setContextProperty".
static constexpr QLatin1StringView s_pattern =
        R"x((\.|->)setContextProperty\s*\(\s*(QStringLiteral\s*\(|QString\s*\(|QLatin1String(View)?\s*\(|u)?\s*"([^"]*)")x"_L1;
static constexpr int s_contextPropertyNameIdxInPattern = 4;

// TODO: use a central list of file extensions that can also be used by qmetatypesjsonprocessor.cpp
// (that needs header file extensions) and Qt6QmlMacros.cmake.
static constexpr std::array s_fileFilters = {
    "*.cpp"_L1, "*.cxx"_L1, "*.cc"_L1, "*.c"_L1, "*.c++"_L1,
    "*.hpp"_L1, "*.hxx"_L1, "*.hh"_L1, "*.h"_L1, "*.h++"_L1,
};

static const QRegularExpression s_matchSetContextProperty{ s_pattern,
                                                           QRegularExpression::MultilineOption };

QList<HeuristicContextProperty>
HeuristicContextProperties::definitionsForName(const QString &name) const
{
    const auto it = m_properties.find(name);
    if (it != m_properties.end())
        return it.value();
    return {};
}

void HeuristicContextProperties::add(const QString &name, const HeuristicContextProperty &property)
{
    if (const auto it = m_properties.find(name); it != m_properties.end()) {
        it.value().append(property);
        return;
    }
    m_properties.insert(name, { property });
}

void HeuristicContextProperties::collectFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QString fileContent = QString::fromUtf8(file.readAll());
    for (const auto &match : s_matchSetContextProperty.globalMatch(fileContent)) {
        const quint32 offset = match.capturedStart(s_contextPropertyNameIdxInPattern);
        const quint32 length = match.capturedLength(s_contextPropertyNameIdxInPattern);
        const auto [row, column] = QQmlJS::SourceLocation::rowAndColumnFrom(fileContent, offset);
        const QQmlJS::SourceLocation sourceLocation{ offset, length, row, column };

        add(match.captured(s_contextPropertyNameIdxInPattern),
            HeuristicContextProperty{ filePath, sourceLocation });
    }
}

void HeuristicContextProperties::grepFallback(const QList<QString> &rootUrls)
{
    const QStringList fileFilters{ s_fileFilters.begin(), s_fileFilters.end() };

    for (const QString &url : rootUrls) {
        for (const auto &dirEntry : QDirListing{ url, fileFilters,
                                                 QDirListing::IteratorFlag::Recursive
                                                         | QDirListing::IteratorFlag::FilesOnly }) {

            const QString filePath = dirEntry.filePath();
            collectFromFile(filePath);
        }
    }
}

#if QT_CONFIG(process) && !defined(Q_OS_WINDOWS)
void HeuristicContextProperties::parseGrepOutput(const QString &output)
{
    for (const auto line : QStringTokenizer{ output, "\n"_L1, Qt::SkipEmptyParts })
        collectFromFile(line.toString());
}
#endif

HeuristicContextProperties
HeuristicContextProperties::collectFromCppSourceDirs(const QList<QString> &cppSourceDirs)
{
    HeuristicContextProperties result;
    result.collectFromDirs(cppSourceDirs);
    return result;
}

/*!
   \internal
    Uses grep to find files that have setContextProperty()-calls, and then search matching files
   with QRegularExpression to extract the location and name of the found context properties.
*/
void HeuristicContextProperties::collectFromDirs(const QList<QString> &dirs)
{
    if (dirs.isEmpty())
        return;

#if QT_CONFIG(process) && !defined(Q_OS_WINDOWS)
    if (qEnvironmentVariableIsSet("QT_QML_NO_GREP")) {
        grepFallback(dirs);
        return;
    }

    QProcess grep;
    QStringList arguments{ "--recursive"_L1,
                           "--null-data"_L1, // match multiline patterns
                           "--files-with-matches"_L1,
                           "--extended-regexp"_L1, // the pattern is "extended"
                           "-e"_L1,
                           s_pattern };

    // don't search non-cpp files
    for (const auto fileFilter : s_fileFilters)
        arguments << "--include"_L1 << fileFilter;

    arguments.append(dirs);
    grep.start("grep"_L1, arguments);
    grep.waitForFinished();
    if (grep.exitStatus() == QProcess::NormalExit) {
        switch (grep.exitCode()) {
        case 0: { // success
            const QString output = QString::fromUtf8(grep.readAllStandardOutput());
            parseGrepOutput(output);
            return;
        }
        case 1: // success but no context properties found
            return;
        default: // grep error
            break;
        }
    }
#endif
    grepFallback(dirs);
}

#if QT_CONFIG(qmlcontextpropertydump)
static SourceLocation deserializeSourceLocation(const QString &string)
{
    constexpr int size = 4;
    const QStringList bits = string.split(u',', Qt::SkipEmptyParts);
    if (bits.length() != size)
        return SourceLocation{};

    SourceLocation result;
    quint32 *destination[size] = { &result.offset, &result.length, &result.startLine,
                                   &result.startColumn };

    bool everythingOk = true;
    for (int i = 0; i < size; ++i) {
        bool ok = false;
        *destination[i] = bits[i].toInt(&ok);
        everythingOk &= ok;
    }

    if (everythingOk)
        return result;
    return SourceLocation{};
}

static QString serializeSourceLocation(const SourceLocation &location)
{
    QString result;
    result.append(QString::number(location.offset)).append(u',');
    result.append(QString::number(location.length)).append(u',');
    result.append(QString::number(location.startLine)).append(u',');
    result.append(QString::number(location.startColumn));
    return result;
}
#endif

#if QT_CONFIG(qmlcontextpropertydump)
static constexpr auto cachedHeuristicListKey = "cachedHeuristicList"_L1;
#endif

HeuristicContextProperties HeuristicContextProperties::collectFrom(QSettings *settings)
{
#if QT_CONFIG(qmlcontextpropertydump)
    HeuristicContextProperties result;
    std::vector<QString> names;

    const int size = settings->beginReadArray(cachedHeuristicListKey);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        names.push_back(settings->value("name").toString());
    }
    settings->endArray();

    for (const auto &name : names) {
        const int size = settings->beginReadArray(u"property_"_s.append(name));
        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            result.add(
                    name,
                    HeuristicContextProperty{
                            settings->value("fileName").toString(),
                            deserializeSourceLocation(settings->value("sourceLocation").toString()),
                    });
        }
        settings->endArray();
    }
    return result;
#else
    Q_UNUSED(settings);
    return HeuristicContextProperties{};
#endif
}

void HeuristicContextProperties::writeCache(const QString &folder) const
{
#if QT_CONFIG(qmlcontextpropertydump)
    QSettings settings(folder + "/.qt/contextPropertyDump.ini"_L1, QSettings::IniFormat);
    settings.beginWriteArray(cachedHeuristicListKey);
    int index = 0;
    for (const auto &[name, _] : m_properties) {
        settings.setArrayIndex(index++);
        settings.setValue("name", name);
    }
    settings.endArray();

    for (const auto &[name, definitions] : m_properties) {
        settings.beginWriteArray(u"property_"_s.append(name));
        for (int i = 0; i < definitions.size(); ++i) {
            settings.setArrayIndex(i);
            settings.setValue("fileName", definitions[i].filename);
            settings.setValue("sourceLocation", serializeSourceLocation(definitions[i].location));
        }
        settings.endArray();
    }
#else
    Q_UNUSED(folder);
#endif
}
} // namespace QQmlJS

QT_END_NAMESPACE
