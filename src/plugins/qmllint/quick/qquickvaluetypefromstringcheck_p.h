// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQUICKVALUETYPEFROMSTRINGCHECK_H
#define QQUICKVALUETYPEFROMSTRINGCHECK_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <private/qqmljsvaluetypefromstringcheck_p.h>

QT_BEGIN_NAMESPACE

class QQuickValueTypeFromStringCheck
{
public:
    static QQmlJSStructuredTypeError check(const QString &typeName, const QString &value);
};

QT_END_NAMESPACE

#endif // QQUICKVALUETYPEFROMSTRINGCHECK_H
