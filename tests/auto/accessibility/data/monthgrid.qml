import QtQuick 2.5
import QtQuick.Window 2.2
import Qt.labs.calendar 1.0

Window {
    visible: true

    MonthGrid {
        id: monthgrid
        objectName: "monthgrid"
        title: "MonthGrid"
        Accessible.name: title
    }
}
