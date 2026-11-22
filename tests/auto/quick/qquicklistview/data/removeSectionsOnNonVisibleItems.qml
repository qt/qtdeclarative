import QtQuick

ListView {
    id: listView

    ListModel {
        id: listModel
        Component.onCompleted: reloadModel()
    }

    property list<Item> sectionItems
    property int sectionType: 0
    property int sectionCount: 0

    function reloadModel() {
        ++listView.sectionType
        listModel.clear()
        for (let i = 0; i < 50; ++i) {
            listModel.append({sectionStr: listView.sectionType + ":" + listView.sectionCount++})
        }
        listView.sectionCount = 0
    }

    width: 640
    height: 480
    spacing: 24
    boundsBehavior: Flickable.DragOverBounds
    displayMarginBeginning: 100
    displayMarginEnd: 100
    contentWidth: listView.width

    model: listModel

    section.property: "sectionStr"
    section.criteria: ViewSection.FullString
    section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart
    section.delegate: Item {
        id: item
        property string sectionData: section
        width: listView.width
        height: 30
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: textDate.width + 20
            height: 16
            radius: 8
            color: "#E0E1D8"
            Text {
                id: textDate
                anchors.centerIn: parent
                font.pixelSize: 10
                text: item.sectionData
            }
        }
        Component.onCompleted: listView.sectionItems.push(item)
        Component.onDestruction: {
            let newSectionItems = []
            for (let index = 0; index < listView.sectionItems.length; index++) {
                if (listView.sectionItems[index] !== null && listView.sectionItems[index] != item)
                    newSectionItems.push(listView.sectionItems[index])
            }
            listView.sectionItems = newSectionItems
        }
    }

    delegate: Item { width: listView.width; height: 10 + index }
}
