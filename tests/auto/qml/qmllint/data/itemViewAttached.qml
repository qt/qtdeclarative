import QtQuick

Item {

    ListView {
        delegate: Text {
            text: ListView.view.highlightResizeDuration
        }
    }

    GridView {
        delegate: Text {
            text: GridView.view.cellHeight
        }
    }
}
