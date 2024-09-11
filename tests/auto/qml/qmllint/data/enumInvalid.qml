import QtQml
import Enumerei

QtObject {
    property bool c: Qt.red > 4
    property bool d: Qt.red < 2

    property int e: EnumTester.Unscoped.U2
    property int f: EnumTesterScoped.Unscoped.U2
    property int g: EnumTesterScoped.S2
}
