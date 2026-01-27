// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmltoolingsettings_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qset.h>
#include <QtCore/qtextstream.h>
#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#endif
#include <QtCore/qstandardpaths.h>

using namespace Qt::StringLiterals;

QQmlToolingSettings::SearchOptions::SearchOptions() = default;
QQmlToolingSettings::SearchOptions::SearchOptions(
        const QString &settingFileName, bool reportFoundSettingsFiles, bool isQmllintSilent)
    : settingsFileName(settingFileName), reportFoundSettingsFiles(reportFoundSettingsFiles),
      isQmllintSilent(isQmllintSilent)
{

}

void QQmlToolingSettings::addOption(const QString &name, const QVariant &defaultValue)
{
    if (defaultValue.isValid()) {
        m_values[name] = defaultValue;
    }
}

QQmlToolingSettings::QQmlToolingSettings(const QString &toolName,
                                         const QStringList &recognizedIniSections)
    : m_searcher(u".%1.ini"_s.arg(toolName), u"%1.ini"_s.arg(toolName)),
      m_recognizedIniSections(recognizedIniSections)
{
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::read(const QString &settingsFilePath,
                                                            SearchOptions options)
{
#if QT_CONFIG(settings)
    Q_ASSERT(QFileInfo::exists(settingsFilePath));

    if (m_currentSettingsPath == settingsFilePath)
        return { SearchResult::ResultType::Found, settingsFilePath };

    QSettings settings(settingsFilePath, QSettings::IniFormat);

    if (!options.isQmllintSilent) {
        const QStringList sections = settings.childGroups() << QLatin1String("General");
        for (const QString &section : sections) {
            if (!m_recognizedIniSections.contains(section)) {
                qWarning().noquote()
                        << "Unrecognized section \"%1\" in %2\n"_L1.arg(section).arg(settingsFilePath)
                                + "Recognized sections are: ["_L1
                                + m_recognizedIniSections.join(", "_L1) + u']';
            }
        }
    }

    for (const QString &key : settings.allKeys())
        m_values[key] = settings.value(key).toString();

    m_currentSettingsPath = settingsFilePath;
    return { SearchResult::ResultType::Found, settingsFilePath };
#else
    Q_UNUSED(settingsFilePath);
    return SearchResult();
#endif
}

bool QQmlToolingSettings::writeDefaults() const
{
#if QT_CONFIG(settings)
    const QString path = QFileInfo(m_searcher.localSettingsFile()).absoluteFilePath();

    QSettings settings(path, QSettings::IniFormat);
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        settings.setValue(it.key(), it.value().isNull() ? QString() : it.value());
    }

    settings.sync();

    if (settings.status() != QSettings::NoError) {
        qWarning() << "Failed to write default settings to" << path
                   << "Error:" << settings.status();
        return false;
    }

    qInfo() << "Wrote default settings to" << path;
    return true;
#else
    return false;
#endif
}

QQmlToolingSettings::SearchResult
QQmlToolingSettings::Searcher::searchCurrentDirInCache(const QString &dirPath)
{
    const auto it = m_seenDirectories.constFind(dirPath);
    return it != m_seenDirectories.constEnd()
            ? SearchResult{ SearchResult::ResultType::Found, *it }
            : SearchResult{ SearchResult::ResultType::NotFound, {} };
}

static QString findIniFile(const QString &local, const QString &global)
{
    // If we reach here, we didn't find the settings file in the current directory or any parent
    // directories. Now we will try to locate the settings file in the standard locations. First try
    // to locate settings file with the standard name.
    const QString iniFile = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, local);
    if (!iniFile.isEmpty())
        return iniFile;

    // If not found, try alternate name format
    return QStandardPaths::locate(QStandardPaths::GenericConfigLocation, global);
}

QQmlToolingSettings::SearchResult
QQmlToolingSettings::Searcher::searchDefaultLocation(const QSet<QString> *visitedDirs)
{
    QString iniFile = findIniFile(m_localSettingsFile, m_globalSettingsFile);

    // Update the seen directories cache unconditionally with the current result
    for (const QString &dir : *visitedDirs)
        m_seenDirectories[dir] = iniFile;

    const SearchResult::ResultType found = iniFile.isEmpty()
            ? SearchResult::ResultType::NotFound
            : SearchResult::ResultType::Found;
    return SearchResult { found, std::move(iniFile) };
}

