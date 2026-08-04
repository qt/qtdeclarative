import QtQml
import QtQml.DesignSupport

QtObject {
    id: root

    property Component objectFactory: Component {
        QtObject {
            property int testProp: 22
        }
    }

    property Component registryFactory: Component {
        ObjectRegistry { key: "StaleTarget"; target: null }
    }

    property QtObject firstObject: null
    property QtObject firstRegistry: null
    property QtObject secondObject: null
    property QtObject secondRegistry: null

    function registerFirst() {
        firstObject = objectFactory.createObject(null);
        firstRegistry = registryFactory.createObject(root);
        firstRegistry.target = firstObject;
    }

    function destroyFirstObject() {
        firstObject.destroy();
        firstObject = null;
    }

    function registerSecond() {
        secondObject = objectFactory.createObject(null);
        secondRegistry = registryFactory.createObject(root);
        secondRegistry.target = secondObject;
    }

    function destroyFirstRegistry() {
        firstRegistry.destroy();
        firstRegistry = null;
    }
}
