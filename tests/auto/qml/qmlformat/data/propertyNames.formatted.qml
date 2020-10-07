Item {
    function x() {
        var copiedItem = "copied value";
        var computedItem = "computedName";
        var obj = {
            "identifierName": "identifier value",
            "string name": "string value",
            "Infinity": "numeric value",
            [computedItem]: "computed value",
            "copiedItem": copiedItem
        };
    }

}
