pragma Strict
import QtQml

QtObject {
    id: last
    property int value: 10

    function verify(i: int) {
        if (last.value !== i)
            console.error("failed", last.value, i);
        else
            console.log("success")
    }

    Component.onCompleted: {
        verify(10)
        verify(11)
    }
}
