import QtQuick.Templates as T
T.Button {
    id: control
    objectName: "FileSystemStyle"
    contentItem: T.Label {
        text: control.text
        color: "#0000ff"
    }
}
