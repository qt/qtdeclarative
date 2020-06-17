import modulewithimplicitversionedimport 2.0 as MyNS
MyNS.Test {
    MyNS.Item { // Implicitly imported from QtQuick
        // containmentMask added in 2.11. Despite version 2.0 above, the module imports 2.15.
        objectName: containmentMask.objectName
    }
    MyNS.ListModel {}
}
