import QtQuick

Item {
    property Main main
    property int cycleAValue: 58

    function getValue(): int {
        return cycleAValue
    }

    function callBackToMain(): int {
        return main.getMainValue() + cycleAValue
    }
}
