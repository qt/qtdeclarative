// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmetatypesjsonprocessor_p.h"

#include "qanystringviewutils_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qqmltyperegistrarutils_p.h"
#include "qqmltypesclassdescription_p.h"
#include "qqmltyperegistrarutils_p.h"

#include <QtCore/qcborarray.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qqueue.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace Constants;
using namespace Constants::MetatypesDotJson;
using namespace Constants::MetatypesDotJson::Qml;
using namespace QAnyStringViewUtils;

const MetaTypePrivate MetaType::s_empty;

// TODO: This could be optimized to store the objects in a more compact way.
std::vector<std::unique_ptr<MetaTypePrivate>> s_pool;

static QCborValue fromJson(const QByteArray &json, QJsonParseError *error)
{
    const QJsonDocument jsonValue = QJsonDocument::fromJson(json, error);
    if (jsonValue.isArray())
        return QCborValue::fromJsonValue(jsonValue.array());
    if (jsonValue.isObject())
        return QCborValue::fromJsonValue(jsonValue.object());
    return QCborValue();
}

QList<QAnyStringView> MetaTypesJsonProcessor::namespaces(const MetaType &classDef)
{
    const QAnyStringView unqualified = classDef.className();
    const QAnyStringView qualified = classDef.qualifiedClassName();

    QList<QAnyStringView> namespaces;
    if (qualified != unqualified) {
        namespaces = split(qualified, "::"_L1);
        Q_ASSERT(namespaces.last() == unqualified);
        namespaces.pop_back();
    }

    return namespaces;
}

