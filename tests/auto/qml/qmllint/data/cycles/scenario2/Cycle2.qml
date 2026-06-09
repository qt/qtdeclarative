import QtQuick

Item {
    property Cycle3 peer
    property int valueFromCycle2: peer.peer.baseValue + 1
}
