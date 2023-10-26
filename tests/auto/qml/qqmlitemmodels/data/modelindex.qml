import QtQml 2.0
import Test 1.0

ItemModelsTest {
    property bool isValid: modelIndex.valid
    property int row: modelIndex.row
    property int column: modelIndex.column
    property var parent: modelIndex.parent
    property var model: modelIndex.model
    property var internalId: modelIndex.internalId
    property var displayData: modelIndex.data(Qt.DisplayRole)

    onSignalWithModelIndex: {
        isValid = index.valid
        row = index.row
        column = index.column
        parent = index.parent
        model = index.model
        internalId = index.internalId
        displayData = index.data(Qt.DisplayRole)
    }
}
