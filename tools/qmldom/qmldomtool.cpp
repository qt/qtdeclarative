// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/QTextStream>
#include <QtCore/QThread>

#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomfilewriter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomelements_p.h>
#include <QtQmlDom/private/qqmldomfieldfilter_p.h>
#include <QtQmlDom/private/qqmldomastdumper_p.h>

#include <cstdio>
#include <optional>

#if QT_CONFIG(commandlineparser)
#    include <QtCore/qcommandlineparser.h>
#endif

#include <QtCore/qlibraryinfo.h>
using namespace QQmlJS::Dom;

namespace tt {
Q_NAMESPACE

enum class Dependencies { None, Required };
Q_ENUM_NS(Dependencies);

};
using namespace tt;

int main(int argc, char *argv[])
{
    FieldFilter filter = FieldFilter::defaultFilter();
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("qmldom");
    QCoreApplication::setApplicationVersion("1.0");
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    parser.setApplicationDescription(QLatin1String("QML dom tool"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dumpOption(QStringList() << "d"
                                                << "dump",
                                  QLatin1String("Dumps the code model"));
    parser.addOption(dumpOption);
    QCommandLineOption reformatOption(QStringList() << "r"
                                                    << "reformat",
                                      QLatin1String("reformats the files explicitly passed in"));
    parser.addOption(reformatOption);

    QCommandLineOption filterOption(
            QStringList() << "f"
                          << "filter-fields",
            QLatin1String("commas separated list of fields to filter out. Prepending a field with "
                          "'-' skips the field, with '+' it adds it. The field might be prepended "
                          "by '<type>:' to apply only to elements of that type"
                          "The default filters are ")
                    + filter.describeFieldsFilter(),
            QLatin1String("fields"));
    parser.addOption(filterOption);

    QCommandLineOption qmltypesDirsOption(
            QStringList() << "I"
                          << "qmldirs",
            QLatin1String("Look for qmltypes files in specified directory"),
            QLatin1String("directory"));
    parser.addOption(qmltypesDirsOption);

    QCommandLineOption qmltypesFilesOption(QStringList() << "i"
                                                         << "qmltypes",
                                           QLatin1String("Include the specified qmltypes files"),
                                           QLatin1String("qmltypes"));
    parser.addOption(qmltypesFilesOption);

    QCommandLineOption pathToDumpOption(
            QStringList() << "path-to-dump",
            QLatin1String("adds a path to dump. By default the base path of each file is dumped. "
                          "If any path starts with $ ($env for example) then the environment (and "
                          "not the loaded files) is used as basis."),
            QLatin1String("pathToDump"));
    parser.addOption(pathToDumpOption);

    QCommandLineOption dependenciesOption(
            QStringList() << "D"
                          << "dependencies",
            QLatin1String("Dependencies to load: none, required, reachable"),
            QLatin1String("dependenciesToLoad"), QLatin1String("none"));
    parser.addOption(dependenciesOption);

    QCommandLineOption reformatDirOption(
            QStringList() << "reformat-dir",
            QLatin1String(
                    "Target directory for the reformatted files, "
                    "if not given the files are reformatted in place (but backup files are kept)"),
            QLatin1String("reformatDir"));
    parser.addOption(reformatDirOption);

    QCommandLineOption nBackupsOption(
            QStringList() << "backups",
            QLatin1String("Number of backup files to generate (default is 2, the oldest, "
                          "and the last version are kept), "),
            QLatin1String("nBackups"));
    parser.addOption(nBackupsOption);

    QCommandLineOption dumpAstOption(QStringList() << "dump-ast",
                                     QLatin1String("Dumps the AST of the given QML file."));
    parser.addOption(dumpAstOption);

    parser.addPositionalArgument(QLatin1String("files"),
                                 QLatin1String("list of qml or js files to verify"));

    parser.process(a);

    const auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        parser.showHelp(-1);
    }

    if (parser.isSet(filterOption)) {
        qDebug() << "filters: " << parser.values(filterOption);
        for (const QString &fFields : parser.values(filterOption)) {
            if (!filter.addFilter(fFields)) {
                return 1;
            }
        }
        filter.setFiltred();
    }

    std::optional<DomType> fileType;
    if (parser.isSet(reformatOption))
        fileType = DomType::QmlFile;

    Dependencies dep = Dependencies::None;
    for (const QString &depName : parser.values(dependenciesOption)) {
        QMetaEnum metaEnum = QMetaEnum::fromType<Dependencies>();
        bool found = false;
        for (int i = 0; i < metaEnum.keyCount(); ++i) {
            if (QLatin1String(metaEnum.key(i)).compare(depName, Qt::CaseInsensitive) == 0) {
                found = true;
                dep = Dependencies(metaEnum.value(i));
            }
        }
        if (!found) {
            QStringList values;
            for (int i = 0; i < metaEnum.keyCount(); ++i)
                values.append(QString::fromUtf8(metaEnum.key(i)).toLower());
            qDebug().noquote() << "Invalid dependencies argument, expected one of "
                               << values.join(QLatin1Char(','));
            return 1;
        }
    }

    int nBackups = 2;
    if (parser.isSet(nBackupsOption)) {
        bool intOk;
        nBackups = parser.value(nBackupsOption).toInt(&intOk);
        if (!intOk) {
            qDebug() << "expected an integer giving the number of backups after --backups, not "
                     << parser.value(nBackupsOption);
        }
    }

    QList<Path> pathsToDump;
    for (const QString &pStr : parser.values(pathToDumpOption)) {
        pathsToDump.append(Path::fromString(pStr));
    }
    if (pathsToDump.isEmpty())
        pathsToDump.append(Path());

    // use host qml import path as a sane default if nothing else has been provided
    QStringList qmltypeDirs = parser.isSet(qmltypesDirsOption)
            ? parser.values(qmltypesDirsOption)
            : QStringList { QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) };

    if (!parser.isSet(qmltypesFilesOption))
        qmltypeDirs << ".";

    QStringList qmltypeFiles =
            parser.isSet(qmltypesFilesOption) ? parser.values(qmltypesFilesOption) : QStringList {};
#else
    QStringList qmltypeDirs {};
    QStringList qmltypeFiles {};
#endif

    {
        QDebug dbg = qDebug();
        dbg << "dirs:\n";
        for (const QString &d : std::as_const(qmltypeDirs))
            dbg << "    '" << d << "'\n";
        dbg << "files:\n";
        for (const QString &f : std::as_const(positionalArguments))
            dbg << "    '" << f << "'\n";
        dbg << "fieldFilter: " << filter.describeFieldsFilter();
        dbg << "\n";
    }
    DomEnvironment::Options options = DomEnvironment::Option::SingleThreaded;
    if (dep == Dependencies::None)
        options = options | DomEnvironment::Option::NoDependencies;
    std::shared_ptr<DomEnvironment> envPtr(new DomEnvironment(qmltypeDirs, options));
    DomItem env(envPtr);
    qDebug() << "will load\n";
    if (dep != Dependencies::None)
        envPtr->loadBuiltins();
    QList<DomItem> loadedFiles(positionalArguments.size());
    qsizetype iPos = 0;
    for (const QString &s : std::as_const(positionalArguments)) {
        envPtr->loadFile(
                FileToLoad::fromFileSystem(envPtr, s),
                [&loadedFiles, iPos](Path, const DomItem &, const DomItem &newIt) {
                    loadedFiles[iPos] = newIt;
                },
                fileType);
    }
    envPtr->loadPendingDependencies();
    bool hadFailures = false;
    const qsizetype largestFileSizeToCheck = 32000;

    if (parser.isSet(reformatOption)) {
        for (auto &qmlFile : loadedFiles) {
            QString qmlFilePath = qmlFile.canonicalFilePath();
            if (qmlFile.internalKind() != DomType::QmlFile) {
                qWarning() << "cannot reformat" << qmlFile.internalKindStr() << "(" << qmlFilePath
                           << ")";
                continue;
            }
            qDebug() << "reformatting" << qmlFilePath;
            FileWriter fw;
            LineWriterOptions lwOptions;
            WriteOutChecks checks = WriteOutCheck::Default;
            if (std::shared_ptr<QmlFile> qmlFilePtr = qmlFile.ownerAs<QmlFile>())
                if (qmlFilePtr->code().size() > largestFileSizeToCheck)
                    checks = WriteOutCheck::None;
            QString target = qmlFilePath;
            QString rDir = parser.value(reformatDirOption);
            if (!rDir.isEmpty()) {
                QFileInfo f(qmlFilePath);
                QDir d(rDir);
                target = d.filePath(f.fileName());
            }
            auto res = qmlFile.writeOut(target, nBackups, lwOptions, &fw, checks);
            switch (fw.status) {
            case FileWriter::Status::ShouldWrite:
            case FileWriter::Status::SkippedDueToFailure:
                qWarning() << "failure reformatting " << qmlFilePath;
                break;
            case FileWriter::Status::DidWrite:
                qDebug() << "success";
                break;
            case FileWriter::Status::SkippedEqual:
                qDebug() << "no change";
            }
            hadFailures = hadFailures || !res;
        }
    } else if (parser.isSet(dumpAstOption)) {
        if (pathsToDump.size() > 1) {
            qWarning() << "--dump-ast can only be used with a single file";
            return 1;
        }
        for (auto &fileItem : loadedFiles) {
            const auto file = fileItem.fileObject().ownerAs<QmlFile>();
            if (!file) {
                qWarning() << "cannot dump AST for" << fileItem.canonicalPath();
                qWarning() << "is it a valid QML file?";
                continue;
            }
            const QString ast =
                    QQmlJS::Dom::astNodeDump(file->ast(), AstDumperOption::DumpNode, 1, 0);
            QTextStream ts(stdout);
            ts << ast << Qt::flush;
        }
    } else if (parser.isSet(dumpOption) || !parser.isSet(reformatOption)
               || !parser.isSet(dumpAstOption)) {
        qDebug() << "will dump\n";
        QTextStream ts(stdout);
        auto sink = [&ts](QStringView v) {
            ts << v; /* ts.flush(); */
        };
        qsizetype iPathToDump = 0;
        bool globalPaths = false;
        for (const auto &p : pathsToDump)
            if (p.headKind() == Path::Kind::Root)
                globalPaths = true;
        if (globalPaths)
            loadedFiles = QList<DomItem>({ env });
        bool dumpDict = pathsToDump.size() > 1 || loadedFiles.size() > 1;
        if (dumpDict)
            sink(u"{\n");
        while (iPathToDump < pathsToDump.size()) {
            for (auto &fileItem : loadedFiles) {
                Path p = pathsToDump.at(iPathToDump++ % pathsToDump.size());
                if (dumpDict) {
                    if (iPathToDump > 1)
                        sink(u",\n");
                    sink(u"\"");
                    if (fileItem.internalKind() != DomType::DomEnvironment) {
                        sinkEscaped(sink, fileItem.canonicalFilePath(),
                                    EscapeOptions::NoOuterQuotes);
                        sink(u"/");
                    }
                    sinkEscaped(sink, p.toString(), EscapeOptions::NoOuterQuotes);
                    sink(u"\":\n");
                }
                fileItem.path(p).dump(sink, 0, filter);
            }
        }
        if (dumpDict)
            sink(u"}\n");
        Qt::endl(ts).flush();
    }
    for (int i = 0; i < 100; ++i)
        QThread::yieldCurrentThread(); // let buggy integrations catch up with the output
    // return a.exec();
    return 0;
}

#include "qmldomtool.moc"
