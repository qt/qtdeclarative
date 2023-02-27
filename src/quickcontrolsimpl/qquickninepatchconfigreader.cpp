// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickninepatchconfigreader_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

QQuickNinePatchConfigReader::QQuickNinePatchConfigReader(QQuickItem *parent)
    : QQuickItem(parent)
{
}

QString QQuickNinePatchConfigReader::configPath() const
{
    return m_configPath;
}

void QQuickNinePatchConfigReader::setConfigPath(const QString &path)
{
    // TODO: set the config path by searching the generated style dir
    if (m_configPath != path) {
        m_configPath = path;
        configPathChanged();
        parseConfig();
    }
}

QJsonObject QQuickNinePatchConfigReader::verticalStretch() const
{
    return m_verticalStretch;
}

QJsonObject QQuickNinePatchConfigReader::horizontalStretch() const
{
    return m_horizontalStretch;
}

QJsonObject QQuickNinePatchConfigReader::paddings() const
{
    return m_paddings;
}

QJsonObject QQuickNinePatchConfigReader::insets() const
{
    return m_insets;
}

/**
 * Parses the JSON configuration file with the 9patch image data
 * and stores the relevant properties.
 * The config file consists of a JSON object where the keys are the
 * asset names and the values the respective 9patch data as follows:
 * {
    "images": {
        "button-background": {
            "size": [100, 44],
            "padding": [6, 6, 6, 6],
            "insets": [6, 6, 6, 6],
            "verticalStretch": [7, 37, 40, 44],
            "horizontalStretch": [7, 93]
        }
    }
 *
 */
void QQuickNinePatchConfigReader::parseConfig()
{
    QFile file(m_configPath);
    if (!file.exists()) {
        qWarning() << "Invalid configuration path. File does not exist";
        return;
    }
    if (!file.open(QIODeviceBase::ReadOnly)) {
        qWarning() << "Could not open configuration file";
        return;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        qWarning() << "No configuration data available";
        return;
    }
    QJsonParseError err;
    const QJsonDocument config(QJsonDocument::fromJson(data, &err));
    if (config.isEmpty()) {
        qWarning() << "Error creating Json document" << err.errorString();
        return;
    }
    const auto jsonObject = config.object();
    if (jsonObject.contains(QString::fromLatin1("images"))) {
        const QJsonObject images = jsonObject[QString::fromLatin1("images")].toObject();

        // go through all images and store the properties
        for (auto it = images.begin(); it < images.end(); it++) {
            const auto key = it.key();
            if (it.value().isObject()) {
                const auto imageData = it.value().toObject();
                if (imageData.contains(QString::fromLatin1("verticalStretch"))) {
                    const auto verticalStretch = imageData[QString::fromLatin1("verticalStretch")];
                    m_verticalStretch.insert(key, verticalStretch);
                }
                if (imageData.contains(QString::fromLatin1("horizontalStretch"))) {
                    const auto horizontalStretch = imageData[QString::fromLatin1("horizontalStretch")];
                    m_horizontalStretch.insert(key, horizontalStretch);
                }
                if (imageData.contains(QString::fromLatin1("padding"))) {
                    const auto paddings = imageData[QString::fromLatin1("padding")];
                    m_paddings.insert(key, paddings);
                }
                if (imageData.contains(QString::fromLatin1("insets"))) {
                    const auto insets = imageData[QString::fromLatin1("insets")];
                    m_insets.insert(key, insets);
                }
            }
        }
    }
}

QT_END_NAMESPACE

#include "moc_qquickninepatchconfigreader_p.cpp"
