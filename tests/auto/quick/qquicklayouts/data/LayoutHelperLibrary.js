.pragma library


function buildLayout(layoutData, parentItem) {
    let layout = null
    switch (layoutData.type) {
    case "GridLayout":
    case "RowLayout":
    case "ColumnLayout":
        layout = Qt.createQmlObject("import QtQuick.Layouts\n" +
                                    layoutData.type + " {}", parentItem)
        break
    default:
        console.log("data.layout.type not recognized(" + layoutdata.type + ")")
    }
    if (layout) {
        for (let name in layoutData) {
            let val = layoutData[name]
            switch (name) {
            case "items":
                let arrLayoutData = layoutData.items
                for (let i = 0; i < arrLayoutData.length; i++) {
                    let layoutItemDesc = arrLayoutData[i]
                    let strProps = ""
                    for (let keyName in layoutItemDesc) {
                        strProps += "Layout." + keyName + ": " + layoutItemDesc[keyName] + ";"
                    }
                    // For some reason we cannot assign the "Layout." attached properties from
                    // here, so for now we have to serialize them as strings.
                    let rect = Qt.createQmlObject("import QtQuick\nimport QtQuick.Layouts\n\nRectangle { implicitWidth: 20; implicitHeight: 20; " + strProps + "}", layout)
                }
                break;
            case "type":
                break;
            default:
                layout[name] = val
                break;
            }
        }
    }
    return layout
}
