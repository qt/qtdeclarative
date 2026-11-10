pragma Strict
pragma ValueTypeBehavior: Addressable
import QtQml
import QtQml as Q
import TestTypes

// For tst_QmlCppCodegen::typeReferences. A type reference stringifies to the name of the type, and
// stays a type wrapper if it is stored in a var.
QtObject {
    property bool flag: true
    property int count: 3

    function direct(): string { return "" + rect }
    function viaVar(): string { var t = rect; return "" + t }
    function readTwice(): string { var t = rect; var a = "" + t; var b = "" + t; return a + b }
    function branch(): string {
        var t = rect
        if (flag)
            return "" + t
        return "" + t
    }
    function loop(): string {
        var t = rect
        var result = ""
        for (var i = 0; i < count; ++i)
            result += t
        return result
    }

    function qmlType(): string { return "" + QtObject }
    function qmlTypeViaVar(): string { var t = QtObject; return "" + t }
    function cppType(): string { return "" + Person }
    function cppTypeViaVar(): string { var t = Person; return "" + t }
    function namespaced(): string { return "" + Q.QtObject }
    function namespacedViaVar(): string { var t = Q.QtObject; return "" + t }

    function composite(): string { return "" + Action }
    function compositeViaVar(): string { var t = Action; return "" + t }

    component IC: QtObject {}
    function inlineComponent(): string { return "" + IC }
    function inlineComponentViaVar(): string { var t = IC; return "" + t }

    function returnRect(): var { var t = rect; return t }
    function returnQtObject(): var { var t = QtObject; return t }
    function returnPerson(): var { var t = Person; return t }

    property var stored
    function storeAndRead(): string { stored = rect; return "" + stored }
    function storeTwice(): string { stored = rect; stored = Person; return "" + stored }
    function storeQtObjectThenPerson(): string {
        stored = QtObject
        stored = Person
        return "" + stored
    }

    // For tst_QmlCppCodegen::typeReferenceMixed. The same functions are in
    // typeReferencesInterpreted.qml, which the compiler doesn't compile. The compiler cannot
    // compare a var to a type or access members of a var, so it cannot check identity here.
    function writeRect(): void { stored = rect }
    function writeParty(): void { stored = BirthdayParty }
    function readOwn(): string { return "" + stored }
    function readOwnVar(): var { return stored }
    function equalSame(): string { return "" + (rect == rect) }
    function equalViaVar(): string { var t = rect; return "" + (t == rect) }
    function equalDifferentQml(): string { return "" + (rect == QtObject) }
    function equalDifferentCpp(): string { return "" + (QtObject == Person) }
    function strictEqualDifferent(): string { return "" + (rect === QtObject) }
}
