import QtQuick

Item {
    // qmltc should see that rectangle is used as the type argument
    // for the list and produce an import for QQuickRectangle.
    // Failure to do so will produce code that cannot compile and the
    // test will fail at build time.
    property list<Rectangle> listWithUniqueReferenceToType : []
}
