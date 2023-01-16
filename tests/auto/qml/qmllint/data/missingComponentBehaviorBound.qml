import QtQuick
Item {
    id: barsty
    property int fooInt: 42

    Repeater {
        model: 5
        Text { text: "Foo=" + barsty.fooInt }
    }
}
