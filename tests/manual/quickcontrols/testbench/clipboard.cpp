// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "clipboard.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QSettings>
#include <QMap>

const QStringList keys = {
    "Palette/window",
    "Palette/windowText",
    "Palette/base",
    "Palette/text",
    "Palette/button",
    "Palette/buttonText",
    "Palette/brightText",
    "Palette/toolTipBase",
    "Palette/toolTipText",
    "Palette/light",
    "Palette/midlight",
    "Palette/dark",
    "Palette/mid",
    "Palette/shadow",
    "Palette/highlight",
    "Palette/highlightedText",
    "Palette/link"
};

Clipboard::Clipboard(QObject *parent) :
    QObject(parent)
{
}

// Converts the JS map into a big string and copies it to the clipboard.
void Clipboard::copy(const QJSValue &keyValueMap)
{
    QString paletteSettingsString;
    QVariantMap map = keyValueMap.toVariant().value<QVariantMap>();
    const QList<QString> mapKeys = map.keys();
    for (const QString &key : mapKeys) {
        paletteSettingsString += "Palette/" + key + "=" + map.value(key).toString() + ",";
    }

    // Remove the trailing comma.
    if (!paletteSettingsString.isEmpty())
        paletteSettingsString.chop(1);

    QGuiApplication::clipboard()->setText(paletteSettingsString);
}

// Converts the big string into a JS map and returns it.
QVariant Clipboard::paste() const
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard->text().isEmpty())
        return QVariant();

    QVariantMap keyValueMap;

    const QStringList settingsList = clipboard->text().split(QLatin1Char(','));
    for (const QString &setting : settingsList) {
        const QStringList keyValuePair = setting.split(QLatin1Char('='));
        if (keyValuePair.size() < 2)
            continue;

        QString key = keyValuePair.first();
        if (keys.contains(key)) {
            key.remove(QLatin1String("Palette/"));
            const QString value = keyValuePair.last();

            keyValueMap.insert(key, value);
        }
    }

    return QVariant(keyValueMap);
}
