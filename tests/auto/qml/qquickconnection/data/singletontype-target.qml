import QtQuick 2.0
import MyTestSingletonType 1.0 as MyTestSingletonType

Item {
    id: rootObject
    objectName: "rootObject"
    property int newIntPropValue: 12

    property int moduleIntPropChangedCount: 0
    property int moduleOtherSignalCount: 0

    function setModuleIntProp() {
        MyTestSingletonType.Api.intProp = newIntPropValue;
        newIntPropValue = newIntPropValue + 1;
    }

    Connections {
        target: MyTestSingletonType.Api
        onIntPropChanged: moduleIntPropChangedCount = moduleIntPropChangedCount + 1;
        onOtherSignal: moduleOtherSignalCount = moduleOtherSignalCount + 1;
    }
}
