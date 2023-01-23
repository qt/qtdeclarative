import QmltcQProcessTests

TypeWithSignals
{
    onSignalWithConstPointerToGadget: (x) => { console.log(x); }
    onSignalWithConstPointerToGadgetConst: (x) => { console.log(x); }
    onSignalWithPointerToGadgetConst: (x) => { console.log(x); }
    onSignalWithPointerToGadget: (x) => { console.log(x); }
    onSignalWithPrimitivePointer: (x) => { console.log(x); }
    onSignalWithConstPrimitivePointer: (x) => { console.log(x); }
}
