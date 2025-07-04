// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QFile>
#include <QCborArray>
#include <QCborValue>

#include "qqmltyperegistrar_p.h"
#include "qqmltypescreator_p.h"
#include "qanystringviewutils_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qqmltyperegistrarutils_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE
using namespace Qt::Literals;
using namespace Constants;
using namespace Constants::MetatypesDotJson;
using namespace Constants::MetatypesDotJson::Qml;
using namespace QAnyStringViewUtils;

struct ExclusiveVersionRange
{
    QAnyStringView fileName;
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
                warning(optionsFile) << "The @ option requires an input file";
                return false;
            }
            QFile f(optionsFile);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                warning(optionsFile) << "Cannot open options file specified with @";
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

int QmlTypeRegistrar::runExtract(
        const QString &baseName, const QString &nameSpace, const MetaTypesJsonProcessor &processor)
{
    if (processor.types().isEmpty()) {
        error(baseName) << "No types to register found in library";
        return EXIT_FAILURE;
    }
    QFile headerFile(baseName + u".h");
    bool ok = headerFile.open(QFile::WriteOnly);
    if (!ok) {
        error(headerFile.fileName()) << "Cannot open header file for writing";
        return EXIT_FAILURE;
    }

    QString includeGuard = baseName;
    static const QRegularExpression nonAlNum(QLatin1String("[^a-zA-Z0-9_]"));
    includeGuard.replace(nonAlNum, QLatin1String("_"));

    auto prefix = QString::fromLatin1(
            "#ifndef %1_H\n"
            "#define %1_H\n"
            "#include <QtQml/qqml.h>\n"
            "#include <QtQml/qqmlmoduleregistration.h>\n").arg(includeGuard);
    auto postfix = QString::fromLatin1("\n#endif // %1_H\n").arg(includeGuard);

    const QList<QString> includes = processor.includes();
    for (const QString &include: includes)
        prefix += u"\n#include <%1>"_s.arg(include);
    if (!nameSpace.isEmpty()) {
        prefix += u"\nnamespace %1 {"_s.arg(nameSpace);
        postfix.prepend(u"\n} // namespace %1"_s.arg(nameSpace));
    }

    headerFile.write((prefix + processor.extractRegisteredTypes() + postfix).toUtf8());

    QFile sourceFile(baseName + u".cpp");
    ok = sourceFile.open(QFile::WriteOnly);
    if (!ok) {
        error(sourceFile.fileName()) << "Cannot open implementation file for writing";
        return EXIT_FAILURE;
    }
    // the string split is necessaury because cmake's automoc scanner would otherwise pick up the include
    QString code = u"#include \"%1.h\"\n#include "_s.arg(baseName);
    code += uR"("moc_%1.cpp")"_s.arg(baseName);
    sourceFile.write(code.toUtf8());
    sourceFile.write("\n");
    return EXIT_SUCCESS;
}

MetaType QmlTypeRegistrar::findType(QAnyStringView name) const
{
    for (const MetaType &type : m_types) {
        if (type.qualifiedClassName() != name)
            continue;
        return type;
    }
    return MetaType();
};

