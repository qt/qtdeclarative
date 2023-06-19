// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfigmaconfigreader_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qdir.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include <QtQml/qqmlengine.h>

#include <QtQuickControls2/qquickstyle.h>

#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

QQuickFigmaConfigReader::QQuickFigmaConfigReader(QObject *parent)
    : QObject(parent)
{
    resolveConfigPath();
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &QQuickFigmaConfigReader::resolveConfigPath);
}

void QQuickFigmaConfigReader::resolveConfigPath()
{
    const QStringList importPaths = QQmlEngine().importPathList();
    const QString styleName = QQuickStyle::name();
    for (const QString &importPath : importPaths) {
        QDir importDir(importPath);
        if (importDir.cd(styleName)) {
            const QString themeDir = qGuiApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark
                    ? QStringLiteral("/dark") : QStringLiteral("/light");
            setConfigPath(importDir.absolutePath() + themeDir + QLatin1String("/config.json"));
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
    if (m_configPath == path)
        return;

    m_configPath = path;
    parseConfig();

    emit controlsChanged();
    emit configPathChanged();
}

QVariantMap QQuickFigmaConfigReader::controls() const
{
    return m_controlsConfig;
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
    if (jsonObject.contains(QString::fromLatin1("controls"))) {
        const QJsonObject controls = jsonObject[QString::fromLatin1("controls")].toObject();
        m_controlsConfig = controls.toVariantMap();
    }
}

QT_END_NAMESPACE

#include "moc_qquickfigmaconfigreader_p.cpp"
