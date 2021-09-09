import QtQml

QtObject {
    property QtObject there: QtObject {}
    property url here: Qt.resolvedUrl(somewhere, there)
    property url somewhere
}