bool MetaTypesJsonProcessor::processTypes(const QStringList &files)
{
    for (const QString &source: files) {
        QCborValue metaObjects;
        {
            QFile f(source);
            if (!f.open(QIODevice::ReadOnly)) {
                error(source) << "Cannot open file for reading";
                return false;
            }
            QJsonParseError parseError = {0, QJsonParseError::NoError};
            metaObjects = fromJson(f.readAll(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                error(source)
                        << "Failed to parse JSON:" << parseError.error
                        << parseError.errorString();
                return false;
            }
        }

        if (metaObjects.isArray()) {
            const QCborArray metaObjectsArray = metaObjects.toArray();
            for (const QCborValue &metaObject : metaObjectsArray) {
                if (!metaObject.isMap()) {
                    error(source) <<  "JSON is not an object";
                    return false;
                }

                processTypes(metaObject.toMap());
            }
        } else if (metaObjects.isMap()) {
            processTypes(metaObjects.toMap());
        } else {
            error(source) << "JSON is not an object or an array";
            return false;
        }
    }

    return true;
}

bool MetaTypesJsonProcessor::processForeignTypes(const QString &types)
{
    QFile typesFile(types);
    if (!typesFile.open(QIODevice::ReadOnly)) {
        error(types) << "Cannot open foreign types file";
        return false;
    }

    QJsonParseError parseError = {0, QJsonParseError::NoError};
    QCborValue foreignMetaObjects = fromJson(typesFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        error(types)
                << "Failed to parse JSON:" << parseError.error
                << parseError.errorString();
        return false;
    }

    const QCborArray foreignObjectsArray = foreignMetaObjects.toArray();
    for (const QCborValue &metaObject : foreignObjectsArray) {
        if (!metaObject.isMap()) {
            error(types) <<  "JSON is not an object";
            return false;
        }

        processForeignTypes(metaObject.toMap());
    }

    return true;
}

bool MetaTypesJsonProcessor::processForeignTypes(const QStringList &foreignTypesFiles)
{
    bool success = true;

    for (const QString &types : foreignTypesFiles) {
        if (!processForeignTypes(types))
            success = false;
    }
    return success;
}

template<typename String>
static void sortStringList(QList<String> *list)
{
    std::sort(list->begin(), list->end());
    const auto newEnd = std::unique(list->begin(), list->end());
    list->erase(typename QList<String>::const_iterator(newEnd), list->constEnd());
}

void MetaTypesJsonProcessor::postProcessTypes()
{
    sortTypes(m_types);
    sortStringList(&m_includes);
}

void MetaTypesJsonProcessor::postProcessForeignTypes()
{
    sortTypes(m_foreignTypes);
    addRelatedTypes();
    sortStringList(&m_referencedTypes);
}

QString MetaTypesJsonProcessor::extractRegisteredTypes() const
{
    QString registrationHelper;
    for (const auto &obj: m_types) {
        const QString className = obj.className().toString();
        const QString qualifiedClassName = obj.qualifiedClassName().toString();
        const QString foreignClassName = className + u"Foreign";
        QStringList qmlElements;
        QString qmlUncreatable;
        QString qmlAttached;
        bool isSingleton = false;
        bool isExplicitlyUncreatable = false;
        bool isNamespace = obj.kind() == MetaType::Kind::Namespace;
        for (const ClassInfo &entry: obj.classInfos()) {
            const auto name = entry.name;
            const auto value = entry.value;
            if (name == S_ELEMENT) {
                if (value == S_AUTO) {
                    qmlElements.append(u"QML_NAMED_ELEMENT("_s + className + u")"_s);
                } else if (value == S_ANONYMOUS) {
                    qmlElements.append(u"QML_ANONYMOUS"_s);
                } else {
                    qmlElements.append(u"QML_NAMED_ELEMENT("_s + value.toString() + u")");
                }
            } else if (name == S_CREATABLE && value == S_FALSE) {
                isExplicitlyUncreatable = true;
            } else if (name == S_UNCREATABLE_REASON) {
                qmlUncreatable = u"QML_UNCREATABLE(\""_s + value.toString() + u"\")";
            } else if (name == S_ATTACHED) {
                qmlAttached = u"QML_ATTACHED("_s + value.toString() + u")";
            } else if (name == S_SINGLETON) {
                isSingleton = true;
            }
        }
        if (qmlElements.isEmpty())
            continue; // no relevant entries found
        const QString spaces = u"    "_s;
        if (isNamespace) {
            registrationHelper += u"\nnamespace "_s + foreignClassName + u"{\n    Q_NAMESPACE\n"_s;
            registrationHelper += spaces + u"QML_FOREIGN_NAMESPACE(" + qualifiedClassName + u")\n"_s;
        } else {
            registrationHelper += u"\nstruct "_s + foreignClassName + u"{\n    Q_GADGET\n"_s;
            registrationHelper += spaces + u"QML_FOREIGN(" + qualifiedClassName + u")\n"_s;
        }
        registrationHelper += spaces + qmlElements.join(u"\n"_s) + u"\n"_s;
        if (isSingleton)
            registrationHelper += spaces + u"QML_SINGLETON\n"_s;
        if (isExplicitlyUncreatable) {
            if (qmlUncreatable.isEmpty())
                registrationHelper += spaces + uR"(QML_UNCREATABLE(""))" + u"n";
            else
                registrationHelper += spaces + qmlUncreatable + u"\n";
        }
        if (!qmlAttached.isEmpty())
            registrationHelper += spaces + qmlAttached + u"\n";
        registrationHelper += u"}";
        if (!isNamespace)
            registrationHelper += u";";
        registrationHelper += u"\n";
    }
    return registrationHelper;
}

MetaTypesJsonProcessor::RegistrationMode MetaTypesJsonProcessor::qmlTypeRegistrationMode(
        const MetaType &classDef)
{
    for (const ClassInfo &entry : classDef.classInfos()) {
        const QAnyStringView name = entry.name;
        if (name == S_ELEMENT) {
            switch (classDef.kind()) {
            case MetaType::Kind::Object:
                return ObjectRegistration;
            case MetaType::Kind::Gadget:
                return GadgetRegistration;
            case MetaType::Kind::Namespace:
                return NamespaceRegistration;
            default:
                warning(classDef)
                        << "Not registering a classInfo which is neither an object,"
                        << "nor a gadget, nor a namespace:"
                        << name.toString();
                break;
            }
            break;
        }
    }
    return NoRegistration;
}

// TODO: Remove this when QAnyStringView gets a proper qHash()
static size_t qHash(QAnyStringView string, size_t seed = 0)
{
    return string.visit([seed](auto view) {
        if constexpr (std::is_same_v<decltype(view), QStringView>)
            return qHash(view, seed);
        if constexpr (std::is_same_v<decltype(view), QLatin1StringView>)
            return qHash(view, seed);
        if constexpr (std::is_same_v<decltype(view), QUtf8StringView>)
            return qHash(QByteArrayView(view.data(), view.length()), seed);
    });
}

static bool qualifiedClassNameLessThan(const MetaType &a, const MetaType &b)
{
    return a.qualifiedClassName() < b.qualifiedClassName();
}

void MetaTypesJsonProcessor::addRelatedTypes()
{
    QSet<QAnyStringView> processedRelatedNativeNames;
    QSet<QAnyStringView> processedRelatedJavaScriptNames;
    QSet<QAnyStringView> unresolvedForeignNames;
    QQueue<MetaType> typeQueue;
    typeQueue.append(m_types);

    const auto addRelatedName
            = [&](QAnyStringView relatedName, const QList<QAnyStringView> &namespaces) {
        if (const FoundType related = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, relatedName, namespaces)) {

            if (!related.javaScript.isEmpty())
                processedRelatedJavaScriptNames.insert(related.javaScript.qualifiedClassName());

            if (!related.native.isEmpty())
                processedRelatedNativeNames.insert(related.native.qualifiedClassName());

            return true;
        } else {
            return false;
        }
    };

    const auto addRelatedType = [&](const MetaType &type) {
        const QAnyStringView qualifiedName = type.qualifiedClassName();
        if (type.inputFile().isEmpty())
            processedRelatedJavaScriptNames.insert(qualifiedName);
        else
            processedRelatedNativeNames.insert(qualifiedName);
    };

    // First mark all classes registered from this module as already processed.
    for (const MetaType &type : std::as_const(m_types)) {
        addRelatedType(type);
        for (const ClassInfo &obj : type.classInfos()) {
            if (obj.name == S_FOREIGN) {
                const QAnyStringView foreign = obj.value;
                if (!addRelatedName(foreign, namespaces(type)))
                    unresolvedForeignNames.insert(foreign);
                break;
            }
        }
    }

    // Then mark all classes registered from other modules as already processed.
    // We don't want to generate them again for this module.
    for (const MetaType &foreignType : std::as_const(m_foreignTypes)) {
        bool seenQmlPrefix = false;
        for (const ClassInfo &obj : foreignType.classInfos()) {
            const QAnyStringView name = obj.name;
            if (!seenQmlPrefix && startsWith(name, "QML."_L1)) {
                addRelatedType(foreignType);
                seenQmlPrefix = true;
            }
            if (name == S_FOREIGN) {
                const QAnyStringView foreign = obj.value;
                if (!addRelatedName(foreign, namespaces(foreignType)))
                    unresolvedForeignNames.insert(foreign);
                break;
            }
        }
    }

    const auto addReference = [&](const MetaType &type, QSet<QAnyStringView> *processedRelatedNames,
                                  FoundType::Origin origin) {
        if (type.isEmpty())
            return;
        QAnyStringView qualifiedName = type.qualifiedClassName();
        m_referencedTypes.append(qualifiedName);
        const qsizetype size = processedRelatedNames->size();
        processedRelatedNames->insert(qualifiedName);

        if (processedRelatedNames->size() == size)
            return;

        typeQueue.enqueue(type);

        if (origin == FoundType::OwnTypes)
            return;

        // Add to own types since we need it for our registrations.
        const auto insert = std::lower_bound(
                m_types.constBegin(), m_types.constEnd(), type,
                qualifiedClassNameLessThan);
        m_types.insert(insert, type);

        // Remove from the foreign types to avoid the ODR warning.
        const auto remove = std::equal_range(
                m_foreignTypes.constBegin(), m_foreignTypes.constEnd(), type,
                qualifiedClassNameLessThan);
        for (auto it = remove.first; it != remove.second; ++it) {
            if (*it == type) {
                m_foreignTypes.erase(it);
                break;
            }
        }
    };

    const auto addInterfaceOrSelfExtension
            = [&](QAnyStringView typeName, const QList<QAnyStringView> &namespaces) {
        if (const FoundType other = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, typeName, namespaces)) {
            if (!other.native.isEmpty()) {
                addReference(other.native, &processedRelatedNativeNames, other.nativeOrigin);
                return true;
            }
        } else {
            // Do not warn about unresolved interfaces.
            // They don't have to have Q_OBJECT or Q_GADGET.
            unresolvedForeignNames.insert(typeName);
        }

        processedRelatedNativeNames.insert(typeName);
        return false;
    };

    const auto addType = [&](const MetaType &context, QAnyStringView typeName,
                             const QList<QAnyStringView> &namespaces) {
        if (const FoundType other = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, typeName, namespaces)) {
            addReference(other.native, &processedRelatedNativeNames, other.nativeOrigin);
            addReference(other.javaScript, &processedRelatedJavaScriptNames, other.javaScriptOrigin);
            return true;
        }

        // If it's an enum, add the surrounding type.
        const QLatin1StringView separator("::");
        if (const qsizetype index = lastIndexOf(typeName, separator); index > 0) {
            if (const FoundType other = QmlTypesClassDescription::findType(
                        m_types, m_foreignTypes, typeName.left(index), namespaces)) {

                const QAnyStringView enumName = typeName.mid(index + separator.length());

                for (const Enum &enumerator : other.native.enums()) {
                    if (enumerator.name != enumName)
                        continue;

                    addReference(other.native, &processedRelatedNativeNames, other.nativeOrigin);
                    addReference(
                            other.javaScript, &processedRelatedJavaScriptNames,
                            other.javaScriptOrigin);
                    return true;
                }
            }
        }

        // If we've detected this type as unresolved foreign and it actually belongs to this module,
        // we'll get to it again when we process it as foreign type. In that case we'll look at the
        // special cases for sequences and extensions.
        if (!unresolvedForeignNames.contains(typeName))
            warning(context) << typeName << "is used but cannot be found.";

        processedRelatedNativeNames.insert(typeName);
        processedRelatedJavaScriptNames.insert(typeName);
        return false;
    };

    const auto addSupers = [&](const MetaType &context, const QList<QAnyStringView> &namespaces) {
        for (const Interface &iface : context.ifaces())
            addInterfaceOrSelfExtension(interfaceName(iface), namespaces);

        // We don't warn about missing bases for value types. They don't have to be registered.
        bool warnAboutSupers = context.kind() != MetaType::Kind::Gadget;

        QList<QAnyStringView> missingSupers;

        for (const BaseType &superObject : context.superClasses()) {
            if (superObject.access != Access::Public)
                continue;

            QAnyStringView typeName = superObject.name;
            if (const FoundType other = QmlTypesClassDescription::findType(
                        m_types, m_foreignTypes, typeName, namespaces)) {
                addReference(
                        other.native, &processedRelatedNativeNames, other.nativeOrigin);
                addReference(
                        other.javaScript, &processedRelatedJavaScriptNames, other.javaScriptOrigin);
                warnAboutSupers = false;
            } else {
                missingSupers.append(typeName);
            }
        }

        for (QAnyStringView typeName : std::as_const(missingSupers)) {
            // If we've found one valid base type, don't complain about the others.
            if (warnAboutSupers && !unresolvedForeignNames.contains(typeName))
                warning(context) << typeName << "is used but cannot be found.";

            processedRelatedNativeNames.insert(typeName);
            processedRelatedJavaScriptNames.insert(typeName);
        }
    };

    // Then recursively iterate the super types and attached types, marking the
    // ones we are interested in as related.
    while (!typeQueue.isEmpty()) {
        QAnyStringView unresolvedForeign;

        // We don't need to resolve the foreign part of sequence registrations.
        bool isSequence = false;
        // We don't need to resolve the foreign part of self-extending value types.
        bool isSelfExtendingValueType = false;
        // We don't want to deal with builtins that have JavaScript extensions. Consider them found.
        bool hasJavaScriptExtension = false;

        const MetaType classDef = typeQueue.dequeue();
        const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(classDef);

        for (const ClassInfo &obj : classDef.classInfos()) {
            const QAnyStringView objNameValue = obj.name;
            if (objNameValue == S_ATTACHED) {
                addType(classDef, obj.value, namespaces);
            } else if (objNameValue == S_SEQUENCE) {
                isSequence = true;
                QAnyStringView value = obj.value;

                if (!value.isEmpty() && value.back() == '*'_L1) {
                    // Pointers as sequence values include the '*'
                    QAnyStringView chopped = value.chopped(1);
                    while (!chopped.isEmpty() && chopped.back() == ' '_L1)
                        chopped = chopped.chopped(1);
                    addType(classDef, chopped, namespaces);
                } else {
                    addType(classDef, value, namespaces);
                }
            } else if (objNameValue == S_EXTENDED) {
                const QAnyStringView value = obj.value;
                if (value == classDef.qualifiedClassName()
                        && classDef.kind() == MetaType::Kind::Gadget) {
                    isSelfExtendingValueType = true;
                    addInterfaceOrSelfExtension(value, namespaces);
                } else {
                    addType(classDef, value, namespaces);
                }
            } else if (objNameValue == S_EXTENSION_IS_JAVA_SCRIPT) {
                hasJavaScriptExtension = true;
            } else if (objNameValue == S_FOREIGN) {
                const QAnyStringView foreignClassName = obj.value;

                // A type declared as QML_FOREIGN will usually be a foreign type, but it can
                // actually be an additional registration of a local type, too.
                const FoundType found = QmlTypesClassDescription::findType(
                        m_foreignTypes, {}, foreignClassName, namespaces);
                if (!found) {
                    if (!QmlTypesClassDescription::findType(
                                m_types, {}, foreignClassName, namespaces)) {
                        unresolvedForeign = foreignClassName;
                    }
                } else {
                    const MetaType other = found.select(classDef, "Foreign");
                    const QList<QAnyStringView> otherNamespaces
                            = MetaTypesJsonProcessor::namespaces(other);
                    addSupers(other, otherNamespaces);

                    for (const ClassInfo &obj : other.classInfos()) {
                        const QAnyStringView objNameValue = obj.name;
                        if (objNameValue == S_ATTACHED || objNameValue == S_SEQUENCE
                                || objNameValue == S_EXTENDED) {
                            addType(classDef, obj.value, otherNamespaces);
                            break;
                        }
                        // No, you cannot chain S_FOREIGN declarations. Sorry.
                    }
                }
            }
        }

        if (!isSequence && !isSelfExtendingValueType && !hasJavaScriptExtension
                && !unresolvedForeign.isEmpty()) {
            warning(classDef)
                    << unresolvedForeign
                    << "is declared as foreign type, but cannot be found.";
        }

        addSupers(classDef, namespaces);
    }
}

