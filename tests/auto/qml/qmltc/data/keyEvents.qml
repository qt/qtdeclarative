import QtQuick
Rectangle {
    property string k: ""
    Keys.onPressed: function(event) {
        if (event.key == Qt.Key_Plus) {
            k = "+";
        } else if (event.key == Qt.Key_Slash) {
            k = "/";
        } else {
            k = "unknown";
        }
    }
}
