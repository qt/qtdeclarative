import QtQuick 2.0
import QtQuick.Layouts 1.0
import Qt.labs.calendar 1.0

//! [1]
RowLayout {
    WeekNumberColumn {
        month: view.month
        year: view.year
        locale: view.locale
        Layout.fillHeight: true
    }

    CalendarView {
        id: view
        month: 11
        year: 2015
        locale: Qt.locale("en_US")
        Layout.fillHeight: true
    }
}
//! [1]
