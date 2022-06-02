// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QFile>
#include <QJsonArray>
#include <QJsonValue>

#include "qqmltyperegistrar_p.h"
#include "qqmltypescreator_p.h"

QT_BEGIN_NAMESPACE
using namespace Qt::Literals;

struct ExclusiveVersionRange
{
    QString claimerName;
    QTypeRevision addedIn;
    QTypeRevision removedIn;
};

/*!
 * \brief True if x was removed before y was introduced.
 * \param o
 * \return
 */
bool operator<(const ExclusiveVersionRange &x, const ExclusiveVersionRange &y)
{
    if (x.removedIn.isValid())
        return y.addedIn.isValid() ? x.removedIn <= y.addedIn : true;
    else
        return false;
}

/*!
 * \brief True when x and y share a common version. (Warning: not transitive!)
 * \param o
 * \return
 */
bool operator==(const ExclusiveVersionRange &x, const ExclusiveVersionRange &y)
{
    return !(x < y) && !(y < x);
}

bool QmlTypeRegistrar::argumentsFromCommandLineAndFile(QStringList &allArguments,
                                                       const QStringList &arguments)
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

int QmlTypeRegistrar::runExtract(const QString &baseName, const MetaTypesJsonProcessor &processor)
{
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

QJsonValue QmlTypeRegistrar::findType(const QString &name) const
{
    for (const QJsonObject &type : m_types) {
        if (type[QLatin1String("qualifiedClassName")] != name)
            continue;
        return type;
    }
    return QJsonValue();
};

QJsonValue QmlTypeRegistrar::findTypeForeign(const QString &name) const
{
    for (const QJsonObject &type : m_foreignTypes) {
        if (type[QLatin1String("qualifiedClassName")] != name)
            continue;
        return type;
    }
    return QJsonValue();
};

QString conflictingVersionToString(const ExclusiveVersionRange &r)
{
    using namespace Qt::StringLiterals;

    QString s = r.claimerName;
    if (r.addedIn.isValid()) {
        s += u" (added in %1.%2)"_s.arg(r.addedIn.majorVersion()).arg(r.addedIn.minorVersion());
    }
    if (r.removedIn.isValid()) {
        s += u" (removed in %1.%2)"_s.arg(r.removedIn.majorVersion())
                     .arg(r.removedIn.minorVersion());
    }
    return s;
};

void QmlTypeRegistrar::write(FILE *output)
{
    fprintf(output,
            "/****************************************************************************\n"
            "** Generated QML type registration code\n**\n");
    fprintf(output,
            "** WARNING! All changes made in this file will be lost!\n"
            "*****************************************************************************/\n\n");
    fprintf(output,
            "#include <QtQml/qqml.h>\n"
            "#include <QtQml/qqmlmoduleregistration.h>\n");

    for (const QString &include : m_includes)
        fprintf(output, "\n#include <%s>", qPrintable(include));

    fprintf(output, "\n\n");

    // Keep this in sync with _qt_internal_get_escaped_uri in CMake
    QString moduleAsSymbol = m_module;
    moduleAsSymbol.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9]")), QStringLiteral("_"));

    QString underscoredModuleAsSymbol = m_module;
    underscoredModuleAsSymbol.replace(QLatin1Char('.'), QLatin1Char('_'));

    if (underscoredModuleAsSymbol != moduleAsSymbol
            || underscoredModuleAsSymbol.isEmpty()
            || underscoredModuleAsSymbol.front().isDigit()) {
        qWarning() << m_module << "is an invalid QML module URI. You cannot import this.";
    }

    const QString functionName = QStringLiteral("qml_register_types_") + moduleAsSymbol;
    fprintf(output,
            "#if !defined(QT_STATIC)\n"
            "#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT\n"
            "#else\n"
            "#define Q_QMLTYPE_EXPORT\n"
            "#endif\n"
            "\n");

    if (!m_targetNamespace.isEmpty())
        fprintf(output, "namespace %s {\n", qPrintable(m_targetNamespace));

    fprintf(output, "Q_QMLTYPE_EXPORT void %s()\n{", qPrintable(functionName));
    const quint8 majorVersion = m_moduleVersion.majorVersion();
    const quint8 minorVersion = m_moduleVersion.minorVersion();

    for (const auto &version : m_pastMajorVersions) {
        fprintf(output,
                "\n    qmlRegisterModule(\"%s\", %u, 0);\n    qmlRegisterModule(\"%s\", %u, 254);",
                qPrintable(m_module), (version), qPrintable(m_module), (version));
    }

    if (minorVersion != 0) {
        fprintf(output, "\n    qmlRegisterModule(\"%s\", %u, 0);", qPrintable(m_module),
                majorVersion);
    }

    QVector<QString> typesRegisteredAnonymously;
    QHash<QString, QList<ExclusiveVersionRange>> qmlElementInfos;

    for (const QJsonObject &classDef : m_types) {
        const QString className = classDef[QLatin1String("qualifiedClassName")].toString();

        QString targetName = className;
        QString extendedName;
        bool seenQmlElement = false;
        QString qmlElementName;
        QTypeRevision addedIn;
        QTypeRevision removedIn;

        const QJsonArray classInfos = classDef.value(QLatin1String("classInfos")).toArray();
        for (const QJsonValueConstRef v : classInfos) {
            const QString name = v[QStringLiteral("name")].toString();
            if (name == QStringLiteral("QML.Element")) {
                seenQmlElement = true;
                qmlElementName = v[QStringLiteral("value")].toString();
            } else if (name == QStringLiteral("QML.Foreign"))
                targetName = v[QLatin1String("value")].toString();
            else if (name == QStringLiteral("QML.Extended"))
                extendedName = v[QStringLiteral("value")].toString();
            else if (name == QStringLiteral("QML.AddedInVersion")) {
                int version = v[QStringLiteral("value")].toString().toInt();
                addedIn = QTypeRevision::fromEncodedVersion(version);
            } else if (name == QStringLiteral("QML.RemovedInVersion")) {
                int version = v[QStringLiteral("value")].toString().toInt();
                removedIn = QTypeRevision::fromEncodedVersion(version);
            }
        }

        if (seenQmlElement && qmlElementName != u"anonymous") {
            if (qmlElementName == u"auto")
                qmlElementName = className;
            qmlElementInfos[qmlElementName].append({ className, addedIn, removedIn });
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

                const QJsonObject *target = QmlTypesClassDescription::findType(m_types, targetName);
                if (!target)
                    target = QmlTypesClassDescription::findType(m_foreignTypes, targetName);

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
                fprintf(output, "\n        Q_CONSTINIT static auto metaType "
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
                fprintf(output,
                        "\n    qmlRegisterNamespaceAndRevisions(%s, "
                        "\"%s\", %u, nullptr, %s, %s);",
                        qPrintable(metaObjectPointer(targetName)), qPrintable(m_module),
                        (majorVersion), qPrintable(metaObjectPointer(className)),
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
                        if (m_moduleVersion < revision) {
                            qWarning().noquote()
                                    << "Warning:" << className << "is trying to register" << type
                                    << object[QStringLiteral("name")].toString()
                                    << "with future version" << revision
                                    << "when module version is only" << m_moduleVersion;
                        }
                    }
                };

                const QJsonArray methods = classDef[QLatin1String("methods")].toArray();
                const QJsonArray properties = classDef[QLatin1String("properties")].toArray();

                if (m_moduleVersion.isValid()) {
                    checkRevisions(properties, QLatin1String("property"));
                    checkRevisions(methods, QLatin1String("method"));
                }

                fprintf(output, "\n    qmlRegisterTypesAndRevisions<%s>(\"%s\", %u);",
                        qPrintable(className), qPrintable(m_module), (majorVersion));

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

                                        if (m_followForeignVersioning) {
                                            fprintf(output,
                                                    "\n    "
                                                    "qmlRegisterAnonymousTypesAndRevisions<%s>(\"%"
                                                    "s\", "
                                                    "%u);",
                                                    qPrintable(typeName), qPrintable(m_module),
                                                    (majorVersion));
                                            break;
                                        }

                                        for (const auto &version : m_pastMajorVersions
                                                     + decltype(m_pastMajorVersions){
                                                             majorVersion }) {
                                            fprintf(output,
                                                    "\n    "
                                                    "qmlRegisterAnonymousType<%s, 254>(\"%s\", "
                                                    "%u);",
                                                    qPrintable(typeName), qPrintable(m_module),
                                                    (version));
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

    for (const auto &[qmlName, exportsForSameQmlName] : qmlElementInfos.asKeyValueRange()) {
        // needs a least two cpp classes exporting the same qml element to potentially have a
        // conflict
        if (exportsForSameQmlName.size() < 2)
            continue;

        // sort exports by versions to find conflicting exports
        std::sort(exportsForSameQmlName.begin(), exportsForSameQmlName.end());
        auto conflictingExportStartIt = exportsForSameQmlName.cbegin();
        while (1) {
            // conflicting versions evaluate to true under operator==
            conflictingExportStartIt =
                    std::adjacent_find(conflictingExportStartIt, exportsForSameQmlName.cend());
            if (conflictingExportStartIt == exportsForSameQmlName.cend())
                break;

            auto conflictingExportEndIt = std::find_if_not(
                    conflictingExportStartIt, exportsForSameQmlName.cend(),
                    [=](const auto &x) -> bool { return x == *conflictingExportStartIt; });
            QString registeringCppClasses = conflictingExportStartIt->claimerName;
            std::for_each(std::next(conflictingExportStartIt), conflictingExportEndIt,
                          [&](const auto &q) {
                              registeringCppClasses += u", %1"_s.arg(conflictingVersionToString(q));
                          });
            qWarning().noquote() << "Warning:" << qmlName
                                 << "was registered multiple times by following Cpp classes: "
                                 << registeringCppClasses;
            conflictingExportStartIt = conflictingExportEndIt;
        }
    }
    fprintf(output, "\n    qmlRegisterModule(\"%s\", %u, %u);", qPrintable(m_module),
            (majorVersion), (minorVersion));
    fprintf(output, "\n}\n");
    fprintf(output, "\nstatic const QQmlModuleRegistration registration(\"%s\", %s);\n",
            qPrintable(m_module), qPrintable(functionName));

    if (!m_targetNamespace.isEmpty())
        fprintf(output, "} // namespace %s\n", qPrintable(m_targetNamespace));
}

void QmlTypeRegistrar::generatePluginTypes(const QString &pluginTypesFile)
{
    QmlTypesCreator creator;
    creator.setOwnTypes(m_types);
    creator.setForeignTypes(m_foreignTypes);
    creator.setReferencedTypes(m_referencedTypes);
    creator.setModule(m_module);
    creator.setVersion(m_moduleVersion);

    creator.generate(pluginTypesFile);
}

void QmlTypeRegistrar::setModuleNameAndNamespace(const QString &module,
                                                 const QString &targetNamespace)
{
    m_module = module;
    m_targetNamespace = targetNamespace;
}
void QmlTypeRegistrar::setModuleVersions(QTypeRevision moduleVersion,
                                         const QList<quint8> &pastMajorVersions,
                                         bool followForeignVersioning)
{
    m_moduleVersion = moduleVersion;
    m_pastMajorVersions = pastMajorVersions;
    m_followForeignVersioning = followForeignVersioning;
}
void QmlTypeRegistrar::setIncludes(const QList<QString> &includes)
{
    m_includes = includes;
}
void QmlTypeRegistrar::setTypes(const QVector<QJsonObject> &types,
                                const QVector<QJsonObject> &foreignTypes)
{
    m_types = types;
    m_foreignTypes = foreignTypes;
}
void QmlTypeRegistrar::setReferencedTypes(const QStringList &referencedTypes)
{
    m_referencedTypes = referencedTypes;
}

QT_END_NAMESPACE
