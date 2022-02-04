import QtQml

QtObject {
    property int someValue: 42
    function foo() {
        if (someValue != 0) {
            t.text = "Bar";
        }
    }

    property QtObject tt: QtObject {
        id: t
        property string text: "Foo"
    }
}
