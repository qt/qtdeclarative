import QtQuick
Rectangle {
    Keys.onBogusSignal: function(event) {
        // the handler is good, but the signal doesn't exist
        console.log(event);
    }
}
