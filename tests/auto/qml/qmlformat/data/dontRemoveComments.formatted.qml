Item {
    property var test: [{
            // Testing
            "foo": "bar"
        }]

    onTestChanged: {
        fooBar(test, {
                // Testing
                "foo": "bar"
            });
    }
}
