import CallJSValue

TypeWithQJSValue {
    // Note: jsValue is not actually callable
    Component.onCompleted: jsValue(42);
}
