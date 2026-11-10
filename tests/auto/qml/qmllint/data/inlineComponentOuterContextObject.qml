pragma ComponentBehavior: Bound

import QtQml

QtObject {
    property int outerProperty: 42

    component Inner : QtObject {
        function getOuterProperty(): int { return outerProperty }
    }
}
