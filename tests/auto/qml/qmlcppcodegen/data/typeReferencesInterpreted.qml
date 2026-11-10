pragma ValueTypeBehavior: Addressable
import QtQml
import TestTypes

// Not strict, and the functions have untyped parameters, so the compiler doesn't compile them.
// A function without parameters would be compiled, even without type annotations.
// The counterpart of typeReferences.qml for tst_QmlCppCodegen::typeReferenceMixed.
QtObject {
    property var stored

    function writeRectInto(target) { target.stored = rect }
    function writePartyInto(target) { target.stored = BirthdayParty }
    function readOther(other) { return "" + other.stored }
    function otherIsRect(other) { return "" + (other.stored === rect) }
    function isRect(value) { return "" + (value === rect) }
    function isTypeWrapper(value) {
        return "" + (Object.prototype.toString.call(value) === "[object QMLTypeWrapper]")
    }
}
