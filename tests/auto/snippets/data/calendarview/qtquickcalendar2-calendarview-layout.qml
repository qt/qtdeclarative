import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Calendar 2.0

//! [1]
GridLayout {
    columns: 2

    DayOfWeekRow {
        locale: view.locale

        Layout.column: 1
        Layout.fillWidth: true
    }

    WeekNumberColumn {
        month: view.month
        year: view.year
        locale: view.locale

        Layout.fillHeight: true
    }

    CalendarView {
        id: view
        month: 12
        year: 2015
        locale: Qt.locale("en_US")

        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
//! [1]
