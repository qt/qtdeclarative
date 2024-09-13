// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlformatsettings_p.h"

QQmlFormatSettings::QQmlFormatSettings(const QString &toolName) : QQmlToolingSettings(toolName)
{
    addOption(s_useTabsSetting);
    addOption(s_indentWidthSetting, 4);
    addOption(s_normalizeSetting);
    addOption(s_newlineSetting, QStringLiteral("native"));
    addOption(s_objectsSpacingSetting);
    addOption(s_functionsSpacingSetting);
}
