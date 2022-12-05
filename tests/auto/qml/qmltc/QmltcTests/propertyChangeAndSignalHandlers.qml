import QtQuick
import QmltcTests 1.0
Item {
    function changeProperties1() {
        one.a++;
        one.b = one.b + "1";
        one.c = one.c + 1;
    }
    function changeProperties2() { two.c = two.c + 1; }
    function changeProperties3(value: real) { three.c = value; }

    property int aChangedCount1: 0
    property int bChangedCount1: 0
    property int cChangedCount1: 0
    property int dChangedCount1: 0
    property int cChangedCount2: 0
    property int dChangedCount2: 0
    property int cChangedCount3: 0
    property int dChangedCount3: 0
    property string dChangedStr3: ""
    property int cChangedCount4: 0
    property int dChangedCount4: 0
    property string dChangedStr4: ""

    TypeWithProperties {
        id: one
        onAChanged: { aChangedCount1++; }
        onBChanged: { bChangedCount1++; }
        onCChanged: { cChangedCount1++; }
        onDChanged: { dChangedCount1++; }
    }

    TypeWithProperties {
        id: two
        onCWeirdSignal: { cChangedCount2++; }
        onDSignal: { dChangedCount2++; }
    }

    TypeWithProperties {
        id: three
        onCChanged: function(value) { cChangedCount3 = value; }
        onDChanged: function(str, value) {
            dChangedStr3 = str;
            dChangedCount3 = value;
        }
    }

    TypeWithProperties {
        id: four
        onCWeirdSignal: function(value) { cChangedCount4 = value + 1; }
        onDSignal: function(str, value) {
            dChangedStr4 = str + "!";
            dChangedCount4 = value / 2;
        }
    }

    TypeWithProperties {
        id: five

        property int mouseButtonA
        property int mouseButtonB
        // check if enums/flags can be compiled when used as signal parameters, instead of throwing asserts
        onSignalWithEnum: function (a, b) {
            mouseButtonA = a
            mouseButtonB = b
        }
    }
}
