import QtQuick 2.12

Item {
    width: 30
    height: 30
    Loader {
        anchors.fill: parent
        sourceComponent: Text {
            rightPadding: 30
            text: "Some text"
            elide: Text.ElideRight
        }
    }
}
