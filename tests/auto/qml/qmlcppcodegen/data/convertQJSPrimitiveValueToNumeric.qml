pragma Strict

import QtQuick
import TestTypes

Moo485 {
    // 1 + undefined -> QJSPrimitiveValue
    u8: 1 + undefined
    i8: 1 + undefined
    u16: 1 + undefined
    i16: 1 + undefined
    u32: 1 + undefined
    i32: 1 + undefined
    // 64 bit integers are not considered primitive
    f: 1 + undefined
    r: 1 + undefined
    d: 1 + undefined
}