QQmlToolingSettings::SearchResult
QQmlToolingSettings::Searcher::searchDirectoryHierarchy(
        QSet<QString> *visitedDirs, const QString &path)
{
    const QFileInfo fileInfo(path);
    QDir dir(fileInfo.isDir() ? path : fileInfo.dir());

    while (dir.exists() && dir.isReadable()) {
        const QString dirPath = dir.absolutePath();

        // Check if we have already seen this directory
        // If we have, we can use the cached INI file path
        // to avoid unnecessary file system operations
        // This is useful for large directory trees where the settings file might be in a parent
        // directory
        if (const SearchResult result = searchCurrentDirInCache(dirPath); result.isValid())
            return result;

        visitedDirs->insert(dirPath);

        // Check if the settings file exists in the current directory
        // If it does, read it and update the seen directories cache
        if (QString ini = dir.absoluteFilePath(m_localSettingsFile); QFileInfo::exists(ini)) {
            for (const QString &visitedDir : std::as_const(*visitedDirs))
                m_seenDirectories[visitedDir] = ini;

            return { SearchResult::ResultType::Found, std::move(ini) };
        }

        if (!dir.cdUp())
            break;
    }

    return SearchResult();
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::Searcher::search(const QString &path)
{
    QSet<QString> visitedDirs;

    // Try to find settings in directory hierarchy
    if (const SearchResult result = searchDirectoryHierarchy(&visitedDirs, path); result.isValid())
        return result;

    // If we didn't find the settings file in the current directory or any parent directories,
    // try to locate the settings file in the standard locations
    if (const SearchResult result = searchDefaultLocation(&visitedDirs); result.isValid())
        return result;

    return SearchResult();
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::search(
        const QString &path, const SearchOptions &options)
{
    const auto maybeReport = qScopeGuard([&]() {
        if (options.reportFoundSettingsFiles)
            reportConfigForFiles({ path });
    });

    // If a specific settings file is provided, read it directly
    if (!options.settingsFileName.isEmpty()) {
        QFileInfo fileInfo(options.settingsFileName);
        return fileInfo.exists() ? read(fileInfo.absoluteFilePath(), options) : SearchResult();
    }

    if (const SearchResult result = m_searcher.search(path); result.isValid())
        return read(result.iniFilePath, options);

    return SearchResult();
}

QVariant QQmlToolingSettings::value(const QString &name) const
{
    return m_values.value(name);
}

QStringList QQmlToolingSettings::valueAsStringList(const QString &name) const
{
    return value(name).toString().split(QDir::listSeparator());
}

void QQmlToolingSettings::resolveRelativeImportPaths(const QString &filePath, QStringList *paths)
{
    // transform relative paths in absolute paths starting from filePath's directory
    const QDir fileDir = QFileInfo(filePath).absoluteDir();
    for (auto it = paths->begin(), end = paths->end(); it != end; ++it) {
        if (QFileInfo(*it).isAbsolute())
            continue;
        *it = QDir::cleanPath(fileDir.filePath(*it));
    }
}

QStringList QQmlToolingSettings::valueAsAbsolutePathList(const QString &name,
                                                         const QString &baseForRelativePaths) const
{
    QStringList paths = valueAsStringList(name);
    resolveRelativeImportPaths(baseForRelativePaths, &paths);
    return paths;
}

bool QQmlToolingSettings::isSet(const QString &name) const
{
    if (!m_values.contains(name))
        return false;

    QVariant variant = m_values[name];

    // Unset is encoded as an empty string
    return !(variant.canConvert(QMetaType(QMetaType::QString)) && variant.toString().isEmpty());
}

bool QQmlToolingSettings::reportConfigForFiles(const QStringList &files)
{
    constexpr int maxAllowedFileLength = 255;
    constexpr int minAllowedFileLength = 40;
    bool headerPrinted = false;
    auto lengthForFile = [maxAllowedFileLength](const QString &file) {
        return std::min(int(file.length()), maxAllowedFileLength);
    };

    int maxFileLength =
            std::accumulate(files.begin(), files.end(), 0, [&](int acc, const QString &file) {
                return std::max(acc, lengthForFile(file));
            });

    if (maxFileLength < minAllowedFileLength)
        maxFileLength = minAllowedFileLength;

    for (const auto &file : files) {
        if (file.isEmpty()) {
            qWarning().noquote() << "Error: Could not find file" << file;
            return false;
        }

        QString displayFile = file;
        if (displayFile.length() > maxAllowedFileLength) {
            displayFile = u"..." + displayFile.right(maxAllowedFileLength - 3);
        }

        const auto result = m_searcher.search(file);

        if (!headerPrinted) {
            QString header =
                    QStringLiteral("%1 | %2").arg("File", -maxFileLength).arg("Settings File");
            qWarning().noquote() << header;
            qWarning().noquote() << QString(header.length(), u'-');
            headerPrinted = true;
        }
        QString line =
                QStringLiteral("%1 | %2").arg(displayFile, -maxFileLength).arg(result.iniFilePath);
        qWarning().noquote() << line;
    }

    return true;
}
