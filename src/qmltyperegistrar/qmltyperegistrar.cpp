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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QFile>
#include <QScopedPointer>
#include <QSaveFile>
#include <QQueue>

#include <cstdlib>

struct ScopedPointerFileCloser
{
    static inline void cleanup(FILE *handle) { if (handle) fclose(handle); }
};

enum RegistrationMode {
    NoRegistration,
    ObjectRegistration,
    GadgetRegistration,
    NamespaceRegistration
};

static RegistrationMode qmlTypeRegistrationMode(const QJsonObject &classDef)
{
    const QJsonArray classInfos = classDef[QLatin1String("classInfos")].toArray();
    for (const QJsonValue &info: classInfos) {
        const QString name = info[QLatin1String("name")].toString();
        if (name == QLatin1String("QML.Element")) {
            if (classDef[QLatin1String("object")].toBool())
                return ObjectRegistration;
            if (classDef[QLatin1String("gadget")].toBool())
                return GadgetRegistration;
            if (classDef[QLatin1String("namespace")].toBool())
                return NamespaceRegistration;
            qWarning() << "Not registering classInfo which is neither an object, "
                          "nor a gadget, nor a namespace:"
                       << name;
            break;
        }
    }
    return NoRegistration;
}

static QVector<QJsonObject> foreignRelatedTypes(const QVector<QJsonObject> &types,
                                                const QVector<QJsonObject> &foreignTypes)
{
    const QLatin1String classInfosKey("classInfos");
    const QLatin1String nameKey("name");
    const QLatin1String qualifiedClassNameKey("qualifiedClassName");
    const QLatin1String qmlNamePrefix("QML.");
    const QLatin1String qmlForeignName("QML.Foreign");
    const QLatin1String qmlAttachedName("QML.Attached");
    const QLatin1String valueKey("value");
    const QLatin1String superClassesKey("superClasses");
    const QLatin1String accessKey("access");
    const QLatin1String publicAccess("public");

    QSet<QString> processedRelatedNames;
    QQueue<QJsonObject> typeQueue;
    typeQueue.append(types.toList());
    QVector<QJsonObject> relatedTypes;

    // First mark all classes registered from this module as already processed.
    for (const QJsonObject &type : types) {
        processedRelatedNames.insert(type.value(qualifiedClassNameKey).toString());
        const auto classInfos = type.value(classInfosKey).toArray();
        for (const QJsonValue &classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            if (obj.value(nameKey).toString() == qmlForeignName) {
                processedRelatedNames.insert(obj.value(valueKey).toString());
                break;
            }
        }
    }

    // Then mark all classes registered from other modules as already processed.
    // We don't want to generate them again for this module.
    for (const QJsonObject &foreignType : foreignTypes) {
        const auto classInfos = foreignType.value(classInfosKey).toArray();
        bool seenQmlPrefix = false;
        for (const QJsonValue &classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            const QString name = obj.value(nameKey).toString();
            if (!seenQmlPrefix && name.startsWith(qmlNamePrefix)) {
                processedRelatedNames.insert(foreignType.value(qualifiedClassNameKey).toString());
                seenQmlPrefix = true;
            }
            if (name == qmlForeignName) {
                processedRelatedNames.insert(obj.value(valueKey).toString());
                break;
            }
        }
    }

    auto addType = [&](const QString &typeName) {
        if (processedRelatedNames.contains(typeName))
            return;
        processedRelatedNames.insert(typeName);
        if (const QJsonObject *other = QmlTypesClassDescription::findType(foreignTypes, typeName)) {
            relatedTypes.append(*other);
            typeQueue.enqueue(*other);
        }
    };

    // Then recursively iterate the super types and attached types, marking the
    // ones we are interested in as related.
    while (!typeQueue.isEmpty()) {
        const QJsonObject classDef = typeQueue.dequeue();

        const auto classInfos = classDef.value(classInfosKey).toArray();
        for (const QJsonValue &classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            if (obj.value(nameKey).toString() == qmlAttachedName) {
                addType(obj.value(valueKey).toString());
            } else if (obj.value(nameKey).toString() == qmlForeignName) {
                const QString foreignClassName = obj.value(valueKey).toString();
                if (const QJsonObject *other = QmlTypesClassDescription::findType(
                            foreignTypes, foreignClassName)) {
                    const auto otherSupers = other->value(superClassesKey).toArray();
                    if (!otherSupers.isEmpty()) {
                        const QJsonObject otherSuperObject = otherSupers.first().toObject();
                        if (otherSuperObject.value(accessKey).toString() == publicAccess)
                            addType(otherSuperObject.value(nameKey).toString());
                    }

                    const auto otherClassInfos = other->value(classInfosKey).toArray();
                    for (const QJsonValue &otherClassInfo : otherClassInfos) {
                        const QJsonObject obj = otherClassInfo.toObject();
                        if (obj.value(nameKey).toString() == qmlAttachedName) {
                            addType(obj.value(valueKey).toString());
                            break;
                        }
                        // No, you cannot chain QML_FOREIGN declarations. Sorry.
                    }
                    break;
                }
            }
        }

        const auto supers = classDef.value(superClassesKey).toArray();
        if (!supers.isEmpty()) {
            const QJsonObject superObject = supers.first().toObject();
            if (superObject.value(accessKey).toString() == publicAccess)
                addType(superObject.value(nameKey).toString());
        }
    }

    return relatedTypes;
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

    QCommandLineOption dependenciesOption(QStringLiteral("dependencies"));
    dependenciesOption.setDescription(QStringLiteral("JSON file with dependencies to be stated in "
                                                     "qmltypes file."));
    dependenciesOption.setValueName(QStringLiteral("dependencies.json"));
    parser.addOption(dependenciesOption);

    parser.addPositionalArgument(QStringLiteral("[MOC generated json file]"),
                                 QStringLiteral("MOC generated json output."));

    parser.process(app);

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

    QStringList includes;
    QVector<QJsonObject> types;
    QVector<QJsonObject> foreignTypes;

    const QString module = parser.value(importNameOption);
    const QStringList files = parser.positionalArguments();
    for (const QString &source: files) {
        QJsonDocument metaObjects;
        {
            QFile f(source);
            if (!f.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "Error opening %s for reading\n", qPrintable(source));
                return EXIT_FAILURE;
            }
            QJsonParseError error = {0, QJsonParseError::NoError};
            metaObjects = QJsonDocument::fromJson(f.readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                fprintf(stderr, "Error parsing %s\n", qPrintable(source));
                return EXIT_FAILURE;
            }
        }

        const bool privateIncludes = parser.isSet(privateIncludesOption);
        auto resolvedInclude = [&](const QString &include) {
            return (privateIncludes && include.endsWith(QLatin1String("_p.h")))
                    ? QLatin1String("private/") + include
                    : include;
        };

        auto processMetaObject = [&](const QJsonObject &metaObject) {
            const QString include = resolvedInclude(metaObject[QLatin1String("inputFile")].toString());
            const QJsonArray classes = metaObject[QLatin1String("classes")].toArray();
            for (const auto &cls : classes) {
                QJsonObject classDef = cls.toObject();
                classDef.insert(QLatin1String("inputFile"), include);

                switch (qmlTypeRegistrationMode(classDef)) {
                case NamespaceRegistration:
                case GadgetRegistration:
                case ObjectRegistration: {
                    if (!include.endsWith(QLatin1String(".h"))
                            && !include.endsWith(QLatin1String(".hpp"))
                            && !include.endsWith(QLatin1String(".hxx"))
                            && include.contains(QLatin1Char('.'))) {
                        fprintf(stderr,
                                "Class %s is declared in %s, which appears not to be a header.\n"
                                "The compilation of its registration to QML may fail.\n",
                                qPrintable(classDef.value(QLatin1String("qualifiedClassName"))
                                           .toString()),
                                qPrintable(include));
                    }
                    includes.append(include);
                    classDef.insert(QLatin1String("registerable"), true);

                    types.append(classDef);
                    break;
                }
                case NoRegistration:
                    foreignTypes.append(classDef);
                    break;
                }
            }
        };

        if (metaObjects.isArray()) {
            const QJsonArray metaObjectsArray = metaObjects.array();
            for (const auto &metaObject : metaObjectsArray) {
                if (!metaObject.isObject()) {
                    fprintf(stderr, "Error parsing %s: JSON is not an object\n",
                            qPrintable(source));
                    return EXIT_FAILURE;
                }

                processMetaObject(metaObject.toObject());
            }
        } else if (metaObjects.isObject()) {
            processMetaObject(metaObjects.object());
        } else {
            fprintf(stderr, "Error parsing %s: JSON is not an object or an array\n",
                    qPrintable(source));
            return EXIT_FAILURE;
        }
    }

    const QLatin1String qualifiedClassNameKey("qualifiedClassName");
    auto sortTypes = [&](QVector<QJsonObject> &types) {
        std::sort(types.begin(), types.end(), [&](const QJsonObject &a, const QJsonObject &b) {
            return a.value(qualifiedClassNameKey).toString() <
                    b.value(qualifiedClassNameKey).toString();
        });
    };

    sortTypes(types);

    std::sort(includes.begin(), includes.end());
    const auto newEnd = std::unique(includes.begin(), includes.end());
    includes.erase(newEnd, includes.end());

    for (const QString &include : qAsConst(includes))
        fprintf(output, "\n#include <%s>", qPrintable(include));

    fprintf(output, "\n\n");

    QString moduleAsSymbol = module;
    moduleAsSymbol.replace(QLatin1Char('.'), QLatin1Char('_'));

    const QString functionName = QStringLiteral("qml_register_types_") + moduleAsSymbol;

    fprintf(output, "void %s()\n{", qPrintable(functionName));
    const auto majorVersion = parser.value(majorVersionOption);

    for (const QJsonObject &classDef : qAsConst(types)) {
        if (!classDef.value(QLatin1String("registerable")).toBool())
            continue;

        const QString className = classDef[QLatin1String("qualifiedClassName")].toString();

        if (classDef.value(QLatin1String("namespace")).toBool()) {
            fprintf(output, "\n    qmlRegisterNamespaceAndRevisions(&%s::staticMetaObject, \"%s\", %s);",
                    qPrintable(className), qPrintable(module), qPrintable(majorVersion));
        } else {
            fprintf(output, "\n    qmlRegisterTypesAndRevisions<%s>(\"%s\", %s);",
                    qPrintable(className), qPrintable(module), qPrintable(majorVersion));
        }
    }

    fprintf(output, "\n    qmlRegisterModule(\"%s\", %s, %s);",
            qPrintable(module), qPrintable(majorVersion),
            qPrintable(parser.value(minorVersionOption)));
    fprintf(output, "\n}\n");
    fprintf(output, "\nstatic const QQmlModuleRegistration registration(\"%s\", %s, %s);\n",
            qPrintable(module), qPrintable(majorVersion), qPrintable(functionName));

    if (!parser.isSet(pluginTypesOption))
        return EXIT_SUCCESS;

    if (parser.isSet(foreignTypesOption)) {
        const QStringList foreignTypesFiles = parser.value(foreignTypesOption)
                .split(QLatin1Char(','));
        for (const QString &types : foreignTypesFiles) {
            QFile typesFile(types);
            if (!typesFile.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "Cannot open foreign types file %s\n", qPrintable(types));
                continue;
            }

            QJsonParseError error = {0, QJsonParseError::NoError};
            QJsonDocument foreignMetaObjects = QJsonDocument::fromJson(typesFile.readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                fprintf(stderr, "Error parsing %s\n", qPrintable(types));
                continue;
            }

            const QJsonArray foreignObjectsArray = foreignMetaObjects.array();
            for (const auto &metaObject : foreignObjectsArray) {
                if (!metaObject.isObject()) {
                    fprintf(stderr, "Error parsing %s: JSON is not an object\n",
                            qPrintable(types));
                    continue;
                }

                const QString include = metaObject[QLatin1String("inputFile")].toString();
                const QJsonArray classes = metaObject[QLatin1String("classes")].toArray();
                for (const auto &cls : classes) {
                    QJsonObject classDef = cls.toObject();
                    classDef.insert(QLatin1String("inputFile"), include);
                    foreignTypes.append(classDef);
                }
            }
        }
    }

    sortTypes(foreignTypes);
    types += foreignRelatedTypes(types, foreignTypes);
    sortTypes(types);

    QmlTypesCreator creator;
    creator.setOwnTypes(std::move(types));
    creator.setForeignTypes(std::move(foreignTypes));
    creator.setModule(module);
    creator.setMajorVersion(parser.value(majorVersionOption).toInt());

    creator.generate(parser.value(pluginTypesOption), parser.value(dependenciesOption));
    return EXIT_SUCCESS;
}
