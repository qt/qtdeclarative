import QtQuick
import QtQuick.Controls

Item {
    SpinBox {
        textFromValue: function(value, locale) { return Number(value).toLocaleString(locale, 'f', 0); }
        valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text); }
    }
    TableView {
        columnWidthProvider: function(column) { return column*50; }
        rowHeightProvider: function(row) { return row*3; }
    }
    Tumbler {
        contentItem: PathView {}
    }
}
