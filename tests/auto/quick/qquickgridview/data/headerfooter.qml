import QtQuick 2.0

GridView {
    id: view

    width: 240
    height: 320

    model: testModel

    header: Rectangle {
        objectName: "header"
        width: flow == GridView.TopToBottom ? 20 : view.width
        height: flow == GridView.TopToBottom ? view.height : 20
        color: "red"
    }
    footer: Rectangle {
        objectName: "footer"
        width: flow == GridView.TopToBottom ? 30 : view.width
        height: flow == GridView.TopToBottom ? view.height : 30
        color: "blue"
    }

    cellWidth: 80;
    cellHeight: 80;

    delegate: Text { width: 80; height: 80; text: index + "(" + x + ")" }
}
