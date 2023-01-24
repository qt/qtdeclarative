import QtQuick 2.15

Item {
    component C: Item {}

    property int a
    property bool b
    property C c
    property var d
    property list<int> e

    component D: Item {}
    C {id: firstC}D{id: firstD}
    C {}   D{}
    C {
        C{}
        C{
            C {}
            C {}
            C {}
        }
        C{}
    }

}
