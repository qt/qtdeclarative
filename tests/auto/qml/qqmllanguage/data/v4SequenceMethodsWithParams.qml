import QtQml

QtObject {
    property rect self: Qt.rect(9, 9, 9, 9)

    required property real i
    required property real j
    required property real k

    property rect r1: Qt.rect(1, 2, 3, 4)
    property rect r2: Qt.rect(5, 6, 7, 8)
    property rect r3: Qt.rect(9, 10, 11, 12)

    function jsArray() { return [r1, r2, r3] }
    property list<rect> v4Sequence: [r1, r2, r3]

    property list<rect> v4SequenceCopyWithin: v4Sequence
    property list<rect> jsArrayCopyWithin: jsArray().copyWithin(i, j, k)

    property list<rect> v4SequenceFill: v4Sequence
    property list<rect> jsArrayFill: jsArray().fill(self, i, Math.min(j, 1024))

    property list<rect> v4SequenceSlice: v4Sequence.slice(i, j)
    property list<rect> jsArraySlice: jsArray().slice(i, j)

    property list<rect> v4SequenceSplice: v4Sequence
    property list<rect> v4SequenceSpliced

    property list<rect> jsArraySplice
    property list<rect> jsArraySpliced

    property int v4SequenceIndexOf: v4Sequence.indexOf(r2, i)
    property int jsArrayIndexOf: jsArray().indexOf(r2, i)

    property int v4SequenceLastIndexOf: v4Sequence.lastIndexOf(r3, i)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(r3, i)

    Component.onCompleted: {
        v4SequenceCopyWithin.copyWithin(i, j, k);
        v4SequenceFill.fill(self, i, Math.min(j, 1024));

        v4SequenceSpliced = v4SequenceSplice.splice(i, j, self, self, self);
        var a = jsArray();
        jsArraySpliced = a.splice(i, j, self, self, self);
        jsArraySplice = a;
    }
}
