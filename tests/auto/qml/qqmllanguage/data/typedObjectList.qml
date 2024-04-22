import QtQml

QtObject {
    property var b;
    property Component c: QtObject {}

    function returnList(a: Component) : list<Component> { return [a] }

    Component.onCompleted: b = { b: returnList(c) }
}
