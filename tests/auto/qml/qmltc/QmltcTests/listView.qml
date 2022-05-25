import QtQuick

Item {
    id: display

    function appendDigit(digit)
    {
        listView.model.append({ "operator": "", "operand": "" });
        var i = listView.model.count - 1;
        listView.model.get(i).operand = digit;
        listView.positionViewAtEnd();
    }

    function appendOperator(operatorValue)
    {
        listView.model.append({ "operator": "", "operand": "" });
        var i = listView.model.count - 1;
        listView.model.get(i).operator = operatorValue;
        listView.positionViewAtEnd();
    }

    ListView {
        id: listView
        delegate: Item {
            id: delegateItem
            Text {
                id: operatorText
                text: model.operator ?? ""
            }
            Text {
                id: operand
                text: model.operand ?? ""
            }
        }
        model: ListModel { }
    }
}
