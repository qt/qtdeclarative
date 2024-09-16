// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import resolveExpressionType.CppTypes

TypeWithAttachedAndEnum {
    sameAsAttached: 42
    property var someAttachedProperty: TypeWithAttachedAndEnum.sameAsAttached
    property var someEnum: TypeWithAttachedAndEnum.Hello
}
