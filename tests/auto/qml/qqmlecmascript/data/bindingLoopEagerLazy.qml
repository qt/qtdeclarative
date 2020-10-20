import test
import QtQml

QtObject {
    property BindingLoop a: BindingLoop {
        value: b.eager1+1
    }

    property BindingLoop b: BindingLoop {
        eager1: a.value+1
    }
}

