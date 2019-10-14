import QtQuick 2.0

AnimatedImage {
    property int currentFrameChangeCount: 0
    property int frameChangeCount: 0
    source: "stickman.gif"
    onCurrentFrameChanged: if (currentFrame > 0) ++currentFrameChangeCount;
    onFrameChanged: if (currentFrame > 0) ++frameChangeCount;
    function scriptedSetCurrentFrame(frame) {
        currentFrame = frame;
    }
}

