import QtQuick

TapHandler {
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    onSingleTapped: (eventPoint, button) => {
        console.log("Single tap at", eventPoint, "with button", button)
    }
    onTapped: console.log("tapped")
}
