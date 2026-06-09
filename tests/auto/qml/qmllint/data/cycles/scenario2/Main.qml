import QtQuick

Item {
    Cycle1 {
        id: c1
        peer: c2
    }

    Cycle2 {
        id: c2
        peer: c3
    }

    Cycle3 {
        id: c3
        peer: c1
    }

    property int sumFromCyclicProperties: c1.valueFromCycle2ThroughCycle3 + c2.valueFromCycle2
}
