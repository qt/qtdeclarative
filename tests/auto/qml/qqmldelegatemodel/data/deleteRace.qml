import QtQuick
import QtQml.Models

Item {
    DelegateModel {
        id: delegateModel
        model: ListModel {
            id: sourceModel

            ListElement { title: "foo" }
            ListElement { title: "bar" }

            function clear() {
                if (count > 0)
                    remove(0, count);
            }
        }

        groups: [
            DelegateModelGroup { name: "selectedItems" }
        ]

        delegate: Text {
            height: DelegateModel.inSelectedItems ? implicitHeight * 2 : implicitHeight
            Component.onCompleted: {
                if (index === 0)
                    DelegateModel.inSelectedItems = true;
            }
        }

        Component.onCompleted: {
            items.create(0)
            items.create(1)
        }
    }

    ListView {
        anchors.fill: parent
        model: delegateModel
    }

    Timer {
        running: true
        interval: 10
        onTriggered: sourceModel.clear()
    }

    property int count: delegateModel.items.count
}

