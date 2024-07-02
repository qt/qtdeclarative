// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import resolveExpressionType.CppTypes as RETM

RETM.TypeWithAttachedAndEnum {
    sameAsAttached: 42
    property var someAttachedProperty: RETM.TypeWithAttachedAndEnum.sameAsAttached
    property var someEnum: RETM.TypeWithAttachedAndEnum.HELLO
}
