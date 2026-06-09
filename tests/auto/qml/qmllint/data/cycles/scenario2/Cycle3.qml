import QtQuick

Item {
    property Cycle1 peer
    property int valueFromCycle3: peer.baseValue + 2
}
