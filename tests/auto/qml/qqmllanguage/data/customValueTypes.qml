import ValueTypes
import QtQml

QtObject {
    property base base
    property derived derived

    onObjectNameChanged: {
        derived.content = derived.nothing();
        derived.increment();
        base = derived;
        derived.increment();
    }
}
