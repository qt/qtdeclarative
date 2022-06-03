import QtQuick 2.15
import QtQuick.Controls 2.15

ListView {
    id: root
    width: 320
    height: 480
    model: 100

    property var pressedDelegates: []
    property var releasedDelegates: []
    property var tappedDelegates: []
    property var canceledDelegates: []

    delegate: Button {
        required property int index
        objectName: text
        text: "button " + index
        height: 100
        width: 320

        onPressed: root.pressedDelegates.push(index)
        onReleased: root.releasedDelegates.push(index)
        onClicked: root.tappedDelegates.push(index)
        onCanceled: root.canceledDelegates.push(index)
    }
}
