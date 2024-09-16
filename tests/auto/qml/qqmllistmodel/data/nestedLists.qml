import QtQuick

Item {
    id: mainWindow

    function load(data) {
        model.clear()
        for (var i = 0; i < data.length; i++)
            model.append(data[i])
    }

    ListModel {
        id: model
    }

    Repeater {
        objectName: "topLevel"
        model: model
        Item {
            objectName: _headline
            Repeater {
                objectName: "month"
                model: _weeks
                Item {
                    objectName: index
                    Repeater {
                        objectName: "week"
                        model: _week
                        Item {
                            objectName: _day
                        }
                    }
                }
            }
        }
    }
}