void MetaTypesJsonProcessor::sortTypes(QVector<MetaType> &types)
{
    std::sort(types.begin(), types.end(), qualifiedClassNameLessThan);
}

QString MetaTypesJsonProcessor::resolvedInclude(QAnyStringView include)
{
    return (m_privateIncludes && endsWith(include, "_p.h"_L1))
            ? QLatin1String("private/") + include.toString()
            : include.toString();
}

void MetaTypesJsonProcessor::processTypes(const QCborMap &types)
{
    const QString include = resolvedInclude(toStringView(types, S_INPUT_FILE));
    const QCborArray classes = types[S_CLASSES].toArray();
    for (const QCborValue &cls : classes) {
        const MetaType classDef(cls.toMap(), include);

        switch (qmlTypeRegistrationMode(classDef)) {
        case NamespaceRegistration:
        case GadgetRegistration:
        case ObjectRegistration: {
            if (!endsWith(include, QLatin1String(".h"))
                    && !endsWith(include, QLatin1String(".hpp"))
                    && !endsWith(include, QLatin1String(".hxx"))
                    && !endsWith(include, QLatin1String(".hh"))
                    && !endsWith(include, QLatin1String(".py"))
                    && contains(include, QLatin1Char('.'))) {
                warning(include)
                        << "Class" << classDef.qualifiedClassName()
                        << "is declared in" << include << "which appears not to be a header."
                        << "The compilation of its registration to QML may fail.";
            }
            m_includes.append(include);
            m_types.emplaceBack(classDef);
            break;
        }
        case NoRegistration:
            m_foreignTypes.emplaceBack(classDef);
            break;
        }
    }
}

