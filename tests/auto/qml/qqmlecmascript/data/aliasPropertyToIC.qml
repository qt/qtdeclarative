import QtQml


QtObject {
    id: root

    component Test : QtObject {}

    property alias myalias: other
    property var direct: Test { id: other }
}
