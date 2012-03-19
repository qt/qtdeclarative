import QtQuick 2.0

GridView {
    id: view
    property bool horizontal: false
    property bool rtl: false
    width: 240
    height: 320

    model: testModel
    
    flow: horizontal ? GridView.TopToBottom : GridView.LeftToRight
    header: Rectangle {
        objectName: "header"
        width: horizontal ? 20 : view.width
        height: horizontal ? view.height : 20
        color: "red"
    }
    footer: Rectangle {
        objectName: "footer"
        width: horizontal ? 30 : view.width
        height: horizontal ? view.height : 30
        color: "blue"
    }

    cellWidth: 80;
    cellHeight: 80;

    delegate: Text { width: 80; height: 80; text: index + "(" + x + ")" }
    layoutDirection: rtl ? Qt.RightToLeft : Qt.LeftToRight
}
