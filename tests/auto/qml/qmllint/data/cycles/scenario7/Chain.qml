import QtQuick

Item {
    property Main main
    property int chainValue: 80

    function getChainValue(): int {
        return chainValue
    }

    function accessMainThroughChain(): int {
        return main.getMainValue() + chainValue
    }
}
