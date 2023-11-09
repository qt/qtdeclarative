import QtQuick
import QtQuick.Controls

Item {
    width: 400
    height: 150

    TextEdit {
        id: textEdit
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        readOnly: true
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignVCenter
        text: "Lorem ipsum dolor sit amet, consectetur adipisicing"
        states: [
            State {
                name: "multi-line"
                when: textEdit.lineCount > 1
                AnchorChanges {
                    target: textEdit
                    anchors.bottom: undefined
                }
                PropertyChanges {
                    target: textEdit
                    height: undefined
                }
            },
            State {
                name: "single-line"
                when: true
                AnchorChanges {
                    target: textEdit
                    anchors.bottom: { return textEdit.parent.bottom }
                }
            }
        ]
    }
}