void MetaTypesJsonProcessor::processForeignTypes(const QCborMap &types)
{
    const QString include = resolvedInclude(toStringView(types, S_INPUT_FILE));
    const QCborArray classes = types[S_CLASSES].toArray();
    for (const QCborValue &cls : classes)
        m_foreignTypes.emplaceBack(cls.toMap(), include);
}

static QTypeRevision getRevision(const QCborMap &cbor)
{
    const auto it = cbor.find(S_REVISION);
    return it == cbor.end()
            ? QTypeRevision()
            : QTypeRevision::fromEncodedVersion(it->toInteger());
}

static Access getAccess(const QCborMap &cbor)
{
    const QAnyStringView access = toStringView(cbor, S_ACCESS);
    if (access == S_PUBLIC)
        return Access::Public;
    if (access == S_PROTECTED)
        return Access::Protected;
    return Access::Private;
}

BaseType::BaseType(const QCborMap &cbor)
    : name(toStringView(cbor, S_NAME))
    , access(getAccess(cbor))
{
}

ClassInfo::ClassInfo(const QCborMap &cbor)
    : name(toStringView(cbor, S_NAME))
    , value(toStringView(cbor, S_VALUE))
{
}

Interface::Interface(const QCborValue &cbor)
{
    if (cbor.isArray()) {
        QCborArray needlessWrapping = cbor.toArray();
        className = needlessWrapping.size() > 0
                ? toStringView(needlessWrapping[0].toMap(), S_CLASS_NAME)
                : QAnyStringView();
    } else {
        className = toStringView(cbor.toMap(), S_CLASS_NAME);
    }
}

