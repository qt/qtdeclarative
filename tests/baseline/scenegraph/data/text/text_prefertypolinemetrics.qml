import QtQuick 2.0

Item {
    width: 180
    height: 60

    Row {
        anchors.fill: parent
        Text {
            font.family: "Arial"
            font.pixelSize: 16
            textFormat: Qt.RichText
            text: "First line<br />Second line<br />Third line"
        }
        Text {
            font.family: "Arial"
            font.pixelSize: 16
            textFormat: Qt.RichText
            text: "First line<br />Second line<br />Third line"
            font.preferTypoLineMetrics: true
        }

    }
}
