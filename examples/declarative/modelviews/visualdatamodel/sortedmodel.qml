import QtQuick 2.0

Rectangle {
    width: 480; height: 640

    Component {
        id: numberDelegate

        Text {
            anchors { left: parent.left; right: parent.right }
            text: number

            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 18
        }
    }

    ListView {
        anchors {
            left: parent.left; top: parent.top;
            right: parent.horizontalCenter; bottom: button.top
            leftMargin: 2; topMargin: 2; rightMargin: 1; bottomMargin: 2
        }

        model: ListModel {
            id: unsortedModel
        }
        delegate: numberDelegate
    }
    ListView {
        anchors {
            left: parent.horizontalCenter; top: parent.top;
            right: parent.right; bottom: button.top
            leftMargin: 1; topMargin: 2; rightMargin: 2; bottomMargin: 2
        }
        model: VisualDataModel {
            model: unsortedModel
            delegate: numberDelegate

            onUpdated: {
                for (var i = 0; i < inserts.length; ++i) {
                    for (var j = inserts[i].start; j < inserts[i].end; ++j) {
                        console.log(j)
                        var number = unsortedModel.get(getItemInfo(j).index).number
                        var k = 0;
                        for (var l = 0; l < unsortedModel.count; ++l) {
                            if (l == inserts[k].start) {
                                l = inserts[k].end - 1
                                ++k
                            } else if (number < unsortedModel.get(getItemInfo(l).index).number) {
                                move(j, l, 1)
                                break
                            }
                        }
                        inserts[i].start += 1;
                    }
                }
            }
        }
    }

    Rectangle {
        id: button

        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 2 }
        height: moreText.implicitHeight + 4

        color: "black"

        Text {
            id: moreText

            anchors.fill: parent
            text: "More"
            color: "white"
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            anchors.fill: parent

            onClicked: unsortedModel.append({ "number": Math.floor(Math.random() * 100) })
        }
    }
}
