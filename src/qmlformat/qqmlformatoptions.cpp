// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlformatoptions_p.h"

QQmlFormatOptions::QQmlFormatOptions()
{
    m_options.updateOptions = QQmlJS::Dom::LineWriterOptions::Update::None;

    setTabsEnabled(false);
    setNormalizeEnabled(false);
    setObjectsSpacing(false);
    setFunctionsSpacing(false);
    setIndentWidth(4);
}

QQmlFormatOptions::LineEndings QQmlFormatOptions::detectLineEndings(const QString &code)
{
    const QQmlJS::Dom::LineWriterOptions::LineEndings defaultEndings =
#if defined(Q_OS_WIN)
            LineEndings::Windows;
#else
            LineEndings::Unix;
#endif
    // find out current line endings...
    int newlineIndex = code.indexOf(QChar(u'\n'));
    int crIndex = code.indexOf(QChar(u'\r'));
    if (newlineIndex >= 0) {
        if (crIndex >= 0) {
            if (crIndex + 1 == newlineIndex)
                return LineEndings::Windows;

            qWarning().noquote() << "Invalid line ending in file, using default";
            return defaultEndings;
        }
        return LineEndings::Unix;
    }
    if (crIndex >= 0) {
        return LineEndings::OldMacOs;
    }

    qWarning().noquote() << "Unknown line ending in file, using default";
    return defaultEndings;
}

QQmlFormatOptionLineEndings QQmlFormatOptions::parseEndings(const QString &endings)
{
    if (endings == u"unix")
        return Unix;
    if (endings == u"windows")
        return Windows;
    if (endings == u"macos")
        return OldMacOs;
    if (endings == u"native")
        return Native;

    qWarning().noquote() << "Unknown line ending type" << endings << ", using default";
#if defined(Q_OS_WIN)
    return Windows;
#else
    return Unix;
#endif
}

void QQmlFormatOptions::applySettings(const QQmlFormatSettings &settings)
{
    // Allow for tab settings to be overwritten by the command line
    if (!indentWidthSet()) {
        if (settings.isSet(QQmlFormatSettings::s_indentWidthSetting))
            setIndentWidth(settings.value(QQmlFormatSettings::s_indentWidthSetting).toInt());
        if (settings.isSet(QQmlFormatSettings::s_useTabsSetting))
            setTabsEnabled(settings.value(QQmlFormatSettings::s_useTabsSetting).toBool());
    }

    if (settings.isSet(QQmlFormatSettings::s_normalizeSetting))
        setNormalizeEnabled(settings.value(QQmlFormatSettings::s_normalizeSetting).toBool());

    if (settings.isSet(QQmlFormatSettings::s_newlineSetting))
        setNewline(QQmlFormatOptions::parseEndings(
                settings.value(QQmlFormatSettings::s_newlineSetting).toString()));

    if (settings.isSet(QQmlFormatSettings::s_objectsSpacingSetting))
        setObjectsSpacing(settings.value(QQmlFormatSettings::s_objectsSpacingSetting).toBool());

    if (settings.isSet(QQmlFormatSettings::s_functionsSpacingSetting))
        setFunctionsSpacing(settings.value(QQmlFormatSettings::s_functionsSpacingSetting).toBool());
}
