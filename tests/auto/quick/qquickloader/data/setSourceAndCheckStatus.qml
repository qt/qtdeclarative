import QtQuick 2.12
import QtQuick.Window 2.12

Window {
    id: root
    visible: true; width: 640; height: 480
    Loader {
        id: loader
        anchors.fill: parent
        function load(url) {
            loader.setSource(url)
        }
    }
}
