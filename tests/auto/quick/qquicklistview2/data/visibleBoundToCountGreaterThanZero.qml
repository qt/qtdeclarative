import QtQuick
import QtQuick.Layouts

ColumnLayout {
    property alias listView: listView

    ListView {
        id: listView

        visible: count > 0 // actual defect. countChanged never fires so this never turns true

        Layout.fillWidth: true
        Layout.preferredHeight: contentHeight // grow with content, initially 0

        model: ListModel {
            id: idModel
        }

        delegate: Text {
            required property string name
            text: name
        }

        Timer {
            running: true
            interval: 10
            repeat: true
            onTriggered: idModel.append({name:"Hello"})
        }
    }
}
