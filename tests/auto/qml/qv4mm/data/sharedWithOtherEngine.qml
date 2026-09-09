import QtQml

QtObject {
    property var payload: ({ marker: "created in the owning engine" })
    property var stored
    function store(value) { stored = value }
}
