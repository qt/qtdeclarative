import QtQuick 2.12

Item {
    id: root
    anchors.fill: parent
    width: 100
    height: 100
    property bool success: checkValue === aliasUser.topMargin
    property int checkValue: 42
    Rectangle {
        id: myItem
        objectName: "myItem"
        color: "blue"
        anchors.topMargin: root.checkValue
        width: 50
        height: 50
        Text {text: "source:\n" + myItem.anchors.topMargin}
    }

    Rectangle {
        property alias topMargin: myItem.anchors.topMargin
        id: aliasUser
        objectName: "aliasUser"
        color: "red"
        anchors.left: myItem.right
        width: 50
        height: 50
        Text {objectName: "myText"; text: "alias:\n" + aliasUser.topMargin}
    }
}

