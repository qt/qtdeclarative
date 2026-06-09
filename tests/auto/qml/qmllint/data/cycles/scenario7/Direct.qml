import QtQuick

Item {
    property Main main
    property int directValue: 60

    function getDirectValue(): int {
        return directValue
    }

    function callMainDirect(): int {
        return main.getMainValue() + directValue
    }
}