MetaType QmlTypeRegistrar::findTypeForeign(QAnyStringView name) const
{
    for (const MetaType &type : m_foreignTypes) {
        if (type.qualifiedClassName() != name)
            continue;
        return type;
    }
    return MetaType();
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

// Return a name for the registration variable containing the module to
// avoid clashes in Unity builds.
static QString registrationVarName(const QString &module)
{
    auto specialCharPred = [](QChar c) { return !c.isLetterOrNumber(); };
    QString result = module;
    result[0] = result.at(0).toLower();
    result.erase(std::remove_if(result.begin(), result.end(), specialCharPred), result.end());
    return result + "Registration"_L1;
}

void QmlTypeRegistrar::write(QTextStream &output, QAnyStringView outFileName) const
{
    output << uR"(/****************************************************************************
** Generated QML type registration code
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

)"_s;

    output << u"#include <QtQml/qqml.h>\n"_s;
    output << u"#include <QtQml/qqmlmoduleregistration.h>\n"_s;

    for (const QString &include : m_includes) {
        output << u"\n#if __has_include(<%1>)"_s.arg(include);
        output << u"\n#  include <%1>"_s.arg(include);
        output << u"\n#endif"_s;
    }

    output << u"\n\n"_s;

    // Keep this in sync with _qt_internal_get_escaped_uri in CMake
    QString moduleAsSymbol = m_module;
    static const QRegularExpression nonAlnumRegexp(QLatin1String("[^A-Za-z0-9]"));
    moduleAsSymbol.replace(nonAlnumRegexp, QStringLiteral("_"));

    QString underscoredModuleAsSymbol = m_module;
    underscoredModuleAsSymbol.replace(QLatin1Char('.'), QLatin1Char('_'));

    if (underscoredModuleAsSymbol != moduleAsSymbol
            || underscoredModuleAsSymbol.isEmpty()
            || underscoredModuleAsSymbol.front().isDigit()) {
        warning(outFileName) << m_module << "is an invalid QML module URI. You cannot import this.";
    }

    const QString functionName = QStringLiteral("qml_register_types_") + moduleAsSymbol;
    output << uR"(
#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
)"_s;

    if (!m_targetNamespace.isEmpty())
        output << u"namespace "_s << m_targetNamespace << u" {\n"_s;

    output << u"Q_QMLTYPE_EXPORT void "_s << functionName << u"()\n{"_s;
    const quint8 majorVersion = m_moduleVersion.majorVersion();
    const quint8 minorVersion = m_moduleVersion.minorVersion();

    for (const auto &version : m_pastMajorVersions) {
        output << uR"(
    qmlRegisterModule("%1", %2, 0);
    qmlRegisterModule("%1", %2, 254);)"_s.arg(m_module)
                          .arg(version);
    }

    if (minorVersion != 0) {
        output << uR"(
    qmlRegisterModule("%1", %2, 0);)"_s.arg(m_module)
                          .arg(majorVersion);
    }

    output << uR"(
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED)"_s;

    QVector<QAnyStringView> typesRegisteredAnonymously;

    const auto fillTypesRegisteredAnonymously = [&](const auto &members, QAnyStringView typeName) {
        bool foundRevisionEntry = false;
        for (const auto &entry : members) {
            if (entry.revision.isValid()) {
                foundRevisionEntry = true;
                break;
            }
        }

        if (!foundRevisionEntry)
            return false;

        if (typesRegisteredAnonymously.contains(typeName))
            return true;

        typesRegisteredAnonymously.append(typeName);

        if (m_followForeignVersioning) {
            output << uR"(
    qmlRegisterAnonymousTypesAndRevisions<%1>("%2", %3);)"_s.arg(typeName.toString(), m_module)
                              .arg(majorVersion);
            return true;
        }

        for (const auto &version
                : m_pastMajorVersions + decltype(m_pastMajorVersions){ majorVersion }) {
            output << uR"(
    qmlRegisterAnonymousType<%1, 254>("%2", %3);)"_s.arg(typeName.toString(), m_module)
                              .arg(version);
        }

        return true;
    };


    QHash<QString, QList<ExclusiveVersionRange>> qmlElementInfos;

    for (const MetaType &classDef : std::as_const(m_types)) {

        // Do not generate C++ registrations for JavaScript types.
        if (classDef.inputFile().isEmpty())
            continue;

        QString className = classDef.qualifiedClassName().toString();
        QString targetName = className;

        // If either the foreign or the local part is a namespace we need to
        // generate a namespace registration.
        bool targetIsNamespace = classDef.kind() == MetaType::Kind::Namespace;

        QAnyStringView extendedName;
        QList<QString> qmlElementNames;
        QTypeRevision addedIn;
        QTypeRevision removedIn;

        for (const ClassInfo &v : classDef.classInfos()) {
            const QAnyStringView name = v.name;
            if (name == S_ELEMENT) {
                qmlElementNames.append(v.value.toString());
            } else if (name == S_FOREIGN) {
                targetName = v.value.toString();
            } else if (name == S_FOREIGN_IS_NAMESPACE) {
                targetIsNamespace = targetIsNamespace || (v.value == S_TRUE);
            } else if (name == S_EXTENDED) {
                extendedName = v.value;
            } else if (name == S_ADDED_IN_VERSION) {
                int version = toInt(v.value);
                addedIn = QTypeRevision::fromEncodedVersion(version);
                addedIn = handleInMinorVersion(addedIn, majorVersion);
            } else if (name == S_REMOVED_IN_VERSION) {
                int version = toInt(v.value);
                removedIn = QTypeRevision::fromEncodedVersion(version);
                removedIn = handleInMinorVersion(removedIn, majorVersion);
            }
        }

        for (QString qmlElementName : std::as_const(qmlElementNames)) {
            if (qmlElementName == S_ANONYMOUS)
                continue;
            if (qmlElementName == S_AUTO)
                qmlElementName = className;
            qmlElementInfos[qmlElementName].append({
                classDef.inputFile(),
                className,
                addedIn,
                removedIn
            });
        }

        // We want all related metatypes to be registered by name, so that we can look them up
        // without including the C++ headers. That's the reason for the QMetaType(foo).id() calls.

        const QList<QAnyStringView> namespaces
                = MetaTypesJsonProcessor::namespaces(classDef);

        const FoundType target = QmlTypesClassDescription::findType(
                m_types, m_foreignTypes, targetName, namespaces);

        // The target may be in a namespace we've resolved already.
        // Update the targetName accordingly.
        if (!target.native.isEmpty())
            targetName = target.native.qualifiedClassName().toString();

        if (targetIsNamespace) {
            // We need to figure out if the _target_ is a namespace. If not, it already has a
            // QMetaType and we don't need to generate one.

            QString targetTypeName = targetName;

            if (!target.javaScript.isEmpty() && target.native.isEmpty())
                warning(target.javaScript) << "JavaScript type cannot be used as namespace";

            if (target.native.kind() == MetaType::Kind::Object)
                targetTypeName += " *"_L1;

            // If there is no foreign type, the local one is a namespace.
            // Otherwise, only do metaTypeForNamespace if the target _metaobject_ is a namespace.
            // Not if we merely consider it to be a namespace for QML purposes.
            if (className == targetName || target.native.kind() == MetaType::Kind::Namespace) {
                output << uR"(
    {
        Q_CONSTINIT static auto metaType = QQmlPrivate::metaTypeForNamespace(
            [](const QtPrivate::QMetaTypeInterface *) {return &%1::staticMetaObject;},
            "%2");
        QMetaType(&metaType).id();
    })"_s.arg(targetName, targetTypeName);
            } else {
                Q_ASSERT(!targetTypeName.isEmpty());
                output << u"\n    QMetaType::fromType<%1>().id();"_s.arg(targetTypeName);
            }

            auto metaObjectPointer = [](QAnyStringView name) -> QString {
                QString result;
                const QLatin1StringView staticMetaObject = "::staticMetaObject"_L1;
                result.reserve(1 + name.length() + staticMetaObject.length());
                result.append('&'_L1);
                name.visit([&](auto view) { result.append(view); });
                result.append(staticMetaObject);
                return result;
            };

            if (!qmlElementNames.isEmpty()) {
                output << uR"(
    qmlRegisterNamespaceAndRevisions(%1, "%2", %3, nullptr, %4, %5);)"_s
                                  .arg(metaObjectPointer(targetName), m_module)
                                  .arg(majorVersion)
                                  .arg(metaObjectPointer(className),
                                       extendedName.isEmpty() ? QStringLiteral("nullptr")
                                                              : metaObjectPointer(extendedName));
            }
        } else {
            if (!qmlElementNames.isEmpty()) {
                auto checkRevisions = [&](const auto &array, QLatin1StringView type) {
                    for (auto it = array.begin(); it != array.end(); ++it) {
                        if (!it->revision.isValid())
                            continue;

                        QTypeRevision revision = it->revision;
                        if (m_moduleVersion < revision) {
                            warning(classDef)
                                    << className << "is trying to register" << type
                                    << it->name
                                    << "with future version" << revision
                                    << "when module version is only" << m_moduleVersion;
                        }
                    }
                };

                const Method::Container methods = classDef.methods();
                const Property::Container properties = classDef.properties();

                if (m_moduleVersion.isValid()) {
                    checkRevisions(properties, S_PROPERTY);
                    checkRevisions(methods, S_METHOD);
                }

                output << uR"(
    qmlRegisterTypesAndRevisions<%1>("%2", %3);)"_s.arg(className, m_module).arg(majorVersion);

                const BaseType::Container superClasses = classDef.superClasses();

                for (const BaseType &object : classDef.superClasses()) {
                        if (object.access != Access::Public)
                            continue;

                        QAnyStringView superClassName = object.name;

                        QVector<QAnyStringView> classesToCheck;

                        auto checkForRevisions = [&](QAnyStringView typeName) -> void {
                            auto typeAsMap = findType(typeName);

                            if (typeAsMap.isEmpty()) {
                                typeAsMap = findTypeForeign(typeName);
                                if (typeAsMap.isEmpty())
                                    return;

                                if (!fillTypesRegisteredAnonymously(
                                            typeAsMap.properties(), typeName)) {
                                    if (!fillTypesRegisteredAnonymously(
                                                typeAsMap.sigs(), typeName)) {
                                        fillTypesRegisteredAnonymously(
                                                typeAsMap.methods(), typeName);
                                    }
                                }
                            }

                            for (const BaseType &object : typeAsMap.superClasses()) {
                                if (object.access == Access::Public)
                                    classesToCheck << object.name;
                            }
                        };

                        checkForRevisions(superClassName);

                        while (!classesToCheck.isEmpty())
                            checkForRevisions(classesToCheck.takeFirst());
                    }
            } else {
                Q_ASSERT(!className.isEmpty());

                // Since we don't have a QML name for this type, it can't refer to another type.
                Q_ASSERT(className == targetName);

                output << uR"(
    QMetaType::fromType<%1%2>().id();)"_s.arg(
                        className, classDef.kind() == MetaType::Kind::Object ? u" *" : u"");
            }
        }

        const auto enums = target.native.enums();
        for (const auto &enumerator : enums) {
            output << uR"(
    qmlRegisterEnum<%1::%2>("%1::%2");)"_s.arg(
                    targetName, enumerator.name.toString());
            if (!enumerator.alias.isEmpty()) {
                output << uR"(
    qmlRegisterEnum<%1::%2>("%1::%2");)"_s.arg(
                        targetName, enumerator.alias.toString());
            }
        }
    }

    for (const auto [qmlName, exportsForSameQmlName] : qmlElementInfos.asKeyValueRange()) {
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
            warning(conflictingExportStartIt->fileName)
                    << qmlName << "is registered multiple times by the following C++ classes:"
                    << registeringCppClasses;
            conflictingExportStartIt = conflictingExportEndIt;
        }
    }

    output << uR"(
    QT_WARNING_POP
    qmlRegisterModule("%1", %2, %3);
}

