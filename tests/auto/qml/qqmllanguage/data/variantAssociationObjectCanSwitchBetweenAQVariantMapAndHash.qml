import Test
import QtQml

VariantAssociationProvider {
    property string emailAfterMapToHashSwitch: ""
    property string emailAfterHashToMapSwitch: ""

    Component.onCompleted: {
        // Populate the list
        variantList = getListOfMap();

        // items has to remain attached. Otherwise this doesn't work.
        // Arguably, this is a bit insane, but then the technique shown here
        // intentionally abuses the reference-vs-object problem.
        const items = variantList;

        // This is a reference object to items[0].
        let originallyAMap = items[0];
        originallyAMap.email = "map@map.map";

        // This switches the underlying type to a QVariantHash.
        items[0] = variantHash;
        items[0].email = "hash@hash.hash";

        // This trigger a read-back with a switched type.
        emailAfterMapToHashSwitch = originallyAMap.email;

        // Now we switch back to a QVariantMap
        items[0] = variantMap;
        items[0].email = "switchedmap@map.map";

        // This trigger another switched read-back.
        emailAfterHashToMapSwitch = originallyAMap.email;
    }
}
