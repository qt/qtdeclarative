import QtQuick

Item {
    id: main
    objectName: "main"
    width: 800
    height: 600

    FocusScope {
        objectName: "focusScope"

        Column {
            Rectangle {
                id: rectangle
                focus: true
                objectName: "rect"
                width: textInput.width
                height: textInput.height
                border.width: 1
                onActiveFocusChanged: textInput.forceActiveFocus()
            }

            TextInput {
                id: textInput
                objectName: "textInput"
                font.pixelSize: 40
                text: "focus me"
            }
        }
    }
}
