import QtQuick 2.0

Item {
    id: root
    function self() { return root }

    property QtObject foo: self()
    property real foox: (foo as Item).x
}
