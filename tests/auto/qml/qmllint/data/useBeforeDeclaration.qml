import QtQml
QtObject {
    signal sig()
    onSig: ()=> {
        argq = 12;
        var argq;
    }
}
