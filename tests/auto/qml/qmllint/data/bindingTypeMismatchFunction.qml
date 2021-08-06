import QtQuick

Item {
    function returnsString() : string {}
    property int number: returnsString()
}
