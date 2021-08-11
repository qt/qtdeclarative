import QtQuick
Text {
    font.pixelSize: 10 + pointSize * 0.1 // pointSize does not exist in the scope of the binding
}
