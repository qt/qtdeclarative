import QtQuick

Item {
    property int cat: 12
    property string cat: "mutantx"

    signal clicked()
    signal clicked(a: int)
}