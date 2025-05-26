import QtQml

QtObject {
    objectName: "outer"
    component Inline: QtObject {
        objectName: "inner"
    }
}
