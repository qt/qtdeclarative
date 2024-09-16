import QtQml

QtObject {
    component Variable: QtObject {
        property int value: 4
    }

    component VariableShadow: Variable {
        property string value: "1"
    }

    property Variable last: VariableShadow {}

    function find(n: int) : int {
        var found = 0
        for (var i = 0; i < n; i++) {
            if (last.value == i)
                ++found
        }
        return found;
    }

    function findStrict(n: int) : int {
        var found = 0
        for (var i = 0; i < n; i++) {
            if (last.value === i)
                ++found
        }
        return found;
    }

    function findNot(n: int) : int {
        var found = 0
        for (var i = 0; i < n; i++) {
            if (last.value != i)
                ++found
        }
        return found;
    }

    function findNotStrict(n: int) : int {
        var found = 0
        for (var i = 0; i < n; i++) {
            if (last.value !== i)
                ++found
        }
        return found;
    }

    property int found: find(3)
    property int foundStrict: findStrict(10)
    property int foundNot: findNot(3)
    property int foundNotStrict: findNotStrict(10)
}
