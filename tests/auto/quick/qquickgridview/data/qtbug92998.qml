import QtQuick 2.12
import QtQuick.Window 2.12

Item {
    width: 450
    height: 650
    GridView {
        objectName: "gridview"
        id: gridView
        width: 450
        height: 650
        layoutDirection: Qt.RightToLeft
        property int cells: calcCells(width)
        cellWidth: width / cells
        cellHeight: cellWidth

        delegate: Component {
            Item {
                width: gridView.cellWidth
                height: gridView.cellHeight
                Rectangle {
                    anchors {
                        fill: parent
                        margins: 10
                    }
                    color: "green"
                }
            }
        }
        model: [
           { number: "1" },
           { number: "2" },
           { number: "3" },
           { number: "4" },
           { number: "5" },
           { number: "6" },
           { number: "7" },
           { number: "8" },
           { number: "9" },
           { number: "10" },
           { number: "11" },
           { number: "12" },
           { number: "13" },
           { number: "14" },
           { number: "15" },
           { number: "16" }];
        function calcCells(w) {
            var rw = 120;
            var c = Math.max(1, Math.round(w / rw));
            return c;
        }
    }
}
