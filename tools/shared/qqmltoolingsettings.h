/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
******************************************************************************/

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
