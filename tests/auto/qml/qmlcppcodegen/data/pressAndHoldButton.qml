import QtQuick

Image {
    id: container

    property int repeatDelay: 300
    property int repeatDuration: 75
    property bool pressed: false

    signal clicked

    scale: pressed ? 0.9 : 1

    function press() {
        autoRepeatClicks.start();
    }

    function release() {
        autoRepeatClicks.stop()
        container.pressed = false
    }

    ParallelAnimation on pressed {
        id: autoRepeatClicks
        running: false

        PropertyAction { target: container; property: "pressed"; value: true }
        ScriptAction { script: container.clicked() }
        PauseAnimation { duration: container.repeatDelay }

        ParallelAnimation {
            loops: Animation.Infinite
            ScriptAction { script: container.clicked() }
            PauseAnimation { duration: container.repeatDuration }
        }
    }
}

