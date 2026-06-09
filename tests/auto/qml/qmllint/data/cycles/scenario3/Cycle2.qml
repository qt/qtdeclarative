import QtQuick

Item {
    property Cycle3 peer

    signal pingFromCycle2(int value)

    function relay(value: int): void {
        peer.pingFromCycle3(value)
    }
}
