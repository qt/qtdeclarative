import QtQml
QtObject {
    signal sig(string arg, int argarg)
    onSig: function(arg, b, c, d) {
        print("SIG", arg, b, c, d);
    }
}

