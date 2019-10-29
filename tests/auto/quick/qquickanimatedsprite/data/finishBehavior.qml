import QtQuick 2.15

Rectangle {
    color: "black"
    width: 320
    height: 320

    AnimatedSprite {
        objectName: "sprite"
        loops: 1
        source: "squarefacesprite.png"
        frameCount: 6
        frameDuration: 64
        width: 160
        height: 160
        finishBehavior: AnimatedSprite.FinishAtFinalFrame
    }
}
