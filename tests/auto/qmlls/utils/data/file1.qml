import QtQuick 2.15

Item {
    component C: Item {}

    property int a
    property bool b
    property C c
    property var d
    property list<int> e

    component D: Item { id: icid }
    C {id: firstC}D{id: firstD}
    C { id: secondC }   D{ id: secondD}
    C {
        C{}
        C{
            C {}
            C {}
            C {}
        }
        C{}
    }

    component IC: Item { property C myC }

}
