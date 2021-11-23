import QML

QtObject {
    id: self
    property var aNull:   null
    property var anInt:   5
    property var aZero:   0
    property var anArray: []
    property var anObject: ({a: 12, b: 14, c: "somestring"})

    property int cmpEqInt: {
        if ({} == 12)
            return 8;
        if ([] == 12)
            return 2;
        if (null == 12)
            return 3;
        if (undefined == 12)
            return 4;
        if (12 == 12)
            return 17;
        return 5;
    }

    property bool nullIsNull:   aNull    == null
    property bool intIsNull:    anInt    == null
    property bool zeroIsNull:   aZero    == null
    property bool arrayIsNull:  anArray  == null
    property bool objectIsNull: anObject == null

    property bool nullIsNotNull:   aNull    != null
    property bool intIsNotNull:    anInt    != null
    property bool zeroIsNotNull:   aZero    != null
    property bool arrayIsNotNull:  anArray  != null
    property bool objectIsNotNull: anObject != null

    property bool boolEqualsBool: nullIsNull == zeroIsNull
    property bool boolNotEqualsBool: nullIsNull != zeroIsNull

    property var aInObject: "a" in anObject
    property var lengthInArray: "length" in anArray
    property var fooInNumber: "foo" in 12
    property var numberInObject: 12 in anObject
    property var numberInArray: 2 in [1, 12, 3]

    property real varPlusVar: aInObject + lengthInArray
    property real varMinusVar: lengthInArray - fooInNumber
    property real varTimesVar: fooInNumber * numberInObject
    property real varDivVar: numberInObject / numberInArray

    property var stringPlusString: "12" + "20"
    property var stringMinusString: "12" - "20"
    property var stringTimesString: "12" * "20"
    property var stringDivString: "12" / "20"

    property string uglyString: "with\nnewline" + 'with"quot' + "with\\slashes";

    property QtObject nullObject1: aInObject ? null : self
    property QtObject nullObject2: {
        if (lengthInArray)
            return self
        if (aInObject)
            return null;
        return undefined;
    }

    property var doneStuff
    function doStuff(stuff) {
        var a = {g: 0, h: 5};
        var b = {i: 6, j: 7};
        var c = {k: 9, l: 9};

        a.g = 4;

        doneStuff = a.g + b.i + c.l
        return stuff[3] ? stuff[3] : stuff.z
    }

    property var passObjectLiteral: doStuff({"x": 13, "y": 17, "z": 11})
    property var passArrayLiteral: doStuff([1, 2, 3, 4])

    property bool neNull: {
        var a = anArray[12]
        if (a)
            return true;
        else
            return false;
    }

    property bool eqNull: {
        var a = anArray[12]
        if (!a)
            return true;
        else
            return false;
    }

    property string undefinedType: typeof undefined
    property string booleanType: typeof true
    property string numberType: typeof 12
    property string stringType: typeof "bah"
    property string objectType: typeof null
    property string symbolType: typeof Symbol("baz") 

    property var modulos: [
        -20 % -1,
        0 % 1,
        20 % 4.4,
        -4.4 % true,
        false % "11",
        undefined % null,
        {} % []
    ]

    property var unaryOps: {
        var a = stringPlusString;
        var b = stringPlusString;
        var c = +stringPlusString;
        var d = -stringPlusString;
        var e = a++;
        var f = b--;
        return [ a, b, c, d, e, f ];
    }

    function retUndefined() {
        var a = 5 + 12;
        if (false)
            return a;
    }
}
