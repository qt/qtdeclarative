import QtQuick 2.0
import QtQuick.Extras 2.0

Tumbler {
    id: tumbler
    model: 5

    Column {
        Repeater {
            model: 3

            Rectangle {
                width: tumbler.contentItem.width
                height: tumbler.contentItem.height / 3
                color: 'transparent'
                border.color: 'red'
            }
        }
    }
}
