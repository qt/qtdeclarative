import QtQuick

Item {
    property Cycle1 peer

    function methodFromCycle3(): int {
        return 1
    }

    function callBackIntoCycle1(): int {
        return peer.methodFromCycle1()
    }
}
