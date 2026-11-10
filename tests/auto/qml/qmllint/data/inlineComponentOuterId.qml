pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: root

    property QtObject outer: QtObject {
        id: outerId
        property int value: 42
    }

    component Inner : QtObject {
        function getOuter(): QtObject { return outerId }
    }
}
