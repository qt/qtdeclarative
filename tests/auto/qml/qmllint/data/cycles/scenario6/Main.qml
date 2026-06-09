import QtQuick

Item {
    id: mainItem
    property CycleB cycleB
    property int mainData: 100

    function getMainData(): int {
        return mainData
    }

    function chainedCall(): int {
        return cycleB.intermediateCall() + getMainData()
    }
}
