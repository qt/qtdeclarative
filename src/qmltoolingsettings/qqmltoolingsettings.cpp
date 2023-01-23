// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltoolingsettings_p.h"

#include <algorithm>

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qset.h>
#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#endif
#include <QtCore/qstandardpaths.h>

using namespace Qt::StringLiterals;

void QQmlToolingSettings::addOption(const QString &name, QVariant defaultValue)
{
    m_values[name] = defaultValue;
}

bool QQmlToolingSettings::read(const QString &settingsFilePath)
{
#if QT_CONFIG(settings)
    if (!QFileInfo::exists(settingsFilePath))
        return false;

    if (m_currentSettingsPath == settingsFilePath)
        return true;

    QSettings settings(settingsFilePath, QSettings::IniFormat);

    for (const QString &key : settings.allKeys())
        m_values[key] = settings.value(key).toString();

    m_currentSettingsPath = settingsFilePath;

    return true;
#else
    return false;
#endif
}

bool QQmlToolingSettings::writeDefaults() const
{
#if QT_CONFIG(settings)
    const QString path = QFileInfo(u".%1.ini"_s.arg(m_toolName)).absoluteFilePath();

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

bool QQmlToolingSettings::search(const QString &path)
{
#if QT_CONFIG(settings)
    QFileInfo fileInfo(path);
    QDir dir(fileInfo.isDir() ? path : fileInfo.dir());

    QSet<QString> dirs;

    const QString settingsFileName = u".%1.ini"_s.arg(m_toolName);

    while (dir.exists() && dir.isReadable()) {
        const QString dirPath = dir.absolutePath();

        if (m_seenDirectories.contains(dirPath)) {
            const QString cachedIniPath = m_seenDirectories[dirPath];
            if (cachedIniPath.isEmpty())
                return false;

            return read(cachedIniPath);
        }

        dirs << dirPath;

        const QString iniFile = dir.absoluteFilePath(settingsFileName);

        if (read(iniFile)) {
            for (const QString &dir : std::as_const(dirs))
                m_seenDirectories[dir] = iniFile;
            return true;
        }

        if (!dir.cdUp())
            break;
    }

    if (const QString iniFile = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, u"%1.ini"_s.arg(m_toolName));
        !iniFile.isEmpty()) {
        if (read(iniFile)) {
            for (const QString &dir : std::as_const(dirs))
                m_seenDirectories[dir] = iniFile;
            return true;
        }
    }

    // No INI file found anywhere, record the failure so we won't have to traverse the entire
    // filesystem again
    for (const QString &dir : std::as_const(dirs))
        m_seenDirectories[dir] = QString();

#endif
    return false;
}

QVariant QQmlToolingSettings::value(QString name) const
{
    return m_values.value(name);
}

bool QQmlToolingSettings::isSet(QString name) const
{
    if (!m_values.contains(name))
        return false;

    QVariant variant = m_values[name];

    // Unset is encoded as an empty string
    return !(variant.canConvert(QMetaType(QMetaType::QString)) && variant.toString().isEmpty());
}
