import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Calendar 2.0

//! [1]
RowLayout {
    WeekNumberColumn {
        month: 12
        year: 2015
        locale: view.locale
        Layout.fillHeight: true
    }

    CalendarView {
        id: view
        month: 12
        year: 2015
        locale: Qt.locale("en_US")
        Layout.fillHeight: true
    }
}
//! [1]
