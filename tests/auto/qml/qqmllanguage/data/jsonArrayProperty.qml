import QtQml
import TypeWithQJsonArrayProperty

TypeWithQJsonArrayProperty {
    function jsArray() { return [1, 2, 3] }

    jsonArray: jsArray()

    property list<int> concatenatedJsonArray: jsonArray.concat([4, 5, 6])
    property list<int> concatenatedJsArray: jsArray().concat([4, 5, 6])

    property bool entriesMatch: {
        var iterator = jsonArray.entries();
        for (var [index, element] of jsArray().entries()) {
            var v = iterator.next().value;
            if (index !== v[0] || element !== v[1]) {
                console.log(index, v[0], element, v[1]);
                return false;
            }
        }

        var iterator = jsArray().entries();
        for (var [index, element] of jsonArray.entries()) {
            var v = iterator.next().value;
            if (index !== v[0] || element !== v[1]) {
                console.log(index, v[0], element, v[1]);
                return false;
            }
        }

        return true;
    }

    property bool jsonArrayEvery: jsonArray.every(element => element != 0)
    property bool jsArrayEvery: jsArray().every(element => element != 0)

    property list<int> jsonArrayFiltered: jsonArray.filter(element => element > 2)
    property list<int> jsArrayFiltered: jsArray().filter(element => element > 2)

    property int jsonArrayFind: jsonArray.find(element => element === 2)
    property int jsArrayFind: jsArray().find(element => element === 2)

    property int jsonArrayFindIndex: jsonArray.findIndex(element => element === 1)
    property int jsArrayFindIndex: jsArray().findIndex(element => element === 1)

    property string jsonArrayForEach
    property string jsArrayForEach

    property bool jsonArrayIncludes: jsonArray.includes(3)
    property bool jsArrayIncludes: jsArray().includes(3)

    property int jsonArrayIndexOf: jsonArray.indexOf(2)
    property int jsArrayIndexOf: jsArray().indexOf(2)

    property string jsonArrayJoin: jsonArray.join()
    property string jsArrayJoin: jsArray().join()

    property bool keysMatch: {
        var iterator = jsonArray.keys();
        for (var index of jsArray().keys()) {
            var v = iterator.next().value;
            if (index !== v) {
                console.log(index, v);
                return false;
            }
        }

        var iterator = jsArray().keys();
        for (var index of jsonArray.keys()) {
            var v = iterator.next().value;
            if (index !== v) {
                console.log(index, v);
                return false;
            }
        }

        return true;
    }

    property int jsonArrayLastIndexOf: jsonArray.lastIndexOf(1)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(1)

    property list<string> jsonArrayMap: jsonArray.map(element => element.toString())
    property list<string> jsArrayMap: jsArray().map(element => element.toString())

    property int jsonArrayReduce: jsonArray.reduce((acc, element) => acc - element, 40)
    property int jsArrayReduce: jsArray().reduce((acc, element) => acc - element, 40)

    property string jsonArrayReduceRight: jsonArray.reduceRight((acc, element) => acc + element.toString(), "")
    property string jsArrayReduceRight: jsArray().reduceRight((acc, element) => acc + element.toString(), "")

    property list<int> jsonArraySlice: jsonArray.slice(0, 1)
    property list<int> jsArraySlice: jsArray().slice(0, 1)

    property bool jsonArraySome: jsonArray.some(element => element === 1)
    property bool jsArraySome: jsArray().some(element => element === 1)

    property string stringifiedLocaleJsonArray: jsonArray.toLocaleString()
    property string stringifiedLocaleJsArray: jsArray().toLocaleString()

    property string stringifiedJsonArray: jsonArray.toString()
    property string stringifiedJsArray: jsArray().toString()

    property bool valuesMatch: {
        var iterator = jsonArray.values();
        for (var obj of jsArray().values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        var iterator = jsArray().values();
        for (var obj of jsonArray.values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        return true;
    }

    // In-place mutation methods.
    // Set by onCompleted if mutating jsonArray and then accessing it
    // respects the mutation and the mutation behaves as for an array.
    property bool jsonArrayWasCopiedWithin: false
    property bool jsonArrayWasFilled: false
    property bool jsonArrayWasPopped: false
    property bool jsonArrayWasPushed: false
    property bool jsonArrayWasReversed: false
    property bool jsonArrayWasShifted: false
    property bool jsonArrayWasSpliced: false
    property bool jsonArrayWasUnshifted: false
    property bool jsonArrayWasSorted: false

    Component.onCompleted: {
        function equals(lhs, rhs) {
            return lhs.toString() === rhs.toString()
        }

        jsonArray.forEach(element =>  jsonArrayForEach += "-" + element + "-");
        jsArray().forEach(element =>  jsArrayForEach += "-" + element + "-");

        var array = jsArray()

        jsonArray.copyWithin(1, 0, 1)
        array.copyWithin(1, 0, 1)
        jsonArrayWasCopiedWithin = equals(jsonArray, array)

        jsonArray.fill(7, 0, 1)
        array.fill(7, 0, 1)
        jsonArrayWasFilled = equals(jsonArray, array)

        jsonArray.pop()
        array.pop()
        jsonArrayWasPopped = equals(jsonArray, array)

        jsonArray.push(23)
        jsonArray.push(11)
        jsonArray.push(54)
        jsonArray.push(42)
        array.push(23)
        array.push(11)
        array.push(54)
        array.push(42)
        jsonArrayWasPushed = equals(jsonArray, array)

        jsonArray.reverse()
        array.reverse()
        jsonArrayWasReversed = equals(jsonArray, array)

        jsonArray.shift()
        array.shift()
        jsonArrayWasShifted = equals(jsonArray, array)

        jsonArray.splice(2, 1, [1, 2], 7, [1, 5])
        array.splice(2, 1, [1, 2], 7, [1, 5])
        jsonArrayWasSpliced = equals(jsonArray, array)

        jsonArray.unshift(4, 71)
        array.unshift(4, 71)
        jsonArrayWasUnshifted = equals(jsonArray, array)

        jsonArray.sort()
        array.sort()
        jsonArrayWasSorted = equals(jsonArray, array)
    }
}
