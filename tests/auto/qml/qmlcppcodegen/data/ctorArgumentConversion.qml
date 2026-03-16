pragma Strict
import QtQml

QtObject {
    // double can be JS-coerced to int via QJSPrimitiveValue
    // We should not mistakenly use the copy ctor here.
    property foreignWithLength w: { return 4.2 }
}
