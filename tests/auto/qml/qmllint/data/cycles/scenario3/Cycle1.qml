import QtQuick

Item {
    property Cycle2 peer

    signal pingFromCycle1(int value)

    function relay(value: int): void {
        peer.pingFromCycle2(value)
    }
}
