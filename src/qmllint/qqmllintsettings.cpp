// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmllintsettings_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlLintSettings::QQmlLintSettings(const QString &name)
    : QQmlToolingSettings(name, { "General"_L1, "Warnings"_L1 })
{
}

QT_END_NAMESPACE
