import QtQuick

Item {
    id: mainItem
    property CycleA cycleA
    property int mainValue: 42

    function getMainValue(): int {
        return mainValue
    }

    function computeSum(): int {
        return cycleA.getValue() + getMainValue()
    }
}
