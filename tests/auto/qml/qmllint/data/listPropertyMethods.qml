import QtQml

QtObject {
    id: self

    required property real i
    required property real j
    required property real k

    property QtObject r1: QtObject {}
    property QtObject r2: QtObject {}
    property QtObject r3: QtObject {}

    property list<QtObject> listProperty: [r1, r2, r3]
    property list<QtObject> listPropertySlice: listProperty.slice(i, j)
    property int listPropertyIndexOf: listProperty.indexOf(r2, i)
    property int listPropertyLastIndexOf: listProperty.lastIndexOf(r3, i)
    property string listPropertyToString: listProperty.toString()
    property string listPropertyToLocaleString: listProperty.toLocaleString()
    property list<QtObject> listPropertyConcat: listProperty.concat(listProperty)
    property QtObject listPropertyFind: listProperty.find(element => element.objectName === "")
    property int listPropertyFindIndex: listProperty.findIndex(element => element === r2)
    property bool listPropertyIncludes: listProperty.includes(r3)
    property string listPropertyJoin: listProperty.join()

    property bool entriesMatch: {
        var iterator = listProperty.entries();
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
        for (var obj of listProperty.values()) {
            var v = iterator.next().value;
            if (obj !== v) {
                console.log(obj, v);
                return false;
            }
        }

        return true;
    }

    property bool listPropertyEvery: listProperty.every((element) => element != null)
    property bool listPropertySome: listProperty.some((element) => element.objectName === "")

    property list<int> listPropertyMap: listProperty.map(((element) => element.objectName.length))
    property list<QtObject> listPropertyFilter: listProperty.filter((element) => element.objectName.length != 1)
    property string listPropertyReduceRight: listProperty.reduceRight((element, v) => v + '-' + element.objectName + 'v', "")
    property string listPropertyReduce: listProperty.reduce((element, v) => v + '-' + element.objectName + 'v', "")
    property list<string> listPropertyOwnPropertyNames: Object.getOwnPropertyNames(listProperty)

    Component.onCompleted: {
        listProperty.reverse();
        listProperty.pop();
        listProperty.push(self);
        listProperty.shift();
        listProperty.unshift(self);
        listProperty.forEach((element) => { console.log("-" + element.objectName + "-") });
        listProperty.sort();
        listProperty.sort((a, b) => (a.objectName.length - b.objectName.length))
        listProperty.copyWithin(i, j, k);
        listProperty.fill(self, i, Math.min(j, 1024));
        listProperty.splice(i, j, self, self, self);
    }
}
