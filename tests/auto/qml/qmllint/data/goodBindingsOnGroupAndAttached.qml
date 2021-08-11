import QtQuick
Rectangle {
    Text {
        font.pixelSize: 42
    }

    Keys.enabled: false
    Keys.onPressed: function(event) {
        console.log(event);
    }
}
