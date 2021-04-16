import QtQuick 2.15
import QtQml.Models 2.15

Item {
    height: 200
    width: 100
    DelegateModel {
        id: dm
        model: 2
        delegate: Item {
            width: 100; height: 20
            property bool isCurrent: ListView.isCurrentItem
        }
    }
    ListView {
        objectName: "listView"
        model: dm
        currentIndex: 1
        anchors.fill: parent
    }
}
