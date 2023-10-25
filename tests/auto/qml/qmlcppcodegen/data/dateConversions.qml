pragma Strict
import QtQml
import TestTypes

QtObject {
    property date date: Druggeljug.myDate
    property date time: Druggeljug.myTime

    property string dateString: date
    property string timeString: time

    property real dateNumber: date
    property real timeNumber: time

    function shuffle() {
        Druggeljug.myDate = date;
        Druggeljug.myTime = time;

        dateString = Druggeljug.myDate;
        timeString = Druggeljug.myTime;
        dateNumber = Druggeljug.myDate;
        timeNumber = Druggeljug.myTime;
    }

    function fool() {
        var tmp = Druggeljug.myTime;
        Druggeljug.myTime = Druggeljug.myDate;
        Druggeljug.myDate = tmp;
    }

    function invalidate() {
        date = new Date("foo", "bar");
        time = new Date("bar", "foo");
    }
}
