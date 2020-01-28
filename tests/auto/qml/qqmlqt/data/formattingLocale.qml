import QtQml 2.15

QtObject {
    required property var myDateTime
    required property var myDate
    property var myTime

    property string dateTimeString: Qt.formatDateTime(myDateTime, Qt.locale("de_DE"), Locale.NarrowFormat)
    property string dateString: Qt.formatDate(myDate, Qt.locale("de_DE"))

    function invalidUsage() { Qt.formatTime(myTime, null, "hello") }
}
