import QtQml 2.0

QtObject {
    property int p1: 1
    property int p2: p1 + 1

    function resetToConstant() {
        p2 = 42;
    }

    function resetToNewBinding() {
        p2 = Qt.binding(function() { return p1 * 2 });
    }
}
