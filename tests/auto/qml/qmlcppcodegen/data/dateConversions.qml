pragma Strict
import QtQml
import TestTypes

QtObject {
    property date date: Druggeljug.myDate
    property date time: Druggeljug.myTime

    property string dateString: date
    property string timeString: time

    function shuffle() {
        Druggeljug.myDate = date;
        Druggeljug.myTime = time;

        dateString = Druggeljug.myDate;
        timeString = Druggeljug.myTime;
    }

    function fool() {
        var tmp = Druggeljug.myTime;
        Druggeljug.myTime = Druggeljug.myDate;
        Druggeljug.myDate = tmp;
    }
}
