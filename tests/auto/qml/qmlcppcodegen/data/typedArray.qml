pragma Strict
import QtQml

QtObject {
    id: self
    property date aDate
    property list<bool> values1: []
    property list<int> values2: []
    property list<int> values3: [1, 2, 3, 4]
    property list<date> values4: [aDate, aDate, aDate]
    property list<real> values5: [1, 2, 3.4, "30", undefined, null]
    property list<QtObject> values6: [self, self, self]
    property string values7: "abcdef"

    property int inIntList: values3[1]
    property date inDateList: values4[2]
    property real inRealList: values5[3]
    property string inCharList: values7[5]

    function stringAt10(s: string): int {
        if (!s[10])
            return 10;
        else
            return 20;
    }
}
