import QtQuick 2.0

Rectangle {
    id: root
    width: 240
    height: 320

    property bool showHeader: false
    property bool showFooter: false

    Component {
        id: myDelegate
        Rectangle {
            id: wrapper
            objectName: "wrapper"
            width: 80
            height: 60
            border.width: 1
            Text { text: index }
            Text {
                x: 30
                text: wrapper.x + ", " + wrapper.y
                font.pixelSize: 12
            }
            Text {
                y: 20
                id: textName
                objectName: "textName"
                text: name
            }
            Text {
                y: 40
                id: textNumber
                objectName: "textNumber"
                text: number
            }

            property string theName: name
            color: GridView.isCurrentItem ? "lightsteelblue" : "white"
        }
    }

    Component {
        id: headerFooter
        Rectangle { width: 30; height: 320; color: "blue" }
    }

    GridView {
        objectName: "grid"
        width: 240
        height: 320
        cellWidth: 80
        cellHeight: 60
        flow: (testTopToBottom == false) ? GridView.LeftToRight : GridView.TopToBottom
        layoutDirection: (testRightToLeft == true) ? Qt.RightToLeft : Qt.LeftToRight
        verticalLayoutDirection: (testBottomToTop == true) ? GridView.BottomToTop : GridView.TopToBottom
        model: testModel
        delegate: myDelegate
        header: root.showHeader ? headerFooter : null
        footer: root.showFooter ? headerFooter : null
    }
}
