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

int QmlTypeRegistrar::runExtract(const QString &baseName, const MetaTypesJsonProcessor &processor)
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
    const QList<QString> includes = processor.includes();
    for (const QString &include: includes)
        prefix += u"\n#include <%1>"_s.arg(include);
    headerFile.write((prefix + processor.extractRegisteredTypes()).toUtf8() + "\n#endif\n");

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

QCborValue QmlTypeRegistrar::findType(QAnyStringView name) const
{
    for (const QCborMap &type : m_types) {
        if (toStringView(type, S_QUALIFIED_CLASS_NAME) != name)
            continue;
        return type;
    }
    return QCborValue();
};

QCborValue QmlTypeRegistrar::findTypeForeign(QAnyStringView name) const
{
    for (const QCborMap &type : m_foreignTypes) {
        if (toStringView(type, S_QUALIFIED_CLASS_NAME) != name)
            continue;
        return type;
    }
    return QCborValue();
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

    for (const QString &include : m_includes)
        output << u"\n#include <%1>"_s.arg(include);

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

    QVector<QAnyStringView> typesRegisteredAnonymously;
    QHash<QString, QList<ExclusiveVersionRange>> qmlElementInfos;

    for (const QCborMap &classDef : std::as_const(m_types)) {

        // Do not generate C++ registrations for JavaScript types.
        if (toStringView(classDef, S_INPUT_FILE).isEmpty())
            continue;

        QString className = classDef[S_QUALIFIED_CLASS_NAME].toString();
        QString targetName = className;

        // If either the foreign or the local part is a namespace we need to
        // generate a namespace registration.
        bool targetIsNamespace = classDef.value(S_NAMESPACE).toBool();

        QAnyStringView extendedName;
        QList<QString> qmlElementNames;
        QTypeRevision addedIn;
        QTypeRevision removedIn;

        const QCborArray classInfos = classDef.value(S_CLASS_INFOS).toArray();
        for (const QCborValueConstRef info : classInfos) {
            const QCborMap v = info.toMap();
            const QAnyStringView name = toStringView(v, S_NAME);
            if (name == S_ELEMENT) {
                qmlElementNames.append(v[S_VALUE].toString());
            } else if (name == S_FOREIGN) {
                targetName = v[S_VALUE].toString();
            } else if (name == S_FOREIGN_IS_NAMESPACE) {
                targetIsNamespace = targetIsNamespace || (v[S_VALUE] == S_TRUE);
            } else if (name == S_EXTENDED) {
                extendedName = toStringView(v, S_VALUE);
            } else if (name == S_ADDED_IN_VERSION) {
                int version = toInt(toStringView(v, S_VALUE));
                addedIn = QTypeRevision::fromEncodedVersion(version);
                addedIn = handleInMinorVersion(addedIn, majorVersion);
            } else if (name == S_REMOVED_IN_VERSION) {
                int version = toInt(toStringView(v, S_VALUE));
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
                toStringView(classDef, S_INPUT_FILE),
                className,
                addedIn,
                removedIn
            });
        }

        // We want all related metatypes to be registered by name, so that we can look them up
        // without including the C++ headers. That's the reason for the QMetaType(foo).id() calls.

        if (targetIsNamespace) {
            // We need to figure out if the _target_ is a namespace. If not, it already has a
            // QMetaType and we don't need to generate one.

            QString targetTypeName = targetName;

            const QList<QAnyStringView> namespaces
                    = MetaTypesJsonProcessor::namespaces(classDef);

            const FoundType target = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, targetName, namespaces);

            if (!target.javaScript.isEmpty() && target.native.isEmpty())
                warning(target.javaScript) << "JavaScript type cannot be used as namespace";

            if (target.native.value(S_OBJECT).toBool())
                targetTypeName += " *"_L1;

            // If there is no foreign type, the local one is a namespace.
            // Otherwise, only do metaTypeForNamespace if the target _metaobject_ is a namespace.
            // Not if we merely consider it to be a namespace for QML purposes.
            if (className == targetName || target.native.value(S_NAMESPACE).toBool()) {
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
                auto checkRevisions = [&](const QCborArray &array, QLatin1StringView type) {
                    for (auto it = array.constBegin(); it != array.constEnd(); ++it) {
                        auto object = it->toMap();
                        if (!object.contains(S_REVISION))
                            continue;

                        QTypeRevision revision = QTypeRevision::fromEncodedVersion(
                                object[S_REVISION].toInteger());
                        if (m_moduleVersion < revision) {
                            warning(classDef)
                                    << className << "is trying to register" << type
                                    << toStringView(object, S_NAME)
                                    << "with future version" << revision
                                    << "when module version is only" << m_moduleVersion;
                        }
                    }
                };

                const QCborArray methods = classDef[S_METHODS].toArray();
                const QCborArray properties = classDef[S_PROPERTIES].toArray();

                if (m_moduleVersion.isValid()) {
                    checkRevisions(properties, S_PROPERTY);
                    checkRevisions(methods, S_METHOD);
                }

                output << uR"(
    qmlRegisterTypesAndRevisions<%1>("%2", %3);)"_s.arg(className, m_module).arg(majorVersion);

                const QCborValue superClasses = classDef[S_SUPER_CLASSES];

                if (superClasses.isArray()) {
                    for (const QCborValueRef entry : superClasses.toArray()) {
                        const QCborMap object = entry.toMap();
                        if (object[S_ACCESS] != S_PUBLIC)
                            continue;

                        QAnyStringView superClassName = toStringView(object, S_NAME);

                        QVector<QAnyStringView> classesToCheck;

                        auto checkForRevisions = [&](QAnyStringView typeName) -> void {
                            auto type = findType(typeName);

                            if (!type.isMap()) {
                                type = findTypeForeign(typeName);
                                if (!type.isMap())
                                    return;

                                const auto typeAsMap = type.toMap();
                                for (QLatin1StringView section : { S_PROPERTIES, S_SIGNALS, S_METHODS }) {
                                    bool foundRevisionEntry = false;
                                    for (const QCborValueRef entry : typeAsMap[section].toArray()) {
                                        if (entry.toMap().contains(S_REVISION)) {
                                            foundRevisionEntry = true;
                                            break;
                                        }
                                    }
                                    if (foundRevisionEntry) {
                                        if (typesRegisteredAnonymously.contains(typeName))
                                            break;

                                        typesRegisteredAnonymously.append(typeName);

                                        if (m_followForeignVersioning) {
                                            output << uR"(
    qmlRegisterAnonymousTypesAndRevisions<%1>("%2", %3);)"_s.arg(typeName.toString(), m_module)
                                                              .arg(majorVersion);
                                            break;
                                        }

                                        for (const auto &version : m_pastMajorVersions
                                                     + decltype(m_pastMajorVersions){
                                                             majorVersion }) {
                                            output << uR"(
    qmlRegisterAnonymousType<%1, 254>("%2", %3);)"_s.arg(typeName.toString(), m_module)
                                                              .arg(version);
                                        }
                                        break;
                                    }
                                }
                            }

                            const QCborValue superClasses = type.toMap()[S_SUPER_CLASSES];

                            if (superClasses.isArray()) {
                                for (const QCborValueRef entry : superClasses.toArray()) {
                                    const QCborMap object = entry.toMap();
                                    if (object[S_ACCESS] != S_PUBLIC)
                                        continue;
                                    classesToCheck << toStringView(object, S_NAME);
                                }
                            }
                        };

                        checkForRevisions(superClassName);

                        while (!classesToCheck.isEmpty())
                            checkForRevisions(classesToCheck.takeFirst());
                    }
                }
            } else {
                Q_ASSERT(!className.isEmpty());
                output << uR"(
    QMetaType::fromType<%1%2>().id();)"_s.arg(
                    className, classDef.value(S_OBJECT).toBool() ? u" *" : u"");
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

bool QmlTypeRegistrar::generatePluginTypes(const QString &pluginTypesFile)
{
    QmlTypesCreator creator;
    creator.setOwnTypes(m_types);
    creator.setForeignTypes(m_foreignTypes);
    creator.setReferencedTypes(m_referencedTypes);
    creator.setModule(m_module.toUtf8());
    creator.setVersion(QTypeRevision::fromVersion(m_moduleVersion.majorVersion(), 0));

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
void QmlTypeRegistrar::setTypes(const QVector<QCborMap> &types,
                                const QVector<QCborMap> &foreignTypes)
{
    m_types = types;
    m_foreignTypes = foreignTypes;
}
void QmlTypeRegistrar::setReferencedTypes(const QList<QAnyStringView> &referencedTypes)
{
    m_referencedTypes = referencedTypes;
}

QT_END_NAMESPACE
