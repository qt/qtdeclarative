import test
import QtQml

BindingLoop {
    id: loop
    value: value2 + 1
    value2: value + 1

    Component.onCompleted: {
        let x = loop.value2 // if we do not read the value, we don't detect the loop...
    }
}
