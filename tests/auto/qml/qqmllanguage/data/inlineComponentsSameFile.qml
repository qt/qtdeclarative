import QtQml 2.15

QtObject {
    component IC : QtObject {
        property string name
        property int age
    }

    property IC other: IC { name: "Toby"; age: 30 }
    property list<IC> listProp: [IC { name: "Alfred Ill"; age: 65 }, IC { name: "Claire Zachanassian"; age: 62}]
}
