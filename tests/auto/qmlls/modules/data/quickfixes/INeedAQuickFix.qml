import QtQuick

Item {
    id: hello

    signal s(xxx: string)
    property int i: 42

    // fix me! add '(xxx) =>' in front of console.log()
    onS: console.log(xxx)

    Item {
        // fix me! prepend 'hello.' to 'i'!
        function f() {
            return i
        }
    }

}
