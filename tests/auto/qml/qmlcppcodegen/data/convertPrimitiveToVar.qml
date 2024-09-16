pragma Strict
import QtQml

QtObject {
    id: foo

    property int offsetValue

    function send(data : variant) {
    }

    Component.onCompleted: () => {
        let deltaOffset = 42
        deltaOffset -= 1
        foo.offsetValue = deltaOffset
        foo.send({offset: deltaOffset})
    }
}
