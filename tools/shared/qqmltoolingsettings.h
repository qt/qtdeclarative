// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTOOLINGSETTINGS_H
#define QQMLTOOLINGSETTINGS_H

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>

class QQmlToolingSettings
{
public:
    QQmlToolingSettings(const QString &toolName) : m_toolName(toolName) { }

    void addOption(const QString &name, const QVariant defaultValue = QVariant());

    bool writeDefaults() const;
    bool search(const QString &path);

    QVariant value(QString name) const;
    bool isSet(QString name) const;

private:
    QString m_toolName;
    QString m_currentSettingsPath;
    QHash<QString, QString> m_seenDirectories;
    QVariantHash m_values;

    bool read(const QString &settingsFilePath);
};

#endif
