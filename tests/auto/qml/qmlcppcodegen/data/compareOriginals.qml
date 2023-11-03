pragma Strict
import QtQml

QtObject {
    component Variable: QtObject {
        property int value: 4
    }

    property Variable first: Variable {}
    property Variable last: Variable {
        id: last
    }

    property int compareOriginals: {
        var matches = 0;
        for (var i = 0; i < 6; i++) {
            first.value = i; // do a shadowed assignment
            if (last.value != i)
                ++matches
        }
        return matches;
    }

    property bool optionalThis: {
        var a
        if (2 == 2)
            a = this
        else
            a = undefined

        var b
        if (2 == 2)
            b = this
        else
            b = undefined

        return a === b
    }
}
