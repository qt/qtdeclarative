import QtQml
import QtQml.Models

ListModel {
    id: filesModel
    property Component testComponent: Component {
        id: testComponent
        QtObject {
            required property string name
        }
    }
    Component.onCompleted: {
        filesModel.clear()
        for(let i = 0; i < 10; i++) {
            filesModel.append({
                path: testComponent.createObject(null, { name: "" + i })
            })
        }
        gc()
    }
}
