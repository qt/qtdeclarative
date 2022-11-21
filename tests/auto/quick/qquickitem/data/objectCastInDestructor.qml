import QtQuick

Rectangle {
    width: 360
    height: 360

    Component {
        id: crashComponent

        Rectangle {
            objectName: "testRectangle"
            property bool customProperty: false
        }
    }

    Loader {
        id: loader
        objectName: "loader"
        anchors.fill: parent
        sourceComponent: crashComponent
    }
}

