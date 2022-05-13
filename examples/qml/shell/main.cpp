// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qscopedpointer.h>

#include <QtCore/QCoreApplication>

#include <QtQml/qjsengine.h>

#include <stdlib.h>


class CommandInterface : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void quit() { m_wantsToQuit = true; }
    static bool wantsToQuit() { return m_wantsToQuit; }
private:
    static bool m_wantsToQuit;
};

bool CommandInterface::m_wantsToQuit = false;


static void interactive(QJSEngine *eng)
{
    QTextStream qin(stdin, QFile::ReadOnly);
    const char *prompt = "qs> ";

    forever {
        QString line;

        printf("%s", prompt);
        fflush(stdout);

        line = qin.readLine();
        if (line.isNull())
            break;

        if (line.trimmed().isEmpty())
            continue;

        line += QLatin1Char('\n');

        QJSValue result = eng->evaluate(line, QLatin1String("typein"));

        fprintf(stderr, "%s\n", qPrintable(result.toString()));

        if (CommandInterface::wantsToQuit())
            break;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QScopedPointer<QJSEngine> eng(new QJSEngine());
    {
        QJSValue globalObject = eng->globalObject();
        QJSValue interface = eng->newQObject(new CommandInterface);
        globalObject.setProperty("qt", interface);
    }

    if (! *++argv) {
        interactive(eng.data());
        return EXIT_SUCCESS;
    }

    while (const char *arg = *argv++) {
        QString fileName = QString::fromLocal8Bit(arg);

        if (fileName == QLatin1String("-i")) {
            interactive(eng.data());
            break;
        }

        QString contents;
        int lineNumber = 1;

        if (fileName == QLatin1String("-")) {
            QTextStream stream(stdin, QFile::ReadOnly);
            contents = stream.readAll();
        } else {
            QFile file(fileName);
            if (file.open(QFile::ReadOnly)) {
                QTextStream stream(&file);
                contents = stream.readAll();
                file.close();

                // strip off #!/usr/bin/env qjs line
                if (contents.startsWith("#!")) {
                    contents.remove(0, contents.indexOf("\n"));
                    ++lineNumber;
                }
            }
        }

        if (contents.isEmpty())
            continue;

        QJSValue result = eng->evaluate(contents, fileName, lineNumber);
        if (result.isError()) {
            fprintf (stderr, "    %s\n\n", qPrintable(result.toString()));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

#include <main.moc>
