import QtQuick 2.15

Item {
    component Required : QtObject {
        property int x
        required x
    }
    Required { x: 5 }
}
