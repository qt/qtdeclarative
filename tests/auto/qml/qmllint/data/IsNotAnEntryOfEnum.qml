import QtQuick

Item {
    id: item
    visible: true

    enum Mode {
        Hours,
        Minutes
    }

    property int mode: item.Mode.Hours
    property string s: item.mode === IsNotAnEntryOfEnum.Mode.Hour ? "green" : "tomato"
}
