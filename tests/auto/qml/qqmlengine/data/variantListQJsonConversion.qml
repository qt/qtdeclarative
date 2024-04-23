import QtQuick
import OnlyDeclarative

Item {

    MiscUtils {
        id: miscUtils
    }

    Component.onCompleted: {
        const varlist = miscUtils.createVariantList();
        const obj = { test: varlist };
        const listProperty = miscUtils.createQmlListProperty();
        miscUtils.logArray(varlist);
        miscUtils.logObject(obj);
        miscUtils.logArray(listProperty);
    }
}
