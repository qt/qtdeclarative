pragma Strict

import QtQml
import QtQuick

QtObject {
    id: root

    component Base : QtObject {
        property int i: 1
    }

    component Derived : Base {
        property string i: "a"
    }

    property Base base: Derived { }
    property var res: root.base?.i
}
