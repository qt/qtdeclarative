import QtQml
import TestTypes

QtObject {
    // Cannot resolve QRectF as return type because TestTypes doesn't depend on QtQml
    // It still works when run through the interpreter.
    property rect r: Theme.area(Theme.BottomRight)
}
