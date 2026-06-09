import QtQuick

Item {
    property Main main
    property int cValue: 300

    function getCValue(): int {
        return cValue
    }

    function backToMain(): Main {
        return main
    }

    function accessMainMethod(): int {
        return main.getMainData() + cValue
    }
}
