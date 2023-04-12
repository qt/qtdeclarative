// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfigmaconfigreader_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qdir.h>
#include <QtQml/qqmlengine.h>

#include <QtQuickControls2/qquickstyle.h>

QT_BEGIN_NAMESPACE

QQuickFigmaConfigReader::QQuickFigmaConfigReader(QObject *parent)
    : QObject(parent)
{
    const QStringList importPaths = QQmlEngine().importPathList();
    const QString styleName = QQuickStyle::name();
    for (const QString &importPath : importPaths) {
        QDir importDir(importPath);
        if (importDir.cd(styleName)) {
            setConfigPath(importDir.absolutePath() + QLatin1String("/config.json"));
            break;
        }
    }
}

QString QQuickFigmaConfigReader::configPath() const
{
    return m_configPath;
}

void QQuickFigmaConfigReader::setConfigPath(const QString &path)
{
    if (m_configPath != path) {
        m_configPath = path;
        configPathChanged();
        parseConfig();
    }
}

QVariantMap QQuickFigmaConfigReader::images() const
{
    return m_imagesConfig;
}

/**
 * Opens and stores the JSON config file into a QVariantMap.
 * The config file consists of a JSON object where the keys are the
 * asset names and the values the respective 9patch data as follows:
 * {
    "images": {
        "button-background": {
            "size": [100, 44],
            "topPadding": 6,
            "bottomPadding": 6,
            "topInset": 6,
            "bottomInset": 6
            "leftOffset": 8,
            "rightOffset": 8,
            ...
        }
    }
 *
 */
void QQuickFigmaConfigReader::parseConfig()
{
    QFile file(m_configPath);
    if (!file.exists()) {
        qWarning() << "Figma style: Invalid configuration file path. File does not exist " << m_configPath;
        return;
    }
    if (!file.open(QIODeviceBase::ReadOnly)) {
        qWarning() << "Figma style: Could not open configuration file " << m_configPath;
        return;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        qWarning() << "Figma style: Configuration file " << m_configPath << " is empty ";
        return;
    }
    QJsonParseError err;
    const QJsonDocument config(QJsonDocument::fromJson(data, &err));
    if (config.isEmpty()) {
        qWarning() << "Figma style: Error creating JSON document from configuration file "
                   << err.errorString();
        return;
    }

    const auto jsonObject = config.object();
    if (jsonObject.contains(QString::fromLatin1("atoms"))) {
        const QJsonObject images = jsonObject[QString::fromLatin1("atoms")].toObject();
        m_imagesConfig = images.toVariantMap();
    }
}

QT_END_NAMESPACE

#include "moc_qquickfigmaconfigreader_p.cpp"
