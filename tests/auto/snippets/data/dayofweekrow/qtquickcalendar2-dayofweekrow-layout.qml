import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Calendar 2.0

//! [1]
ColumnLayout {
    DayOfWeekRow {
        locale: view.locale
        Layout.fillWidth: true
    }

    CalendarView {
        id: view
        month: 12
        year: 2015
        locale: Qt.locale("en_US")
        Layout.fillWidth: true
    }
}
//! [1]
