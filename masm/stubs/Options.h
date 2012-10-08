#ifndef OPTIONS_H
#define OPTIONS_H

namespace JSC {

struct Options {
    static bool showDisassembly() { return true; }
    static bool showDFGDisassembly() { return true; }
};

}

#endif // MASM_STUBS/OPTIONS_H
