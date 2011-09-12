import QtQuick 2.0
import MyTestModuleApi 1.0 as MyTestModuleApi

Item {
    id: rootObject
    objectName: "rootObject"
    property int newIntPropValue: 12

    property int moduleIntPropChangedCount: 0
    property int moduleOtherSignalCount: 0

    function setModuleIntProp() {
        MyTestModuleApi.intProp = newIntPropValue;
        newIntPropValue = newIntPropValue + 1;
    }

    Connections {
        target: MyTestModuleApi
        onIntPropChanged: moduleIntPropChangedCount = moduleIntPropChangedCount + 1;
        onOtherSignal: moduleOtherSignalCount = moduleOtherSignalCount + 1;
    }
}
