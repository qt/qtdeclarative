import QtQuick 2.0
import Test 1.0

Item {
    MyEnum1Class {
        id: enum1Class
        objectName: "enum1Class"
    }

    MyEnumDerivedClass {
        id: enumDerivedClass
        objectName: "enumDerivedClass"

        onValueAChanged: {
            aValue = newValue;
        }

        onValueBChanged: {
            bValue = newValue;
        }

        property int aValue: 0
        property int bValue: 0
    }

    function setEnumValues() {
        enum1Class.setValue(MyEnum1Class.A_13);
        enumDerivedClass.setValueA(MyEnum1Class.A_11);
        enumDerivedClass.setValueB(MyEnum2Class.B_37);
    }
}
