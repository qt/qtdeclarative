import Qt.test 1.0

MyQmlObject {
    property var c: Qt.binding(function() { return 2; })
    qjsvalue: Qt.binding(function() { return 2; })
}
