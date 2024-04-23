import QtQuick

Item {
    id: root
    width: 400
    height: 600

    ListView {
        id: rawList
        objectName: "list"
        anchors.top:    parent.top
        anchors.left:   parent.left
        anchors.right:  parent.right
        height: 300

        // full disabling of automatic viewport positioning
        highlightFollowsCurrentItem: false
        snapMode: ListView.NoSnap
        highlightRangeMode: ListView.NoHighlightRange

        delegate: Rectangle {
            color: model.index === rawList.currentIndex ? "red" : "white"
            border.color: rawList.currentItem === this ? "red" : "black"
            height: 100
            width: 400

            Text {
                anchors.centerIn: parent
                text: model.index
                font.pixelSize: 50
            }

            MouseArea {
                // only for using this file to do manual testing
                // autotest calls setCurrentIndex
                anchors.fill: parent

                onClicked: {
                    rawList.currentIndex = model.index;
                }
            }
        }

        model: 30
    }

}
