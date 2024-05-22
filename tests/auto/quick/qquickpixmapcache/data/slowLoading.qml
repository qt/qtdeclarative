import QtQuick
import PixmapCacheTest

DeviceLoadingImage {
    id: root
    width: 240
    height: 240
    sourceSize.width: width
    sourceSize.height: height
    source: "image://slow/200"
    asynchronous: true
    retainWhileLoading: true
}
