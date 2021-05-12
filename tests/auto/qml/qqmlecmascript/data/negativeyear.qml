import QtQuick 2.0
import Qt.test 1.0

Item {
    // Use the second of January, to avoid any time-zone glitch pulling to previous year !
    function check_negative_tostring() {
        return "result: " + new Date(-2000, 0, 2);
    }

    function check_negative_toisostring() {
        return "result: " + (new Date(-2000, 0, 2)).toISOString();
    }
}
