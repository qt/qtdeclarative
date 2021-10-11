pragma ListPropertyAssignBehavior: ReplaceIfNotDefault

import QtQml

QtObject {
    component WithDefault: QtObject {
        default property list<QtObject> defaultList
    }

    component MyChild: WithDefault {
        QtObject { objectName: "default1" }

        property list<QtObject> myList: [
            QtObject { objectName: "test1" }
        ]
    }

    property MyChild myChild1: MyChild {
        myList: [
            QtObject { objectName: "test2" }
        ]

        QtObject { objectName: "default2" }
    }

    property MyChild myChild2: MyChild {
        property list<QtObject> myOwnList: [
            QtObject { property string str: "test3" }
        ]
        myList: myOwnList
        defaultList: myOwnList
    }

    property int length1: myChild1.myList.length
    property int length2: myChild2.myList.length

    property int default1: myChild1.defaultList.length
    property int default2: myChild2.defaultList.length
}
