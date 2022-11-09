import QtQuick
import QtQml.Models

Item {
    property var visualIndexBeforeMove: [-1, -1, -1]
    property var visualIndexAfterMove: [-1, -1, -1]

    DelegateModel {
        id: visualModel
        model: ListModel {
            ListElement { name: "Apple (id 0)" }
            ListElement { name: "Orange (id 1)" }
            ListElement { name: "Banana (id 2)" }
        }

        delegate: DropArea {
            id: delegateRoot

            required property string name
            // property that was not updated correctly in QTBUG-104047
            readonly property int visualIndex: DelegateModel.itemsIndex
            Rectangle {
                id: child

                readonly property int visualIndex: delegateRoot.visualIndex
            }
        }
    }

    Repeater {
        id: root
        model: visualModel
    }

    Component.onCompleted: {
        visualIndexBeforeMove[0] = root.itemAt(0).visualIndex
        visualIndexBeforeMove[1] = root.itemAt(1).visualIndex
        visualIndexBeforeMove[2] = root.itemAt(2).visualIndex
        visualModel.items.move(2, 0)
        // test that bindings on the QQmlDelegateModelAttached are captured properly:
        // after the move, the visualIndex property should be updated to the new value,
        // which in this case is also the index used in itemAt()
        visualIndexAfterMove[0] = root.itemAt(0).visualIndex
        visualIndexAfterMove[1] = root.itemAt(1).visualIndex
        visualIndexAfterMove[2] = root.itemAt(2).visualIndex
    }

}
