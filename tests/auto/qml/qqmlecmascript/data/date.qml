import QtQuick 2.0
import Qt.test 1.0

Item {
    MyDateClass {
        id: mdc
    }

    function test_is_invalid_qtDateTime()
    {
        var dt = mdc.invalidDate();
        return isNaN(dt);
    }

    function test_is_invalid_jsDateTime()
    {
        var dt = new Date("");
        return isNaN(dt);
    }
}
