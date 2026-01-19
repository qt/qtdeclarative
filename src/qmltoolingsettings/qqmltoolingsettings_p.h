// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLTOOLINGSETTINGS_P_H
#define QQMLTOOLINGSETTINGS_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmutex.h>
#include <QtCore/qset.h>
#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

class QQmlToolingSettings
{
public:
    QQmlToolingSettings(const QString &toolName,
                        const QStringList &recognizedIniSections = { QStringLiteral("General") });
    struct SearchOptions
    {
        SearchOptions();
        SearchOptions(const QString &settingFileName, bool reportFoundSettingsFiles = false,
                      bool isQmllintSilent = false);
        QString settingsFileName;
        bool reportFoundSettingsFiles = false;
        bool isQmllintSilent = false;
    };
    struct SearchResult
    {
        enum class ResultType { Found, NotFound };
        ResultType type = ResultType::NotFound;
        QString iniFilePath;
        bool isValid() const { return type == ResultType::Found && !iniFilePath.isEmpty(); }
    };

    class Searcher
    {
    public:
        Searcher(const QString &localSettingsFile, const QString &globalSettingsFile)
            : m_localSettingsFile(localSettingsFile), m_globalSettingsFile(globalSettingsFile)
        {
        }
        SearchResult search(const QString &path);

        QString localSettingsFile() const { return m_localSettingsFile; }

    private:
        SearchResult searchDefaultLocation(const QSet<QString> *visitedDirs);
        SearchResult searchCurrentDirInCache(const QString &dirPath);
        SearchResult searchDirectoryHierarchy(QSet<QString> *visitedDir, const QString &path);

        const QString m_localSettingsFile;
        const QString m_globalSettingsFile;
        QHash<QString, QString> m_seenDirectories;
    };

    void addOption(const QString &name, const QVariant &defaultValue = QVariant());
    SearchResult search(const QString &path, const SearchOptions &options = {});
    bool writeDefaults() const;

    QVariant value(const QString &name) const;
    QStringList valueAsStringList(const QString &name) const;

    static void resolveRelativeImportPaths(const QString &filePath, QStringList *paths);
    QStringList valueAsAbsolutePathList(const QString &name,
                                        const QString &baseForRelativePaths) const;
    bool isSet(const QString &name) const;

    bool reportConfigForFiles(const QStringList &files);

    void saveValues() { m_savedValues = m_values; }
    void restoreValues() { m_values = m_savedValues; }

protected:
    QString m_currentSettingsPath;
    QVariantHash m_savedValues;
    QVariantHash m_values;
    Searcher m_searcher;
    QStringList m_recognizedIniSections;

    SearchResult read(const QString &settingsFilePath, SearchOptions options);
};

class QQmlToolingSharedSettings : private QQmlToolingSettings
{
public:
    QQmlToolingSharedSettings(const QString &toolName) : QQmlToolingSettings(toolName) { }

    void addOption(const QString &name, const QVariant &defaultValue = QVariant())
    {
        QMutexLocker lock(&m_mutex);
        QQmlToolingSettings::addOption(name, defaultValue);
    }

    bool writeDefaults() const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::writeDefaults();
    }

    SearchResult search(const QString &path, const SearchOptions &options = {})
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::search(path, options);
    }

    QVariant value(const QString &name) const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::value(name);
    }

    QStringList valueAsStringList(const QString &name) const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::valueAsStringList(name);
    }

    QStringList valueAsAbsolutePathList(const QString &name,
                                        const QString &baseForRelativePaths) const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::valueAsAbsolutePathList(name, baseForRelativePaths);
    }

    bool isSet(const QString &name) const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::isSet(name);
    }

private:
    mutable QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // QQMLTOOLINGSETTINGS_P_H
