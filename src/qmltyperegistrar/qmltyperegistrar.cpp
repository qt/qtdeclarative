/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <cstdlib>

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

    parser.addPositionalArgument(QStringLiteral("[MOC generated json file]"),
                                 QStringLiteral("MOC generated json output."));

    QStringList arguments;
    if (!argumentsFromCommandLineAndFile(arguments, app.arguments()))
        return EXIT_FAILURE;

    parser.process(arguments);

    FILE *output = stdout;
    QScopedPointer<FILE, ScopedPointerFileCloser> outputFile;

    if (parser.isSet(outputOption)) {
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

    const QString module = parser.value(importNameOption);

    MetaTypesJsonProcessor processor(parser.isSet(privateIncludesOption));
    if (!processor.processTypes(parser.positionalArguments()))
        return EXIT_FAILURE;

    processor.postProcessTypes();

    if (parser.isSet(foreignTypesOption))
        processor.processForeignTypes(parser.value(foreignTypesOption).split(QLatin1Char(',')));

    processor.postProcessForeignTypes();

    const QStringList includes = processor.includes();
    for (const QString &include : includes)
        fprintf(output, "\n#include <%s>", qPrintable(include));

    fprintf(output, "\n\n");

    QString moduleAsSymbol = module;
    moduleAsSymbol.replace(QLatin1Char('.'), QLatin1Char('_'));

    const QString functionName = QStringLiteral("qml_register_types_") + moduleAsSymbol;

    fprintf(output,
            "#if !defined(QT_STATIC)\n"
            "#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT\n"
            "#else\n"
            "#define Q_QMLTYPE_EXPORT\n"
            "#endif\n"
            "\n");
    fprintf(output, "Q_QMLTYPE_EXPORT void %s()\n{", qPrintable(functionName));
    const auto majorVersion = parser.value(majorVersionOption);
    const auto pastMajorVersions = parser.values(pastMajorVersionOption);
    const auto minorVersion = parser.value(minorVersionOption);

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
    for (const QJsonObject &classDef : types) {
        const QString className = classDef[QLatin1String("qualifiedClassName")].toString();

        QString targetName = className;
        bool seenQmlElement = false;
        const QJsonArray classInfos = classDef.value(QLatin1String("classInfos")).toArray();
        for (const QJsonValue v : classInfos) {
            const QString name = v[QStringLiteral("name")].toString();
            if (name == QStringLiteral("QML.Element"))
                seenQmlElement = true;
            else if (name == QStringLiteral("QML.Foreign"))
                targetName = v[QLatin1String("value")].toString();
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

            if (seenQmlElement) {
                fprintf(output, "\n    qmlRegisterNamespaceAndRevisions(&%s::staticMetaObject, "
                                "\"%s\", %s, nullptr, &%s::staticMetaObject);",
                        qPrintable(targetName), qPrintable(module), qPrintable(majorVersion),
                        qPrintable(className));
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