static const QQmlModuleRegistration %5("%1", %4);
)"_s.arg(m_module)
                      .arg(majorVersion)
                      .arg(minorVersion)
                      .arg(functionName, registrationVarName(m_module));

    if (!m_targetNamespace.isEmpty())
        output << u"} // namespace %1\n"_s.arg(m_targetNamespace);
}

bool QmlTypeRegistrar::generatePluginTypes(const QString &pluginTypesFile, bool generatingJSRoot)
{
    QmlTypesCreator creator;
    creator.setOwnTypes(m_types);
    creator.setForeignTypes(m_foreignTypes);
    creator.setReferencedTypes(m_referencedTypes);
    creator.setUsingDeclarations(m_usingDeclarations);
    creator.setModule(m_module.toUtf8());
    creator.setVersion(QTypeRevision::fromVersion(m_moduleVersion.majorVersion(), 0));
    creator.setGeneratingJSRoot(generatingJSRoot);

    return creator.generate(pluginTypesFile);
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
void QmlTypeRegistrar::setTypes(
        const QVector<MetaType> &types, const QVector<MetaType> &foreignTypes)
{
    m_types = types;
    m_foreignTypes = foreignTypes;
}
void QmlTypeRegistrar::setReferencedTypes(const QList<QAnyStringView> &referencedTypes)
{
    m_referencedTypes = referencedTypes;
}

void QmlTypeRegistrar::setUsingDeclarations(const QList<UsingDeclaration> &usingDeclarations)
{
    m_usingDeclarations = usingDeclarations;
}

QT_END_NAMESPACE
