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

    property int resultViaCycle: c1.callThroughCycle()
    property int resultViaIntermediate: c1.peer.methodFromCycle2()
}
