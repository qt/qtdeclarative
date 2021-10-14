import QtQuick

ListView {
    id: listView
    width: 240
    height: 300

    model: ListModel {
        ListElement { section: "section 1" }
        ListElement { section: "section 1" }
        ListElement { section: "section 1" }
        ListElement { section: "section 2" }
        ListElement { section: "section 2" }
        ListElement { section: "section 2" }
        ListElement { section: "section 3" }
        ListElement { section: "section 3" }
        ListElement { section: "section 3" }
        ListElement { section: "section 4" }
        ListElement { section: "section 4" }
        ListElement { section: "section 4" }
        ListElement { section: "section 5" }
        ListElement { section: "section 5" }
        ListElement { section: "section 5" }
    }

    section.property: "section"
    section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart
    section.delegate: Rectangle {
        width: listView.width
        height: 30
        Text {
            anchors.fill: parent
            text: section
        }
        color: "lightblue"
    }

    snapMode: ListView.SnapToItem

    delegate: Rectangle {
        width: listView.width
        height: 30
        Text {
            anchors.fill: parent
            text: index
        }
        border {
            width: 1
            color: "black"
        }
    }
}
