import QtQml

QtObject {
    property rect self: Qt.rect(9, 9, 9, 9)

    required property real i
    required property real j
    required property real k

    property rect r1: Qt.rect(1, 2, 3, 4)
    property rect r2: Qt.rect(5, 6, 7, 8)
    property rect r3: Qt.rect(9, 10, 11, 12)

    property list<rect> v4Sequence: [r1, r2, r3]
    property list<rect> v4SequenceSlice: v4Sequence.slice(i, j)
    property int v4SequenceIndexOf: v4Sequence.indexOf(r2, i)
    property int v4SequenceLastIndexOf: v4Sequence.lastIndexOf(r3, i)
    property string v4SequenceToString: v4Sequence.toString()
    property string v4SequenceToLocaleString: v4Sequence.toLocaleString()
    property list<rect> v4SequenceConcat: v4Sequence.concat(v4Sequence)
    property rect v4SequenceFind: v4Sequence.find(element => element.x === 1)
    property int v4SequenceFindIndex: v4Sequence.findIndex(element => element === r2)
    property bool v4SequenceIncludes: v4Sequence.includes(r3)
    property string v4SequenceJoin: v4Sequence.join()

    property bool entriesMatch: {
        var iterator = v4Sequence.entries();
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
        for (var obj of v4Sequence.values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        return true;
    }

    property bool v4SequenceEvery: v4Sequence.every((element) => element != null)
    property bool v4SequenceSome: v4Sequence.some((element) => element.x === 1)

    property list<int> v4SequenceMap: v4Sequence.map(((element) => element.x))
    property list<rect> v4SequenceFilter: v4Sequence.filter((element) => element.x != 1)
    property string v4SequenceReduceRight: v4Sequence.reduceRight((element, v) => v + '-' + element.x + 'v', "")
    property string v4SequenceReduce: v4Sequence.reduce((element, v) => v + '-' + element.x + 'v', "")
    property list<string> v4SequenceOwnPropertyNames: Object.getOwnPropertyNames(v4Sequence)

    Component.onCompleted: {
        v4Sequence.reverse();
        v4Sequence.pop();
        v4Sequence.push(self);
        v4Sequence.shift();
        v4Sequence.unshift(self);
        v4Sequence.forEach((element) => { console.log("-" + element.x + "-") });
        v4Sequence.sort();
        v4Sequence.sort((a, b) => (a.x - b.x))
        v4Sequence.copyWithin(i, j, k);
        v4Sequence.fill(self, i, Math.min(j, 1024));
        v4Sequence.splice(i, j, self, self, self);
    }
}
