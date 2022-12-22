import ValueTypes
import StaticTest
import QtQml

QtObject {
    id: self

    property base v1
    property derived v2

    property var valueTypeMethod: v1.report

    property var qtMethod: Qt.rect

    property rect badRect: self.qtMethod(1, 2, 3, 4)
    property rect goodRect1: qtMethod.call(undefined, 1, 2, 3, 4)
    property rect goodRect2: qtMethod.call(Qt, 1, 2, 3, 4)

    property string badString: self.valueTypeMethod()
    property string goodString1: valueTypeMethod.call(undefined)
    property string goodString2: valueTypeMethod.call(v1)
    property string goodString3: valueTypeMethod.call(v2)

    property string goodString4: toString.call(Qt)
    property string badString2: toString.call(goodRect1)

    property var mm: OriginalSingleton.mm
    property int badInt: self.mm()
    property int goodInt1: mm.call(undefined)
    property int goodInt2: mm.call(OriginalSingleton)
    property int goodInt3: mm.call(DerivedSingleton)
}
