import QtQuick

Canvas {
    onPaint: {
        // Does not actually compile to C++ because getContext() is not properly specified
        // However, it shouldn't crash
        var ctx = getContext("2d");
        ctx.clearRect(0, 0, width, height);
    }
}
