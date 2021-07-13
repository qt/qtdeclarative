import QtQml 2.0

QtObject {
    property int counter: 0
    property var f: function() {
        counter++;
    }
}
