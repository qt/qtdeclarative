import QtQuick
import QtQuick.Controls

Rectangle {
    width: 640
    height: 480
    color: "#FFFFFF"
    ListView {
        objectName: "list"
        anchors.fill: parent

        delegate: Rectangle {
            objectName: value
            implicitHeight: text.implicitHeight
            color: "#ff3"

            Text {
                id: text
                width: parent.width
                padding: 5
                font.pixelSize: 20
                text: value
            }
        }

        section {
            property: "section"

            delegate: Rectangle {
                objectName: section
                width: parent.width
                implicitHeight: text.implicitHeight
                color: "#3ff"

                Text {
                    id: text
                    width: parent.width
                    padding: 5
                    font.pixelSize: 20
                    text: section
                    wrapMode: Text.Wrap
                }
            }
        }

        model: ListModel {
            ListElement { value: "Element1"; section: "Section1" }
            ListElement { value: "Element2"; section: "Section1" }
            ListElement { value: "Element3"; section: "Section1" }
            ListElement { value: "Element4"; section: "Section2" }
            ListElement { value: "Element5"; section: "Section2" }
            ListElement { value: "Element6"; section: "Section2" }
            ListElement { value: "Element7"; section: "Section2" }
            ListElement { value: "Element8"; section: "Section3" }
            ListElement { value: "Element9"; section: "Section3" }
        }
    }
}
