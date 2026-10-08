// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml
import CppShadowsQml

QtObject {
    id: self
    property Shadowed typed: Shadowed {}
    property QtObject untyped: Shadowed {}
    property string fromTyped: typed.origin
    property string fromCast: (untyped as Shadowed).origin
    property string fromArg: read(typed)
    property string fromSelf: self.typed.origin

    function read(o: Shadowed): string { return o.origin }
}
