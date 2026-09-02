import QtQuick

TextWithAssignedFont {
    // Note: we cannot re-assign the font here as it produces an error in
    // QQmlComponent

    // font: TestApplication.createDummyFont()
    font.pixelSize: 22
}
