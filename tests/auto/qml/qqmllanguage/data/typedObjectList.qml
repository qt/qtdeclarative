import QtQml

QtObject {
    property var b;
    property Component c: QtObject {}

    // In 6.5 and earlier we don't have heap-managed QQmlListProperty, yet.
    property list<Component> ll;

    function returnList(a: Component) : list<Component> { ll.push(a); return ll; }

    Component.onCompleted: b = { b: returnList(c) }
}
