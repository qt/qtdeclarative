import QtQuick

Item {
    property Chain chainRef
    property int indirectValue: 70

    function getIndirectValue(): int {
        return indirectValue
    }

    function accessChain(): int {
        return chainRef.getChainValue() + indirectValue
    }
}
