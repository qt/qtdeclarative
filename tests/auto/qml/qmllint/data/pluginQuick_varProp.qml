import QtQuick
import QtQuick.Controls

Item {
    SpinBox {
        textFromValue: null
        valueFromText: { return 6; }
    }
    TableView {
        columnWidthProvider: null
        rowHeightProvider: { return 3; }
    }
    Tumbler {
        contentItem: Item {}
    }
}
