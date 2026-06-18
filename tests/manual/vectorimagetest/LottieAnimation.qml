import QtQuick
import Qt.labs.lottieqt
import QtQuick.VectorImage

Item {
    width: lottieAnimation.width * (VectorImageManager.scale / 10.0)
    height: lottieAnimation.height * (VectorImageManager.scale / 10.0)
    scale: VectorImageManager.scale / 10.0
    transformOrigin: Item.TopLeft

    Image {
        source: "background.png"
        fillMode: Image.Tile
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
        scale: 1.0 / parent.scale
        width: parent.width
        height: parent.height
        transformOrigin: Item.TopLeft
    }

    LottieAnimation {
        id: lottieAnimation
        textureSize: Qt.size(width * parent.scale, height * parent.scale)
        source: VectorImageManager.currentSource.toString().endsWith("json") ? VectorImageManager.currentSource : ""
        loops: VectorImageManager.looping ? LottieAnimation.Infinite : 1
        autoPlay: false
        onStatusChanged: {
            if (status === LottieAnimation.Ready)
                connections.onCurrentTimeChanged(VectorImageManager.currentTime)
        }
    }

    Connections {
        id: connections
        target: VectorImageManager

        function onCurrentTimeChanged(time) {
            var frame = ((time / 100.0) * (lottieAnimation.endFrame - lottieAnimation.startFrame) + lottieAnimation.startFrame)
            lottieAnimation.gotoAndStop(frame)
        }
    }
}
