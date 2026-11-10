import QtQml

// A function stays callable after the object it is defined in has been destroyed. It then runs
// without its scope object.
QtObject {
    property int foo: 1
    property var stored

    readonly property QtObject child: QtObject {
        property int foo: 2
        function probe(): string { return "" + foo }
    }

    function store() { stored = child.probe }
    function callStored(): string {
        try {
            return stored()
        } catch (e) {
            return "<" + e.name + ">"
        }
    }
}
