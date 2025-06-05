import QtQml
import StaticTest

QtObject {
    enum QmlEnum { A, B, C }

    function f1(a: EnumTypeAnnotations.QmlEnum) {}
    function f2() : EnumTypeAnnotations.QmlEnum {}
    function f3(a: EnumTypeAnnotations.QmlEnum) : EnumTypeAnnotations.QmlEnum {}
    function f4(a: real, b: EnumTypeAnnotations.QmlEnum, c: bool) : void {}

    function f5(a: CppEnum.Scoped) {}
    function f6() : CppEnum.Scoped {}
    function f7(a: CppEnum.Unscoped) : CppEnum.Unscoped {}
    function f8(a: real, b: CppEnum.Unscoped, c: bool) : void {}

    function f9(a: EnumNamespace.Scoped) {}
    function f10() : EnumNamespace.Scoped {}
    function f11(a: EnumNamespace.Unscoped) : EnumNamespace.Unscoped {}
    function f12(a: real, b: EnumNamespace.Unscoped, c: bool) : void {}

    function f13(a: Qt.ColorScheme) {}
}
