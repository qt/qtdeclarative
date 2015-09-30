import QtQuick 2.0
import Qt.labs.calendar 1.0
import QtQuick.Controls 2.0
import QtQuick.Templates 2.0 as T

//! [1]
ListView {
    id: listview

    width: 200; height: 200
    snapMode: ListView.SnapOneItem
    orientation: ListView.Horizontal
    highlightRangeMode: ListView.StrictlyEnforceRange

    model: CalendarModel {
        from: new Date(2015, 0, 1)
        to: new Date(2015, 11, 31)
    }

    delegate: CalendarView {
        width: listview.width
        height: listview.height

        month: model.month
        year: model.year
        locale: Qt.locale("en_US")
    }

    T.ScrollIndicator.horizontal: ScrollIndicator { }
}
//! [1]
