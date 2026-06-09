import QtQuick

Item {
    id: root

    Cycle1 {
        id: c1
        peer: c2

        function onPingFromCycle1(value: int): void {
            root.seen1 = value
        }
    }

    Cycle2 {
        id: c2
        peer: c3

        function onPingFromCycle2(value: int): void {
            root.seen2 = value
        }
    }

    Cycle3 {
        id: c3
        peer: c1

        function onPingFromCycle3(value: int): void {
            root.seen3 = value
        }
    }

    property int seen1: 0
    property int seen2: 0
    property int seen3: 0

    Component.onCompleted: {
        c1.relay(11)
        c2.relay(12)
        c3.relay(13)
    }
}
