import QtQuick 2.0
import Qt.labs.calendar 1.0

//! [1]
MonthGrid {
    month: Calendar.December
    year: 2015
    locale: Qt.locale("en_US")
}
//! [1]
