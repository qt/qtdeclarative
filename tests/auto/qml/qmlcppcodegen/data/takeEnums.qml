// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml

QtObject {
    id: root

    // Calling through a property of a non-final type passes the arguments as variants, which
    // then have to be converted to the enumeration types.
    property TakeEnums target: TakeEnums {}

    function passEnums() {
        root.target.takeInt8(TakeEnums.Int8.Value)
        root.target.takeUInt8(TakeEnums.UInt8.Value)
        root.target.takeInt16(TakeEnums.Int16.Value)
        root.target.takeUInt16(TakeEnums.UInt16.Value)
        root.target.takeInt64(TakeEnums.Int64.Value)
    }

    function passNumbers(i8: int, u8: int, i16: int, u16: int, i64: int) {
        root.target.takeInt8(i8)
        root.target.takeUInt8(u8)
        root.target.takeInt16(i16)
        root.target.takeUInt16(u16)
        root.target.takeInt64(i64)
    }
}
