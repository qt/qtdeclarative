// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QScopedPointer>

#include <cstdlib>

#include <QtQmlTypeRegistrar/private/qqmltyperegistrar_p.h>
#include <QtQmlTypeRegistrar/private/qqmltyperegistrarutils_p.h>

using namespace Qt::Literals;

int main(int argc, char **argv)
{
    // Produce reliably the same output for the same input by disabling QHash's random seeding.
    QHashSeed::setDeterministicGlobalSeed();

    // No, you are not supposed to mess with the message pattern.
    // Qt Creator wants to read those messages as-is and we want the convenience
    // of QDebug to print them.
    qputenv("QT_MESSAGE_PATTERN", "%{if-category}%{category}: %{endif}%{message}");

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qmltyperegistrar"));
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outputOption(QStringLiteral("o"));
    outputOption.setDescription(QStringLiteral("Write output to specified file."));
    outputOption.setValueName(QStringLiteral("file"));
    outputOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(outputOption);

    QCommandLineOption privateIncludesOption(
            QStringLiteral("private-includes"),
            QStringLiteral("Include headers ending in \"_p.h\" using \"#include <private/foo_p.h>\""
                           "rather than \"#include <foo_p.h>\"."));
    parser.addOption(privateIncludesOption);

    QCommandLineOption importNameOption(QStringLiteral("import-name"));
    importNameOption.setDescription(QStringLiteral("Name of the module to use for type and module "
                                                   "registrations."));
    importNameOption.setValueName(QStringLiteral("module name"));
    parser.addOption(importNameOption);

    QCommandLineOption pastMajorVersionOption(QStringLiteral("past-major-version"));
    pastMajorVersionOption.setDescription(
            QStringLiteral("Past major version to use for type and module "
                           "registrations."));
    pastMajorVersionOption.setValueName(QStringLiteral("past major version"));
    parser.addOption(pastMajorVersionOption);

    QCommandLineOption majorVersionOption(QStringLiteral("major-version"));
    majorVersionOption.setDescription(QStringLiteral("Major version to use for type and module "
                                                     "registrations."));
    majorVersionOption.setValueName(QStringLiteral("major version"));
    parser.addOption(majorVersionOption);

    QCommandLineOption minorVersionOption(QStringLiteral("minor-version"));
    minorVersionOption.setDescription(QStringLiteral("Minor version to use for module "
                                                     "registration."));
    minorVersionOption.setValueName(QStringLiteral("minor version"));
    parser.addOption(minorVersionOption);

    QCommandLineOption namespaceOption(QStringLiteral("namespace"));
    namespaceOption.setDescription(QStringLiteral("Generate type registration functions "
                                                  "into a C++ namespace."));
    namespaceOption.setValueName(QStringLiteral("namespace"));
    parser.addOption(namespaceOption);

    QCommandLineOption pluginTypesOption(QStringLiteral("generate-qmltypes"));
    pluginTypesOption.setDescription(QStringLiteral("Generate qmltypes into specified file."));
    pluginTypesOption.setValueName(QStringLiteral("qmltypes file"));
    parser.addOption(pluginTypesOption);

    QCommandLineOption foreignTypesOption(QStringLiteral("foreign-types"));
    foreignTypesOption.setDescription(
            QStringLiteral("Comma separated list of other modules' metatypes files "
                           "to consult for foreign types when generating "
                           "qmltypes file."));
    foreignTypesOption.setValueName(QStringLiteral("foreign types"));
    parser.addOption(foreignTypesOption);

    QCommandLineOption followForeignVersioningOption(QStringLiteral("follow-foreign-versioning"));
    followForeignVersioningOption.setDescription(
            QStringLiteral("If this option is set the versioning scheme of foreign base classes "
                           "will be respected instead of ignored. Mostly useful for modules who "
                           "want to follow Qt's versioning scheme."));
    parser.addOption(followForeignVersioningOption);

    QCommandLineOption jsroot(QStringLiteral("jsroot"));
    jsroot.setDescription(
            QStringLiteral("Use the JavaScript root object's meta types as sole input and do not "
                           "generate any C++ output. Only useful in combination with "
                           "--generate-qmltypes"));
    parser.addOption(jsroot);

    QCommandLineOption extract(u"extract"_s);
    extract.setDescription(
            u"Extract QML types from a module and use QML_FOREIGN to register them"_s);
    parser.addOption(extract);

    QCommandLineOption mergeQtConf("merge-qt-conf"_L1);
    mergeQtConf.setValueName("qtconf list"_L1);
    mergeQtConf.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(mergeQtConf);

    parser.addPositionalArgument(QStringLiteral("[MOC generated json file]"),
                                 QStringLiteral("MOC generated json output."));

    QStringList arguments;
    if (!QmlTypeRegistrar::argumentsFromCommandLineAndFile(arguments, app.arguments()))
        return EXIT_FAILURE;

    parser.process(arguments);

    if (parser.isSet(mergeQtConf)) {
        return mergeQtConfFiles(parser.value(mergeQtConf));
    }

    const QString module = parser.value(importNameOption);

    const QLatin1String jsrootMetaTypes
            = QLatin1String(":/qt-project.org/meta_types/jsroot_metatypes.json");
    QStringList files = parser.positionalArguments();
    if (parser.isSet(jsroot)) {
        if (parser.isSet(extract)) {
            error(module) << "If --jsroot is passed, no type registrations can be extracted.";
            return EXIT_FAILURE;
        }
        if (parser.isSet(outputOption)) {
            error(module) << "If --jsroot is passed, no C++ output can be generated.";
            return EXIT_FAILURE;
        }
        if (!files.isEmpty() || parser.isSet(foreignTypesOption)) {
            error(module) << "If --jsroot is passed, no further metatypes can be processed.";
            return EXIT_FAILURE;
        }

        files.append(jsrootMetaTypes);
    }

    MetaTypesJsonProcessor processor(parser.isSet(privateIncludesOption));
    if (!processor.processTypes(files))
        return EXIT_FAILURE;

    if (parser.isSet(extract)) {
        if (!parser.isSet(outputOption)) {
            error(module) << "The output file name must be provided";
            return EXIT_FAILURE;
        }
        QString baseName = parser.value(outputOption);
        return QmlTypeRegistrar::runExtract(baseName, parser.value(namespaceOption), processor);
    }

    processor.postProcessTypes();

    if (!parser.isSet(jsroot)) {
        processor.processForeignTypes(jsrootMetaTypes);
        if (parser.isSet(foreignTypesOption))
            processor.processForeignTypes(parser.value(foreignTypesOption).split(QLatin1Char(',')));
    }

    processor.postProcessForeignTypes();

    QmlTypeRegistrar typeRegistrar;
    typeRegistrar.setIncludes(processor.includes());
    typeRegistrar.setModuleNameAndNamespace(module, parser.value(namespaceOption));
    QTypeRevision moduleVersion = QTypeRevision::fromVersion(
            parser.value(majorVersionOption).toInt(), parser.value(minorVersionOption).toInt());
    QList<quint8> pastMajorVersions;
    for (const auto &x : parser.values(pastMajorVersionOption))
        pastMajorVersions.append(x.toUInt());

    typeRegistrar.setModuleVersions(moduleVersion, pastMajorVersions,
                                    parser.isSet(followForeignVersioningOption));
    typeRegistrar.setTypes(processor.types(), processor.foreignTypes());

    if (!parser.isSet(jsroot)) {
        if (module.isEmpty()) {
            warning(module) << "The module name is empty. Cannot generate C++ code";
        } else if (parser.isSet(outputOption)) {
            // extract does its own file handling
            QString outputName = parser.value(outputOption);
            QFile file(outputName);
            if (!file.open(QIODeviceBase::WriteOnly)) {
                error(QDir::toNativeSeparators(outputName))
                        << "Cannot open file for writing:" << file.errorString();
                return EXIT_FAILURE;
            }
            QTextStream output(&file);
            typeRegistrar.write(output, outputName);
        } else {
            QTextStream output(stdout);
            typeRegistrar.write(output, "stdout");
        }
    }

    if (!parser.isSet(pluginTypesOption))
        return EXIT_SUCCESS;

    typeRegistrar.setReferencedTypes(processor.referencedTypes());
    typeRegistrar.setUsingDeclarations(processor.usingDeclarations());
    const QString qmltypes = parser.value(pluginTypesOption);
    if (!typeRegistrar.generatePluginTypes(qmltypes, parser.isSet(jsroot))) {
        error(qmltypes) << "Cannot generate qmltypes file";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
