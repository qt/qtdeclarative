// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmllintsettings_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlLintSettings::QQmlLintSettings(const QString &name)
    : QQmlToolingSettings(name, { "General"_L1, "Warnings"_L1 })
{
    addOption(s_maxWarnings, -1);
}

qsizetype QQmlLintSettings::maxWarnings() const
{
    if (!isSet(QQmlLintSettings::s_maxWarnings))
        return -1;
    bool ok = false;
    qsizetype result = value(QQmlLintSettings::s_maxWarnings).toLongLong(&ok);
    return ok ? result : -1;
}

QT_END_NAMESPACE
