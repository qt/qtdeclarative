import QtQuick 2.13
import QtQuick.Window 2.13

Window {
    visible: true
    width: 640
    height: 640
    title: qsTr("Hello World")

    ListModelSort {
        id: lms
        anchors.fill: parent
    }

    function doSort() {
        lms.doSort()
    }

    function verify(): bool {
        let ok = lms.verify()
        return ok
    }
}
