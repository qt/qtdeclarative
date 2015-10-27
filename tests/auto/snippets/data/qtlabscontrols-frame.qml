import QtQuick 2.0
import QtQuick.Layouts 1.0
import Qt.labs.controls 1.0

//! [1]
Frame {
    ColumnLayout {
        anchors.fill: parent
        CheckBox { text: qsTr("E-mail") }
        CheckBox { text: qsTr("Calendar") }
        CheckBox { text: qsTr("Contacts") }
    }
}
//! [1]
