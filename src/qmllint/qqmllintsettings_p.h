// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLLINTSETTINGS_P_H
#define QQMLLINTSETTINGS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmljsloggingutils_p.h>
#include <private/qqmltoolingsettings_p.h>

QT_BEGIN_NAMESPACE

class QQmlLintSettings : public QQmlToolingSettings
{
public:
    QQmlLintSettings(const QString &name = QLatin1String("qmllint"));
};

QT_END_NAMESPACE
#endif // QQMLLINTSETTINGS_P_H
