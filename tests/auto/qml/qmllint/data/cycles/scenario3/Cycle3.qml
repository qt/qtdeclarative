import QtQuick

Item {
    property Cycle1 peer

    signal pingFromCycle3(int value)

    function relay(value: int): void {
        peer.pingFromCycle1(value)
    }
}
