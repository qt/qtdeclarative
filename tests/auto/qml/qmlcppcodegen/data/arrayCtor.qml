pragma Strict
import QML

QtObject {
    property list<int> defaultCtor: new Array()
    property list<int> oneArgCtor: new Array(5)
    property list<int> multiArgCtor: new Array(2, 3, 3, 4)
    property list<bool> arrayTrue: new Array(true)
    property list<bool> arrayFalse: new Array(false)
    property list<real> arrayNegative: new Array(-14)
}
