import QtQuick

Text {
    property bool testBool: false
    font.family: "Ar" + "iallll"
    onTestBoolChanged: font.pixelSize = 16;
}
