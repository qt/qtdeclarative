import QtQuick

Item {
    property Cycle2 peer

    function methodFromCycle1(): int {
        return 41
    }

    function callThroughCycle(): int {
        return peer.callThroughCycle2()
    }
}
