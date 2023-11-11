import QtQuick

Item {
    width: 600
    height: 600

    ListModel {
        id: cursorsModel
        ListElement { cursorShape: Qt.ArrowCursor; text: "Arrow" }
        ListElement { cursorShape: Qt.UpArrowCursor; text: "UpArrow" }
        ListElement { cursorShape: Qt.CrossCursor; text: "Cross" }
        ListElement { cursorShape: Qt.WaitCursor; text: "Wait" }
        ListElement { cursorShape: Qt.IBeamCursor; text: "IBeam" }
        ListElement { cursorShape: Qt.SizeVerCursor; text: "SizeVer" }
        ListElement { cursorShape: Qt.SizeHorCursor; text: "SizeHor" }
        ListElement { cursorShape: Qt.SizeBDiagCursor; text: "SizeBDiag" }
        ListElement { cursorShape: Qt.SizeFDiagCursor; text: "SizeFDiag" }
        ListElement { cursorShape: Qt.SizeAllCursor; text: "SizeAll" }
        ListElement { cursorShape: Qt.BlankCursor; text: "Blank" }
        ListElement { cursorShape: Qt.SplitVCursor; text: "SplitV" }
        ListElement { cursorShape: Qt.SplitHCursor; text: "SplitH" }
        ListElement { cursorShape: Qt.PointingHandCursor; text: "PointingHand" }
        ListElement { cursorShape: Qt.ForbiddenCursor; text: "Forbidden" }
        ListElement { cursorShape: Qt.WhatsThisCursor; text: "WhatsThis" }
        ListElement { cursorShape: Qt.BusyCursor; text: "Busy" }
        ListElement { cursorShape: Qt.OpenHandCursor; text: "OpenHand" }
        ListElement { cursorShape: Qt.ClosedHandCursor; text: "ClosedHand" }
        ListElement { cursorShape: Qt.DragCopyCursor; text: "DragCopy" }
        ListElement { cursorShape: Qt.DragMoveCursor; text: "DragMove" }
        ListElement { cursorShape: Qt.DragLinkCursor; text: "DragLink" }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: flow.height
        Flow {
            id: flow
            width: parent.width
            Repeater {
                model: cursorsModel
                Rectangle {
                    id: root
                    color: "white"
                    border.width: 5
                    border.color: "black"

                    width: 200
                    height: 200

                    Text {
                        id: textItem
                        anchors.fill: parent
                        anchors.margins: parent.width * 0.1
                        text: model.text
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 10; font.pixelSize: height
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: model.cursorShape
                    }
                }
            }
        }
    }
}
