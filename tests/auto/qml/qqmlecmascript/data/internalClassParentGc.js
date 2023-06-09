function init() {
    Array.prototype.doPush = Array.prototype.push
}

function nasty() {
    var sc_Vector = Array;
    var push = sc_Vector.prototype.doPush;

    // Change the memberData to hold something nasty on the current internalClass
    sc_Vector.prototype.doPush = 5;

    // Trigger a re-allocation of memberData
    for (var i = 0; i < 256; ++i)
        sc_Vector.prototype[i + "string"] = function() { return 98; }

    // Change the (new) memberData back, to hold our doPush function again.
    // This should propagate a protoId change all the way up to the lookup.
    sc_Vector.prototype.doPush = push;
}

function func() {
    var b = [];

    // This becomes a lookup internally, which stores protoId and a pointer
    // into the memberData. It should get invalidated when memberData is re-allocated.
    b.doPush(3);

    return b;
}
