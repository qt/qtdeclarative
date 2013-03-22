import QtQml 2.0

QtObject {
    property string originalName
    property string originalVersion
    property string currentName: Qt.application.name
    property string currentVersion: Qt.application.version
    Component.onCompleted: {
        originalName = Qt.application.name
        originalVersion = Qt.application.version
        Qt.application.name = "Test B"
        Qt.application.version = "0.0B"
    }
}
