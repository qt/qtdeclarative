import QtQuick

Rectangle {
    width: 240
    height: 320

    ListView {
        id: list
        objectName: "list"
        // deliberately unsized with currentIndex set
        currentIndex: 2

        model: ListModel {
            ListElement { name: "A0"; section: "A" }
            ListElement { name: "A1"; section: "A" }
            ListElement { name: "A2"; section: "A" }
            ListElement { name: "B0"; section: "B" }
            ListElement { name: "B1"; section: "B" }
            ListElement { name: "C0"; section: "C" }
            ListElement { name: "C1"; section: "C" }
        }

        delegate: Item {
            objectName: "wrapper"
            required property string name
            width: ListView.view ? ListView.view.width : 0
            height: 20
        }

        section.property: "section"
        section.criteria: ViewSection.FullString
        section.delegate: Rectangle {
            id: sectionDelegate
            objectName: "sect_" + section
            required property string section
            width: list.width
            height: 20
        }
    }
}
