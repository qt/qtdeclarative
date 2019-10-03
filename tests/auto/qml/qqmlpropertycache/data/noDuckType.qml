import QtQml 2.9

QtObject {
    property SpecialObject1 obj1: SpecialObject1 {}
    property SpecialObject2 obj2: SpecialObject2 {}
    property string result: (obj1 instanceof SpecialObject2) ? "bad" : "good"
}
