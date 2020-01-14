import QtQuick 2.0

Item {
    property QtObject foo: Item { x: 4 }
    property real foox: (foo as Item).rrr
}
