import QtQml
import Enumerei

QtObject {
    property int a: EnumTester.S2
    property int b: EnumTester.U2
    property int c: EnumTesterScoped.U2

    property int d: EnumTester.Scoped.S2
    property int e: EnumTesterScoped.Scoped.S2
}
