import QtQuick 2.13
import QtQml 2.13
import QtQml.Models 2.13

Rectangle {
    width: 640
    height: 480
    property var model1: null
    property var model2: null
    Component {
        id: m1
        ObjectModel {
            Rectangle { height: 30; width: 80; color: "red" }
            Rectangle { height: 30; width: 80; color: "green" }
            Rectangle { height: 30; width: 80; color: "blue" }
        }
    }
    Component {
        id: m2
        ObjectModel {
            Rectangle { height: 30; width: 80; color: "red" }
        }
    }
    ListView {
        anchors.fill: parent
        Component.onCompleted: {
            model1 = m1.createObject()
            model = model1
            model2 = m2.createObject()
            model = model2
            model1.destroy()
        }
    }
}
