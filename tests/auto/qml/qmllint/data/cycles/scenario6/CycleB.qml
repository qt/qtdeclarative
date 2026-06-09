import QtQuick

Item {
    property CycleC cycleC
    property int bValue: 200

    function intermediateCall(): int {
        return cycleC.getCValue() + bValue
    }

    function callMainFromChain(): int {
        return cycleC.backToMain().getMainData()
    }
}
