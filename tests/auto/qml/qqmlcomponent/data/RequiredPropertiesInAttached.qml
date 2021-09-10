import qt.test 1.0
import QtQml
QtObject {
    AttachedRequiredProperty.index: -1
    AttachedRequiredProperty.name: "minus one"
} // error: we don't support required attached properties