Property::Property(const QCborMap &cbor)
    : name(toStringView(cbor, S_NAME))
    , type(toStringView(cbor, S_TYPE))
    , member(toStringView(cbor, S_MEMBER))
    , read(toStringView(cbor, S_READ))
    , write(toStringView(cbor, S_WRITE))
    , reset(toStringView(cbor, S_RESET))
    , notify(toStringView(cbor, S_NOTIFY))
    , bindable(toStringView(cbor, S_BINDABLE))
    , privateClass(toStringView(cbor, S_PRIVATE_CLASS))
    , index(cbor[S_INDEX].toInteger(-1))
    , revision(getRevision(cbor))
    , isFinal(cbor[S_FINAL].toBool())
    , isConstant(cbor[S_CONSTANT].toBool())
    , isRequired(cbor[S_REQUIRED].toBool())
{
}

Argument::Argument(const QCborMap &cbor)
    : name(toStringView(cbor, S_NAME))
    , type(toStringView(cbor, S_TYPE))
{
}

Method::Method(const QCborMap &cbor, bool isConstructor)
    : name(toStringView(cbor, S_NAME))
    , returnType(toStringView(cbor, S_RETURN_TYPE))
    , revision(getRevision(cbor))
    , access(getAccess(cbor))
    , isCloned(cbor[S_IS_CLONED].toBool())
    , isJavaScriptFunction(cbor[S_IS_JAVASCRIPT_FUNCTION].toBool())
    , isConstructor(isConstructor || cbor[S_IS_CONSTRUCTOR].toBool())
{
    const QCborArray args = cbor[S_ARGUMENTS].toArray();
    for (const QCborValue &argument : args)
        arguments.emplace_back(argument.toMap());

    if (arguments.size() == 1) {
        const QAnyStringView type = arguments[0].type;
        if (type == "QQmlV4FunctionPtr"_L1 || type == "QQmlV4Function*"_L1)
            isJavaScriptFunction = true;
    }
}

