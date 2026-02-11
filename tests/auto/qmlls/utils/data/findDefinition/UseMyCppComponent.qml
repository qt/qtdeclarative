import ModuleWithComponent

MyComponentFromCpp {
    myProperty: 123
    function readMyProperty() { return myProperty; }
    Item { x: 42; }
    function callSignals() { someSignal(); mySlot(); myInvokable(); }
    gadget.myProperty: 34
    gadget { myProperty2: 35 }
    property int someEnum: MyComponentFromCpp.HelloEnum
    property int someScopedEnum: MyComponentFromCpp.MyEnum.HelloEnum
    property int someFlag: MyComponentFromCpp.HelloFlag
    enum EnumFromQml { HelloEnumFromQml, ByeEnumFromQml }
    property int someEnum2: UseMyCppComponent.ByeEnumFromQml
    property int someScopedEnum2: UseMyCppComponent.EnumFromQml.ByeEnumFromQml
    property var notFromCpp: MyComponentFromNonCpp {}
}
