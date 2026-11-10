pragma Singleton
import QtQml

// Used the way tst_QmlCppCodegen::unqualifiedName uses the names, it produces "type".
QtObject {
    objectName: "type"
    function max(a: int, b: int): string { return "type" }
    function toString(): string { return "type" }
}
