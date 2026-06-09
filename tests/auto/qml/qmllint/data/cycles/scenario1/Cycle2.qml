import QtQuick

Item {
    property Cycle3 peer

    function methodFromCycle2(): int {
        return peer.methodFromCycle3() + 1
    }

    function callThroughCycle2(): int {
        return peer.callBackIntoCycle1()
    }
}
