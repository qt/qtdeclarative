import QtQuick 1.1

SequentialAnimation {
    id: animation
    running: true
    ScriptAction { script: animation.paused = true }
}
