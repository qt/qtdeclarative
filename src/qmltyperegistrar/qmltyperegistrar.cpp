// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltypescreator.h"
#include "metatypesjsonprocessor.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QtDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QScopedPointer>
#include <QSaveFile>
#include <QFileInfo>

#include <cstdlib>

using namespace Qt::StringLiterals;

struct ScopedPointerFileCloser
{
    static inline void cleanup(FILE *handle) { if (handle) fclose(handle); }
};

static bool argumentsFromCommandLineAndFile(QStringList &allArguments, const QStringList &arguments)
{
    allArguments.reserve(arguments.size());
    for (const QString &argument : arguments) {
        // "@file" doesn't start with a '-' so we can't use QCommandLineParser for it
        if (argument.startsWith(QLatin1Char('@'))) {
            QString optionsFile = argument;
            optionsFile.remove(0, 1);
            if (optionsFile.isEmpty()) {
                fprintf(stderr, "The @ option requires an input file");
                return false;
            }
            QFile f(optionsFile);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                fprintf(stderr, "Cannot open options file specified with @");
                return false;
            }
            while (!f.atEnd()) {
                QString line = QString::fromLocal8Bit(f.readLine().trimmed());
                if (!line.isEmpty())
                    allArguments << line;
            }
        } else {
            allArguments << argument;
        }
    }
    return true;
}

