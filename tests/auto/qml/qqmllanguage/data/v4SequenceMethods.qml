import QtQml

QtObject {
    property rect self: Qt.rect(9, 9, 9, 9)

    property rect r1: Qt.rect(1, 2, 3, 4)
    property rect r2: Qt.rect(5, 6, 7, 8)
    property rect r3: Qt.rect(9, 10, 11, 12)

    function jsArray() { return [r1, r2, r3] }
    property list<rect> v4Sequence: [r1, r2, r3]

    property string jsArrayToString: jsArray().toString()
    property string v4SequenceToString: v4Sequence.toString()

    property string jsArrayToLocaleString: jsArray().toLocaleString()
    property string v4SequenceToLocaleString: v4Sequence.toLocaleString()

    property list<rect> v4SequenceConcat: v4Sequence.concat(v4Sequence)
    property list<rect> jsArrayConcat: jsArray().concat(jsArray())

    property rect v4SequenceFind: v4Sequence.find(element => element.x === 1)
    property rect jsArrayFind: jsArray().find(element => element.x === 1)

    property int v4SequenceFindIndex: v4Sequence.findIndex(element => element === r2)
    property int jsArrayFindIndex: jsArray().findIndex(element => element === r2)

    property bool v4SequenceIncludes: v4Sequence.includes(r3)
    property bool jsArrayIncludes: jsArray().includes(r3)

    property string v4SequenceJoin: v4Sequence.join()
    property string jsArrayJoin: jsArray().join()

    property bool entriesMatch: {
        var iterator = v4Sequence.entries();
        for (var [index, element] of jsArray().entries()) {
            var v = iterator.next().value;
            if (index !== v[0] || element !== v[1]) {
                console.log(index, v[0], element, v[1]);
                return false;
            }
        }

        var iterator = jsArray().entries();
        for (var [index, element] of v4Sequence.entries()) {
            var v = iterator.next().value;
            if (index !== v[0] || element !== v[1]) {
                console.log(index, v[0], element, v[1]);
                return false;
            }
        }

        return true;
    }

    property bool keysMatch: {
        var iterator = v4Sequence.keys();
        for (var index of jsArray().keys()) {
            var v = iterator.next().value;
            if (index !== v) {
                console.log(index, v);
                return false;
            }
        }

        var iterator = jsArray().keys();
        for (var index of v4Sequence.keys()) {
            var v = iterator.next().value;
            if (index !== v) {
                console.log(index, v);
                return false;
            }
        }

        return true;
    }

    property bool valuesMatch: {
        var iterator = v4Sequence.values();
        for (var obj of jsArray().values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        var iterator = jsArray().values();
        for (var obj of v4Sequence.values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        return true;
    }

    property list<rect> v4SequencePop: v4Sequence
    property rect v4SequencePopped

    property list<rect> jsArrayPop
    property rect jsArrayPopped

    property list<rect> v4SequencePush: v4Sequence
    property int v4SequencePushed

    property list<rect> jsArrayPush
    property int jsArrayPushed

    property list<rect> v4SequenceReverse: v4Sequence
    property list<rect> jsArrayReverse: jsArray().reverse()

    property list<rect> v4SequenceShift: v4Sequence
    property rect v4SequenceShifted

    property list<rect> jsArrayShift
    property rect jsArrayShifted

    property list<rect> v4SequenceSplice: v4Sequence
    property list<rect> v4SequenceSpliced

    property list<rect> jsArraySplice
    property list<rect> jsArraySpliced

    property list<rect> v4SequenceUnshift: v4Sequence
    property int v4SequenceUnshifted

    property list<rect> jsArrayUnshift
    property int jsArrayUnshifted

    property int v4SequenceIndexOf: v4Sequence.indexOf(r2)
    property int jsArrayIndexOf: jsArray().indexOf(r2)

    property int v4SequenceLastIndexOf: v4Sequence.lastIndexOf(r3)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(r3)

    property bool v4SequenceEvery: v4Sequence.every((element) => element != null)
    property bool jsArrayEvery: jsArray().every((element) => element != null)

    property bool v4SequenceSome: v4Sequence.some((element) => element.x === 1)
    property bool jsArraySome: jsArray().some((element) => element.x === 1)

    property string v4SequenceForEach
    property string jsArrayForEach

    property list<int> v4SequenceMap: v4Sequence.map(((element) => element.x))
    property list<int> jsArrayMap: jsArray().map(((element) => element.x))

    property list<rect> v4SequenceFilter: v4Sequence.filter((element) => element.x != 1)
    property list<rect> jsArrayFilter: jsArray().filter((element) => element.x != 1)

    property string v4SequenceReduce: v4Sequence.reduce((element, v) => v + '-' + element.x + 'v', "")
    property string jsArrayReduce: jsArray().reduce((element, v) => v + '-' + element.x + 'v', "")

    property string v4SequenceReduceRight: v4Sequence.reduceRight((element, v) => v + '-' + element.x + 'v', "")
    property string jsArrayReduceRight: jsArray().reduceRight((element, v) => v + '-' + element.x + 'v', "")

    property list<string> jsArrayOwnPropertyNames: Object.getOwnPropertyNames(jsArray())
    property list<string> v4SequenceOwnPropertyNames: Object.getOwnPropertyNames(v4Sequence)

    property list<rect> v4SequenceSort1: v4Sequence
    property list<rect> jsArraySort1: jsArray().sort()

    property list<rect> v4SequenceSort2: v4Sequence
    property list<rect> jsArraySort2: jsArray().sort((a, b) => (a.x - b.x))

    Component.onCompleted: {
        v4SequenceReverse.reverse();

        v4SequencePopped = v4SequencePop.pop();
        var a = jsArray();
        jsArrayPopped = a.pop();
        jsArrayPop = a;

        v4SequencePushed = v4SequencePush.push(self);
        a = jsArray();
        jsArrayPushed = a.push(self);
        jsArrayPush = a;

        v4SequenceShifted = v4SequenceShift.shift();
        a = jsArray();
        jsArrayShifted = a.shift();
        jsArrayShift = a;

        v4SequenceUnshifted = v4SequenceUnshift.unshift(self);
        a = jsArray();
        jsArrayUnshifted = a.unshift(self);
        jsArrayUnshift = a;

        v4Sequence.forEach((element) => { v4SequenceForEach += "-" + element.x + "-" });
        jsArray().forEach((element) => { jsArrayForEach += "-" + element.x + "-" });

        v4SequenceSort1.sort();
        v4SequenceSort2.sort((a, b) => (a.x - b.x))
    }
}
