import QtQml

QtObject {
    property var foo: new Error("bar")
    property int aaa: 12

    function ouch() {
        aaa = 13;
        throw new Error("ouch")
        aaa = 25;
    }
}
