import QtQuick 2.0
import QtQuick.Layouts 1.0
import Qt.labs.calendar 1.0

//! [1]
ColumnLayout {
    DayOfWeekRow {
        locale: grid.locale
        Layout.fillWidth: true
    }

    MonthGrid {
        id: grid
        month: Calendar.December
        year: 2015
        locale: Qt.locale("en_US")
        Layout.fillWidth: true
    }
}
//! [1]