static int runExtract(const QString & baseName, const MetaTypesJsonProcessor &processor) {
    if (processor.types().isEmpty()) {
        fprintf(stderr, "Error: No types to register found in library\n");
        return EXIT_FAILURE;
    }
    QFile headerFile(baseName + u".h");
    bool ok = headerFile.open(QFile::WriteOnly);
    if (!ok) {
        fprintf(stderr, "Error: Cannot open %s for writing\n", qPrintable(headerFile.fileName()));
        return EXIT_FAILURE;
    }
    auto prefix = QString::fromLatin1(
            "#ifndef %1_H\n"
            "#define %1_H\n"
            "#include <QtQml/qqml.h>\n"
            "#include <QtQml/qqmlmoduleregistration.h>\n").arg(baseName.toUpper());
    const QStringList includes = processor.includes();
    for (const QString &include: includes)
        prefix += u"\n#include <%1>"_s.arg(include);
    headerFile.write((prefix + processor.extractRegisteredTypes()).toUtf8() + "\n#endif");

    QFile sourceFile(baseName + u".cpp");
    ok = sourceFile.open(QFile::WriteOnly);
    if (!ok) {
        fprintf(stderr, "Error: Cannot open %s for writing\n", qPrintable(sourceFile.fileName()));
        return EXIT_FAILURE;
    }
    // the string split is necessaury because cmake's automoc scanner would otherwise pick up the include
    QString code = u"#include \"%1.h\"\n#include "_s.arg(baseName);
    code += uR"("moc_%1.cpp")"_s.arg(baseName);
    sourceFile.write(code.toUtf8());
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    // Produce reliably the same output for the same input by disabling QHash's random seeding.
    qSetGlobalQHashSeed(0);

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
    pastMajorVersionOption.setDescription(QStringLiteral("Past major version to use for type and module "
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
    foreignTypesOption.setDescription(QStringLiteral(
                                          "Comma separated list of other modules' metatypes files "
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

    QCommandLineOption extract(u"extract"_s);
    extract.setDescription(u"Extract QML types from a module and use QML_FOREIGN to register them"_s);
    parser.addOption(extract);

    parser.addPositionalArgument(QStringLiteral("[MOC generated json file]"),
                                 QStringLiteral("MOC generated json output."));

    QStringList arguments;
    if (!argumentsFromCommandLineAndFile(arguments, app.arguments()))
        return EXIT_FAILURE;

    parser.process(arguments);

    const QString module = parser.value(importNameOption);

    MetaTypesJsonProcessor processor(parser.isSet(privateIncludesOption));
    if (!processor.processTypes(parser.positionalArguments()))
        return EXIT_FAILURE;

    processor.postProcessTypes();

    if (parser.isSet(foreignTypesOption))
        processor.processForeignTypes(parser.value(foreignTypesOption).split(QLatin1Char(',')));

    processor.postProcessForeignTypes();


    if (parser.isSet(extract)) {
        if (!parser.isSet(outputOption)) {
            fprintf(stderr, "Error: The output file name must be provided\n");
            return EXIT_FAILURE;
        }
        QString baseName = parser.value(outputOption);
        return runExtract(baseName, processor);
    }

    FILE *output = stdout;
    QScopedPointer<FILE, ScopedPointerFileCloser> outputFile;


    if (parser.isSet(outputOption)) {
        // extract does its own file handling
        QString outputName = parser.value(outputOption);
#if defined(_MSC_VER)
        if (_wfopen_s(&output, reinterpret_cast<const wchar_t *>(outputName.utf16()), L"w") != 0) {
#else
        output = fopen(QFile::encodeName(outputName).constData(), "w"); // create output file
        if (!output) {
#endif
            fprintf(stderr, "Error: Cannot open %s for writing\n", qPrintable(outputName));
            return EXIT_FAILURE;
        }
        outputFile.reset(output);
    }

    fprintf(output,
            "/****************************************************************************\n"
            "** Generated QML type registration code\n**\n");
    fprintf(output,
            "** WARNING! All changes made in this file will be lost!\n"
            "*****************************************************************************/\n\n");
    fprintf(output,
            "#include <QtQml/qqml.h>\n"
            "#include <QtQml/qqmlmoduleregistration.h>\n");

    const QStringList includes = processor.includes();
    for (const QString &include : includes)
        fprintf(output, "\n#include <%s>", qPrintable(include));

    fprintf(output, "\n\n");

    // Keep this in sync with _qt_internal_get_escaped_uri in CMake
    QString moduleAsSymbol = module;
    moduleAsSymbol.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9]")), QStringLiteral("_"));

    QString underscoredModuleAsSymbol = module;
    underscoredModuleAsSymbol.replace(QLatin1Char('.'), QLatin1Char('_'));

    if (underscoredModuleAsSymbol != moduleAsSymbol
            || underscoredModuleAsSymbol.isEmpty()
            || underscoredModuleAsSymbol.front().isDigit()) {
        qWarning() << module << "is an invalid QML module URI. You cannot import this.";
    }

    const QString functionName = QStringLiteral("qml_register_types_") + moduleAsSymbol;
    fprintf(output,
            "#if !defined(QT_STATIC)\n"
            "#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT\n"
            "#else\n"
            "#define Q_QMLTYPE_EXPORT\n"
            "#endif\n"
            "\n");

    const QString targetNamespace = parser.value(namespaceOption);
    if (!targetNamespace.isEmpty())
        fprintf(output, "namespace %s {\n", qPrintable(targetNamespace));

    fprintf(output, "Q_QMLTYPE_EXPORT void %s()\n{", qPrintable(functionName));
    const auto majorVersion = parser.value(majorVersionOption);
    const auto pastMajorVersions = parser.values(pastMajorVersionOption);
    const auto minorVersion = parser.value(minorVersionOption);
    const bool followForeignVersioning = parser.isSet(followForeignVersioningOption);

    for (const auto &version : pastMajorVersions) {
        fprintf(output, "\n    qmlRegisterModule(\"%s\", %s, 0);\n    qmlRegisterModule(\"%s\", %s, 254);",
                qPrintable(module), qPrintable(version), qPrintable(module), qPrintable(version));
    }

    if (minorVersion.toInt() != 0) {
        fprintf(output, "\n    qmlRegisterModule(\"%s\", %s, 0);",
                qPrintable(module), qPrintable(majorVersion));
    }

    auto moduleVersion = QTypeRevision::fromVersion(majorVersion.toInt(), minorVersion.toInt());

    const QVector<QJsonObject> types = processor.types();
    const QVector<QJsonObject> foreignTypes = processor.foreignTypes();
    QVector<QString> typesRegisteredAnonymously;

    const auto &findType = [&](const QString &name) -> QJsonValue {
        for (const QJsonObject &type : types) {
            if (type[QLatin1String("qualifiedClassName")] != name)
                continue;
            return type;
        }
        return QJsonValue();
    };

    const auto &findTypeForeign = [&](const QString &name) -> QJsonValue {
        for (const QJsonObject &type : foreignTypes) {
            if (type[QLatin1String("qualifiedClassName")] != name)
                continue;
            return type;
        }
        return QJsonValue();
    };

    for (const QJsonObject &classDef : types) {
        const QString className = classDef[QLatin1String("qualifiedClassName")].toString();

        QString targetName = className;
        QString extendedName;
        bool seenQmlElement = false;
        const QJsonArray classInfos = classDef.value(QLatin1String("classInfos")).toArray();
        for (const QJsonValueConstRef v : classInfos) {
            const QString name = v[QStringLiteral("name")].toString();
            if (name == QStringLiteral("QML.Element"))
                seenQmlElement = true;
            else if (name == QStringLiteral("QML.Foreign"))
                targetName = v[QLatin1String("value")].toString();
            else if (name == QStringLiteral("QML.Extended"))
                extendedName = v[QStringLiteral("value")].toString();
        }

        // We want all related metatypes to be registered by name, so that we can look them up
        // without including the C++ headers. That's the reason for the QMetaType(foo).id() calls.

        if (classDef.value(QLatin1String("namespace")).toBool()) {
            // We need to figure out if the _target_ is a namespace. If not, it already has a
            // QMetaType and we don't need to generate one.

            QString targetTypeName = targetName;
            const auto targetIsNamespace = [&]() {
                if (className == targetName)
                    return true;

                const QJsonObject *target = QmlTypesClassDescription::findType(types, targetName);
                if (!target)
                    target = QmlTypesClassDescription::findType(foreignTypes, targetName);

                if (!target)
                    return false;

                if (target->value(QStringLiteral("namespace")).toBool())
                    return true;

                if (target->value(QStringLiteral("object")).toBool())
                    targetTypeName += QStringLiteral(" *");

                return false;
            };

            if (targetIsNamespace()) {
                fprintf(output, "\n    {");
                fprintf(output, "\n        static const auto metaType "
                                "= QQmlPrivate::metaTypeForNamespace("
                                "[](const QtPrivate::QMetaTypeInterface *) { "
                                "return &%s::staticMetaObject; "
                                "}, \"%s\");",
                        qPrintable(targetName), qPrintable(targetTypeName));
                fprintf(output, "\n        QMetaType(&metaType).id();");
                fprintf(output, "\n    }");
            } else {
                fprintf(output, "\n    QMetaType::fromType<%s>().id();",
                        qPrintable(targetTypeName));
            }

            auto metaObjectPointer = [](const QString &name) -> QString {
                return u'&' + name + QStringLiteral("::staticMetaObject");
            };

            if (seenQmlElement) {
                fprintf(output, "\n    qmlRegisterNamespaceAndRevisions(%s, "
                                "\"%s\", %s, nullptr, %s, %s);",
                        qPrintable(metaObjectPointer(targetName)), qPrintable(module),
                        qPrintable(majorVersion), qPrintable(metaObjectPointer(className)),
                        extendedName.isEmpty() ? "nullptr"
                                               : qPrintable(metaObjectPointer(extendedName)));
            }
        } else {
            if (seenQmlElement) {
                auto checkRevisions = [&](const QJsonArray &array, const QString &type) {
                    for (auto it = array.constBegin(); it != array.constEnd(); ++it) {
                        auto object = it->toObject();
                        if (!object.contains(QLatin1String("revision")))
                            continue;

                        QTypeRevision revision = QTypeRevision::fromEncodedVersion(object[QLatin1String("revision")].toInt());
                        if (moduleVersion < revision) {
                            qWarning().noquote()
                                    << "Warning:" << className << "is trying to register" << type
                                    << object[QStringLiteral("name")].toString()
                                    << "with future version" << revision
                                    << "when module version is only" << moduleVersion;
                        }
                    }
                };

                const QJsonArray methods = classDef[QLatin1String("methods")].toArray();
                const QJsonArray properties = classDef[QLatin1String("properties")].toArray();

                if (moduleVersion.isValid()) {
                    checkRevisions(properties, QLatin1String("property"));
                    checkRevisions(methods, QLatin1String("method"));
                }

                fprintf(output, "\n    qmlRegisterTypesAndRevisions<%s>(\"%s\", %s);",
                        qPrintable(className), qPrintable(module), qPrintable(majorVersion));

                const QJsonValue superClasses = classDef[QLatin1String("superClasses")];

                if (superClasses.isArray()) {
                    for (const QJsonValueRef object : superClasses.toArray()) {
                        if (object[QStringLiteral("access")] != QStringLiteral("public"))
                            continue;

                        QString superClassName = object[QStringLiteral("name")].toString();

                        QVector<QString> classesToCheck;

                        auto checkForRevisions = [&](const QString &typeName) -> void {
                            auto type = findType(typeName);

                            if (!type.isObject()) {
                                type = findTypeForeign(typeName);
                                if (!type.isObject())
                                    return;

                                for (const QString &section :
                                     { QStringLiteral("properties"), QStringLiteral("signals"),
                                       QStringLiteral("methods") }) {
                                    bool foundRevisionEntry = false;
                                    for (const QJsonValueRef entry : type[section].toArray()) {
                                        if (entry.toObject().contains(QStringLiteral("revision"))) {
                                            foundRevisionEntry = true;
                                            break;
                                        }
                                    }
                                    if (foundRevisionEntry) {
                                        if (typesRegisteredAnonymously.contains(typeName))
                                            break;

                                        typesRegisteredAnonymously.append(typeName);

                                        if (followForeignVersioning) {
                                            fprintf(output,
                                                    "\n    "
                                                    "qmlRegisterAnonymousTypesAndRevisions<%s>(\"%"
                                                    "s\", "
                                                    "%s);",
                                                    qPrintable(typeName), qPrintable(module),
                                                    qPrintable(majorVersion));
                                            break;
                                        }

                                        for (const QString &version :
                                             pastMajorVersions + QStringList { majorVersion }) {
                                            fprintf(output,
                                                    "\n    "
                                                    "qmlRegisterAnonymousType<%s, 254>(\"%s\", "
                                                    "%s);",
                                                    qPrintable(typeName), qPrintable(module),
                                                    qPrintable(version));
                                        }
                                        break;
                                    }
                                }
                            }

                            const QJsonValue superClasses = type[QLatin1String("superClasses")];

                            if (superClasses.isArray()) {
                                for (const QJsonValueRef object : superClasses.toArray()) {
                                    if (object[QStringLiteral("access")]
                                        != QStringLiteral("public"))
                                        continue;
                                    classesToCheck << object[QStringLiteral("name")].toString();
                                }
                            }
                        };

                        checkForRevisions(superClassName);

                        while (!classesToCheck.isEmpty())
                            checkForRevisions(classesToCheck.takeFirst());
                    }
                }
            } else {
                fprintf(output, "\n    QMetaType::fromType<%s%s>().id();",
                        qPrintable(className),
                        classDef.value(QLatin1String("object")).toBool() ? " *" : "");
            }
        }
    }

    fprintf(output, "\n    qmlRegisterModule(\"%s\", %s, %s);",
            qPrintable(module), qPrintable(majorVersion), qPrintable(minorVersion));
    fprintf(output, "\n}\n");
    fprintf(output, "\nstatic const QQmlModuleRegistration registration(\"%s\", %s);\n",
            qPrintable(module), qPrintable(functionName));

    if (!targetNamespace.isEmpty())
        fprintf(output, "} // namespace %s\n", qPrintable(targetNamespace));

    if (!parser.isSet(pluginTypesOption))
        return EXIT_SUCCESS;

    QmlTypesCreator creator;
    creator.setOwnTypes(processor.types());
    creator.setForeignTypes(processor.foreignTypes());
    creator.setReferencedTypes(processor.referencedTypes());
    creator.setModule(module);
    creator.setVersion(QTypeRevision::fromVersion(parser.value(majorVersionOption).toInt(), 0));

    creator.generate(parser.value(pluginTypesOption));
    return EXIT_SUCCESS;
}
