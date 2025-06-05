import QtQml

QtObject {
    enum QmlEnum { A, B, C }
    function f(a: EnumsAreNotTypes_functionAnnotations.QmlEnum)
        : EnumsAreNotTypes_functionAnnotations.QmlEnum { return 1 }
}
