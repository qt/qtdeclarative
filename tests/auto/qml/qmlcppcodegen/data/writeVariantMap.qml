pragma Strict
import StringBuilderTestTypes

WritableVariantMap {
    id: dragSource
    property string modelData: "Drag Me"
    data: ({
        "text/plain": "%" + dragSource.modelData + "%"
    })
}
