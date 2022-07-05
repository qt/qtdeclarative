import QML

QtObject {
    id: self

    property QtObject l1: QtObject { objectName: "klaus" }
    property QtObject l2: QtObject { function toString(): string { return "teil" } }
    property QtObject l3: QtObject { }

    function jsArray() { return [l1, l2, l3] }
    property list<QtObject> listProperty: [l1, l2, l3]

    property string jsArrayToString: jsArray().toString()
    property string listPropertyToString: listProperty.toString()

    property string jsArrayToLocaleString: jsArray().toLocaleString()
    property string listPropertyToLocaleString: listProperty.toLocaleString()

    property list<QtObject> listPropertyConcat: listProperty.concat(listProperty)
    property list<QtObject> jsArrayConcat: jsArray().concat(jsArray())

    property QtObject listPropertyFind: listProperty.find(element => element.objectName === "klaus")
    property QtObject jsArrayFind: jsArray().find(element => element.objectName === "klaus")

    property int listPropertyFindIndex: listProperty.findIndex(element => element === l2)
    property int jsArrayFindIndex: jsArray().findIndex(element => element === l2)

    property bool listPropertyIncludes: listProperty.includes(l3)
    property bool jsArrayIncludes: jsArray().includes(l3)

    property string listPropertyJoin: listProperty.join()
    property string jsArrayJoin: jsArray().join()

    property bool entriesMatch: {
        var iterator = listProperty.entries();
        for (var [index, element] of jsArray().entries()) {
            var v = iterator.next().value;
            if (index !== v[0] || element !== v[1]) {
                console.log(index, v[0], element, v[1]);
                return false;
            }
        }

        var iterator = jsArray().entries();
        for (var [index, element] of listProperty.entries()) {
            var v = iterator.next().value;
            if (index !== v[0] || element !== v[1]) {
                console.log(index, v[0], element, v[1]);
                return false;
            }
        }

        return true;
    }

    property bool keysMatch: {
        var iterator = listProperty.keys();
        for (var index of jsArray().keys()) {
            var v = iterator.next().value;
            if (index !== v) {
                console.log(index, v);
                return false;
            }
        }

        var iterator = jsArray().keys();
        for (var index of listProperty.keys()) {
            var v = iterator.next().value;
            if (index !== v) {
                console.log(index, v);
                return false;
            }
        }

        return true;
    }

    property bool valuesMatch: {
        var iterator = listProperty.values();
        for (var obj of jsArray().values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        var iterator = jsArray().values();
        for (var obj of listProperty.values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        return true;
    }

    property list<QtObject> listPropertyPop: listProperty
    property QtObject listPropertyPopped

    property list<QtObject> jsArrayPop
    property QtObject jsArrayPopped

    property list<QtObject> listPropertyPush: listProperty
    property int listPropertyPushed

    property list<QtObject> jsArrayPush
    property int jsArrayPushed

    property list<QtObject> listPropertyReverse: listProperty
    property list<QtObject> jsArrayReverse: jsArray().reverse()

    property list<QtObject> listPropertyShift: listProperty
    property QtObject listPropertyShifted

    property list<QtObject> jsArrayShift
    property QtObject jsArrayShifted

    property list<QtObject> listPropertySplice: listProperty
    property list<QtObject> listPropertySpliced

    property list<QtObject> jsArraySplice
    property list<QtObject> jsArraySpliced

    property list<QtObject> listPropertyUnshift: listProperty
    property int listPropertyUnshifted

    property list<QtObject> jsArrayUnshift
    property int jsArrayUnshifted

    property int listPropertyIndexOf: listProperty.indexOf(l2)
    property int jsArrayIndexOf: jsArray().indexOf(l2)

    property int listPropertyLastIndexOf: listProperty.lastIndexOf(l3)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(l3)

    property bool listPropertyEvery: listProperty.every((element) => element != null)
    property bool jsArrayEvery: jsArray().every((element) => element != null)

    property bool listPropertySome: listProperty.some((element) => element.objectName === "klaus")
    property bool jsArraySome: jsArray().some((element) => element.objectName === "klaus")

    property string listPropertyForEach
    property string jsArrayForEach

    property list<string> listPropertyMap: listProperty.map(((element) => element.objectName))
    property list<string> jsArrayMap: jsArray().map(((element) => element.objectName))

    property list<QtObject> listPropertyFilter: listProperty.filter((element) => element.objectName != "klaus")
    property list<QtObject> jsArrayFilter: jsArray().filter((element) => element.objectName != "klaus")

    property string listPropertyReduce: listProperty.reduce((element, v) => v + '-' + element.objectName + 'v', "")
    property string jsArrayReduce: jsArray().reduce((element, v) => v + '-' + element.objectName + 'v', "")

    property string listPropertyReduceRight: listProperty.reduceRight((element, v) => v + '-' + element.objectName + 'v', "")
    property string jsArrayReduceRight: jsArray().reduceRight((element, v) => v + '-' + element.objectName + 'v', "")

    property list<string> jsArrayOwnPropertyNames: Object.getOwnPropertyNames(jsArray())
    property list<string> listPropertyOwnPropertyNames: Object.getOwnPropertyNames(listProperty)

    property list<QtObject> listPropertySort1: listProperty
    property list<QtObject> jsArraySort1: jsArray().sort()

    property list<QtObject> listPropertySort2: listProperty
    property list<QtObject> jsArraySort2: jsArray().sort((a, b) => (a.objectName.length - b.objectName.length))

    Component.onCompleted: {
        listPropertyReverse.reverse();

        listPropertyPopped = listPropertyPop.pop();
        var a = jsArray();
        jsArrayPopped = a.pop();
        jsArrayPop = a;

        listPropertyPushed = listPropertyPush.push(self);
        a = jsArray();
        jsArrayPushed = a.push(self);
        jsArrayPush = a;

        listPropertyShifted = listPropertyShift.shift();
        a = jsArray();
        jsArrayShifted = a.shift();
        jsArrayShift = a;

        listPropertyUnshifted = listPropertyUnshift.unshift(self);
        a = jsArray();
        jsArrayUnshifted = a.unshift(self);
        jsArrayUnshift = a;

        listProperty.forEach((element) => { listPropertyForEach += "-" + element.objectName + "-" });
        jsArray().forEach((element) => { jsArrayForEach += "-" + element.objectName + "-" });

        listPropertySort1.sort();
        listPropertySort2.sort((a, b) => (a.objectName.length - b.objectName.length))

        console.log(jsArraySort1, listPropertySort1);
        console.log(jsArraySort2, listPropertySort2);
    }
}
