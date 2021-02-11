import QtQuick 2.0
Window {
    width: 800
    height: 600
    visible: true
    signal sig(string arg, int argarg)
    signal sig2(int foo, bool bar)

    onSig: function(argarg) {
        print("SIG", argarg);
    }

    onSig2: (foo, bar)=> { sig(foo, bar); }
}

