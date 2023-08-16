import QtQuick
import 'MyModuleName' as MyModuleName

Item {
    property var a: MyModuleName.Font.exampleVar
    property var b: Font.SmallCaps
    property var c: Font.exampleVar
}
