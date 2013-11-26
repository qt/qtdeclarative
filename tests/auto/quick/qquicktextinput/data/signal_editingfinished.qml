import QtQuick 2.2

Item {
    property variant input1: input1
    property variant input2: input2

    width: 800; height: 600;

    Column{
        TextInput { id: input1; }
        TextInput { id: input2; }
    }
}
