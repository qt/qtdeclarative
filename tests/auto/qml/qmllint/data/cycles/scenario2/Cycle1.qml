import QtQuick

Item {
    property Cycle2 peer
    property int baseValue: 12
    property int valueFromCycle2ThroughCycle3: peer.peer.valueFromCycle3
}
