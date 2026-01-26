import QtQml
import Enumerei

QtObject {
    property int a: EnumTester.S2
    property int b: EnumTester.U2
    property int c: EnumTester.Scoped.S2
    property int d: EnumTester.Unscoped.U2

    property int e: EnumTesterScoped.U2
    property int f: EnumTesterScoped.Scoped.S2
    property int g: EnumTesterScoped.Unscoped.U2
}
