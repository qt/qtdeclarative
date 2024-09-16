pragma Strict
import QtQml

QtObject {
    property list<QtObject> elms: [this, null, null]

    property int found: find(this)
    property int foundNot: findNot(this)
    property int foundStrict: findStrict(this)
    property int foundStrictNot: findStrictNot(this)

    function find(elm : QtObject) : int {
        let found = 0;
        for (var i = 0; i < elms.length; i++) {
            var value = elms[i];
            if (value == elm)
                ++found;
        }
        return found;
    }

    function findNot(elm : QtObject) : int {
        let found = 0;
        for (var i = 0; i < elms.length; i++) {
            var value = elms[i];
            if (value != elm)
                ++found;
        }
        return found;
    }

    function findStrict(elm : QtObject) : int {
        let found = 0;
        for (var i = 0; i < elms.length; i++) {
            var value = elms[i];
            if (value === elm)
                ++found;
        }
        return found;
    }

    function findStrictNot(elm : QtObject) : int {
        let found = 0;
        for (var i = 0; i < elms.length; i++) {
            var value = elms[i];
            if (value !== elm)
                ++found;
        }
        return found;
    }

    property bool optionalNull: {
        let a // Produces a QJsPrimitiveValue we can compare to null below
        if (objectName.length === 0)
            a = null
        else
            a = undefined

        return a === null
    }

    property int undefinedEqualsUndefined: {
        var matches = 0;

        // Overrun the array so that we get some undefined !== undefined.
        for (var i = 0; i < 4; i++) {
            var val1 = elms[i]
            for (var j = 0; j < 4; j++) {
                var val2 = elms[j]
                if (!(val1 !== val2))
                    ++matches
            }
        }

        return matches;
    }
}