Enum::Enum(const QCborMap &cbor)
    : name(toStringView(cbor, S_NAME))
    , alias(toStringView(cbor, S_ALIAS))
    , type(toStringView(cbor, S_TYPE))
    , isFlag(cbor[S_IS_FLAG].toBool())
    , isClass(cbor[S_IS_CLASS].toBool())
{
    const QCborArray vals = cbor[S_VALUES].toArray();
    for (const QCborValue &value : vals)
        values.emplace_back(toStringView(value));
}

MetaTypePrivate::MetaTypePrivate(const QCborMap &cbor, const QString &inputFile)
    : cbor(cbor)
    , inputFile(inputFile)
{
    className = toStringView(cbor, S_CLASS_NAME);
    qualifiedClassName = toStringView(cbor, S_QUALIFIED_CLASS_NAME);

    const QCborArray cborSuperClasses = cbor[S_SUPER_CLASSES].toArray();
    for (const QCborValue &superClass : cborSuperClasses)
        superClasses.emplace_back(superClass.toMap());

    const QCborArray cborClassInfos = cbor[S_CLASS_INFOS].toArray();
    for (const QCborValue &classInfo : cborClassInfos)
        classInfos.emplace_back(classInfo.toMap());

    const QCborArray cborIfaces = cbor[S_INTERFACES].toArray();
    for (const QCborValue &iface : cborIfaces)
        ifaces.emplace_back(iface);

    const QCborArray cborProperties = cbor[S_PROPERTIES].toArray();
    for (const QCborValue &property : cborProperties)
        properties.emplace_back(property.toMap());

    for (const QCborArray &cborMethods : { cbor[S_METHODS].toArray(), cbor[S_SLOTS].toArray() }) {
        for (const QCborValue &method : cborMethods)
            methods.emplace_back(method.toMap(), false);
    }

    const QCborArray cborSigs = cbor[S_SIGNALS].toArray();
    for (const QCborValue &sig : cborSigs)
        sigs.emplace_back(sig.toMap(), false);

    const QCborArray cborConstructors = cbor[S_CONSTRUCTORS].toArray();
    for (const QCborValue &constructor : cborConstructors)
        constructors.emplace_back(constructor.toMap(), true);

    const QCborArray cborEnums = cbor[S_ENUMS].toArray();
    for (const QCborValue &enumerator : cborEnums)
        enums.emplace_back(enumerator.toMap());

    if (cbor[S_GADGET].toBool())
        kind = Kind::Gadget;
    else if (cbor[S_OBJECT].toBool())
        kind = Kind::Object;
    else if (cbor[S_NAMESPACE].toBool())
        kind = Kind::Namespace;
}

MetaType::MetaType(const QCborMap &cbor, const QString &inputFile)
    : d(s_pool.emplace_back(std::make_unique<MetaTypePrivate>(cbor, inputFile)).get())
{}

QT_END_NAMESPACE
