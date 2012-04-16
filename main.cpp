
#include "qmljs_objects.h"
#include "qv4codegen_p.h"

#include <QtCore>
#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>

#include <iostream>

int main(int argc, char *argv[])
{
    using namespace QQmlJS;

    GC_INIT();

    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    Engine engine;
    foreach (const QString &fn, args) {
        QFile file(fn);
        if (file.open(QFile::ReadOnly)) {
            const QString code = QString::fromUtf8(file.readAll());
            file.close();

            Lexer lexer(&engine);
            lexer.setCode(code, 1, false);
            Parser parser(&engine);

            const bool parsed = parser.parseProgram();

            foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {
                std::cerr << qPrintable(fn) << ':' << m.loc.startLine << ':' << m.loc.startColumn
                          << ": error: " << qPrintable(m.message) << std::endl;
            }

            if (parsed) {
                using namespace AST;
                Program *program = AST::cast<Program *>(parser.rootNode());

                Codegen cg;
                cg(program);
            }
        }
    }
}
