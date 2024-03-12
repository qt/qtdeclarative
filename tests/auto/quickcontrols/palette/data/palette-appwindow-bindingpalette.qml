import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 500
    height: 300
    palette: customPalette

    property alias cstmPalette: customPalette

    Palette { id: customPalette }

    Component.onCompleted: { window.palette.buttonText = "white" }
}
