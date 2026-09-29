import QtQuick

ListView {
    id: listView

    width: 320
    height: 480

    model: ListModel {
        id: listModel
        Component.onCompleted: {
            for (let i = 0; i < 50; ++i)
                listModel.append({ name: "item " + i, sectionStr: "section " + (i % 5) })
        }
    }

    property int sectionCount: 0

    section.property: "sectionStr"
    section.criteria: ViewSection.FullString
    section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart

    // The section delegate is bound, so section items reuse the ListView's own
    // context (see tst_QQuickListView::orphanedSectionContext()).
    section.delegate: Rectangle {
        width: listView.width
        height: 30
        color: "#E0E1D8"
        Text {
            anchors.centerIn: parent
            font.pixelSize: 12
            text: section
        }
        Component.onCompleted: ++listView.sectionCount
    }

    delegate: Item {
        width: listView.width
        height: 40
        Text { text: name }
    }
}
