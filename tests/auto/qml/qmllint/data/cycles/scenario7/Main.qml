import QtQuick

Item {
    id: mainItem
    property Direct directCycle
    property Indirect indirectCycle
    property int mainValue: 50

    function directMethodCall(): int {
        return directCycle.getDirectValue()
    }

    function indirectMethodCall(): int {
        return indirectCycle.getIndirectValue()
    }

    function getMainValue(): int {
        return mainValue
    }
}
