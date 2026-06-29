import QtQuick

ListView {
    id: list
    objectName: "list"
    width: 240
    height: 320

    model: ListModel {
        ListElement { name: "A1"; type: "A" }
        ListElement { name: "A2"; type: "A" }
        ListElement { name: "A3"; type: "A" }
        ListElement { name: "B1"; type: "B" }
        ListElement { name: "B2"; type: "B" }
        ListElement { name: "B3"; type: "B" }
    }

    delegate: Rectangle {
        required property string name
        objectName: "item_" + name
        activeFocusOnTab: true
        width: list.width
        height: 30
        color: activeFocus ? "lightblue" : "white"
        Text { anchors.centerIn: parent; text: parent.name }
    }

    section.property: "type"
    section.delegate: Rectangle {
        required property string section
        objectName: "section_" + section
        activeFocusOnTab: true
        width: list.width
        height: 20
        color: activeFocus ? "steelblue" : "gray"
        Text { anchors.centerIn: parent; text: parent.section }
    }
}
