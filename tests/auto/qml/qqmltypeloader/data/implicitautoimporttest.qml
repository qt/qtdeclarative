import modulewithimplicitautoimport 2.11 as MyNS
MyNS.Test {
    MyNS.Item { // Implicitly imported from QtQuick
        // containmentMask added in 2.11. Version is forwarded.
        objectName: containmentMask.objectName
    }
    MyNS.ListModel {}
}
