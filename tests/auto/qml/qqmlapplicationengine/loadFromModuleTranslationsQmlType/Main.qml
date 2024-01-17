import QtQml

QtObject {
    property string expected: "placeholder"
    property string actual: qsTr("Hello")

    function f() {
        if (expected === actual)
            Qt.exit(0)
        else
            Qt.exit(actual.charCodeAt(0))
    }
}
