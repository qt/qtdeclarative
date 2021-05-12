/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmltoolingsettings.h"

#include <algorithm>

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qset.h>
#include <QtCore/qsettings.h>
#include <QtCore/qstandardpaths.h>

void QQmlToolingSettings::addOption(const QString &name, QVariant defaultValue)
{
    m_values[name] = defaultValue;
}

bool QQmlToolingSettings::read(const QString &settingsFilePath)
{
    if (!QFileInfo::exists(settingsFilePath))
        return false;

    if (m_currentSettingsPath == settingsFilePath)
        return true;

    QSettings settings(settingsFilePath, QSettings::IniFormat);

    for (const QString &key : settings.allKeys())
        m_values[key] = settings.value(key).toString();

    m_currentSettingsPath = settingsFilePath;

    return true;
}

bool QQmlToolingSettings::writeDefaults() const
{
    const QString path = QFileInfo(u".%1.ini"_qs.arg(m_toolName)).absoluteFilePath();

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
}

bool QQmlToolingSettings::search(const QString &path)
{
    QFileInfo fileInfo(path);
    QDir dir(fileInfo.isDir() ? path : fileInfo.dir());

    QSet<QString> dirs;

    const QString settingsFileName = u".%1.ini"_qs.arg(m_toolName);

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
            for (const QString &dir : qAsConst(dirs))
                m_seenDirectories[dir] = iniFile;
            return true;
        }

        if (!dir.cdUp())
            break;
    }

    if (const QString iniFile = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, u"%1.ini"_qs.arg(m_toolName)); !iniFile.isEmpty()) {
        if (read(iniFile)) {
            for (const QString &dir : qAsConst(dirs))
                m_seenDirectories[dir] = iniFile;
            return true;
        }
    }

    // No INI file found anywhere, record the failure so we won't have to traverse the entire
    // filesystem again
    for (const QString &dir : qAsConst(dirs))
        m_seenDirectories[dir] = QString();

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
