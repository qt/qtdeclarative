// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltyperegistrarutils_p.h"
#include "qqmltypesclassdescription_p.h"

#include "qanystringviewutils_p.h"
#include "qmetatypesjsonprocessor_p.h"
#include "qqmltyperegistrarconstants_p.h"

#include <QtCore/qcborarray.h>
#include <QtCore/qcbormap.h>
QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace Constants;
using namespace Constants::MetatypesDotJson;
using namespace Constants::MetatypesDotJson::Qml;
using namespace QAnyStringViewUtils;

template<typename Container>
static void collectExtraVersions(const Container &items, QList<QTypeRevision> &extraVersions)
{
    for (const auto &obj : items) {
        if (obj.revision.isValid() && !extraVersions.contains(obj.revision))
            extraVersions.append(obj.revision);
    }
}

struct Compare {
    bool operator()(const QAnyStringView &typeName, const MetaType &type) const
    {
        return typeName < type.qualifiedClassName();
    }

    bool operator()(const MetaType &type, const QAnyStringView &typeName) const
    {
        return type.qualifiedClassName() < typeName;
    }
};

FoundType::FoundType(const MetaType &single, FoundType::Origin origin)
{
    if (single.inputFile().isEmpty()) {
        javaScript = single;
        javaScriptOrigin = origin;
    } else {
        native = single;
        nativeOrigin = origin;
    }
}

MetaType FoundType::select(const MetaType &category, QAnyStringView relation) const
{
    if (category.inputFile().isEmpty()) {
        if (javaScript.isEmpty()) {
            warning(category)
                    << relation << "type of" << category.qualifiedClassName()
                    << "is not a JavaScript type";
        }
        return javaScript;
    }

    if (native.isEmpty()) {
        warning(category)
                << relation << "of" << category.qualifiedClassName()
                << "is not a native type";
    }
    return native;
}

FoundType QmlTypesClassDescription::findType(
        const QVector<MetaType> &types, const QVector<MetaType> &foreign,
        const QAnyStringView &name, const QList<QAnyStringView> &namespaces)
{
    const auto tryFindType = [&](QAnyStringView qualifiedName) -> FoundType {
        FoundType result;
        for (const QVector<MetaType> &t : {types, foreign}) {
            const auto [first, last] = std::equal_range(
                    t.begin(), t.end(), qualifiedName, Compare());
            for (auto it = first; it != last; ++it) {
                Q_ASSERT(it->qualifiedClassName() == qualifiedName);

                if (it->inputFile().isEmpty()) {
                    if (result.javaScript.isEmpty()) {
                        result.javaScript = *it;
                        result.javaScriptOrigin = (&t == &types)
                                ? FoundType::OwnTypes
                                : FoundType::ForeignTypes;
                    } else {
                        warning(result.javaScript)
                                << "Multiple JavaScript types called" << qualifiedName << "found!";
                    }
                } else if (result.native.isEmpty()) {
                    result.native = *it;
                    result.nativeOrigin = (&t == &types)
                            ? FoundType::OwnTypes
                            : FoundType::ForeignTypes;
                } else {
                    warning(result.native)
                            << "Multiple C++ types called" << qualifiedName << "found!"
                            << "This violates the One Definition Rule!";
                }
            }
        }

        return result;
    };

    if (startsWith(name, QLatin1String("::")))
        return tryFindType(name.mid(2));

    QString qualified;
    for (int i = 0, end = namespaces.length(); i != end; ++i) {
        for (int j = 0; j < end - i; ++j) {
            namespaces[j].visit([&](auto data) { qualified.append(data); });
            qualified.append(QLatin1String("::"));
        }
        name.visit([&](auto data) { qualified.append(data); });
        if (const FoundType found = tryFindType(qualified))
            return found;

        qualified.truncate(0);
    }

    return tryFindType(name);
}

void QmlTypesClassDescription::collectSuperClasses(
        const MetaType &classDef, const QVector<MetaType> &types,
        const QVector<MetaType> &foreign, CollectMode mode,  QTypeRevision defaultRevision)
{
    const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(classDef);
    for (const BaseType &superObject : std::as_const(classDef.superClasses())) {
        if (superObject.access == Access::Public) {
            const QAnyStringView superName = superObject.name;

            const CollectMode superMode = (mode == TopLevel) ? SuperClass : RelatedType;
            if (const FoundType found = findType(types, foreign, superName, namespaces)) {
                const MetaType other = found.select(classDef, "Base");
                collect(other, types, foreign, superMode, defaultRevision);
                if (mode == TopLevel && superClass.isEmpty())
                    superClass = other.qualifiedClassName();
            }

            // If we cannot locate a type for it, there is no point in recording the superClass
        }
    }
}

void QmlTypesClassDescription::collectInterfaces(const MetaType &classDef)
{
    for (const Interface &iface : classDef.ifaces())
        implementsInterfaces << interfaceName(iface);
}

void QmlTypesClassDescription::collectLocalAnonymous(
        const MetaType &classDef, const QVector<MetaType> &types,
        const QVector<MetaType> &foreign, QTypeRevision defaultRevision)
{
    file = classDef.inputFile();

    resolvedClass = classDef;
    className = classDef.qualifiedClassName();

    switch (classDef.kind()) {
    case MetaType::Kind::Object:
        accessSemantics = DotQmltypes::S_REFERENCE;
        break;
    case MetaType::Kind::Gadget:
        accessSemantics = DotQmltypes::S_VALUE;
        break;
    case MetaType::Kind::Namespace:
    case MetaType::Kind::Unknown:
        accessSemantics = DotQmltypes::S_NONE;
        break;
    }

    for (const ClassInfo &obj : classDef.classInfos()) {
        if (obj.name == S_DEFAULT_PROPERTY)
            defaultProp = obj.value;
        else if (obj.name == S_PARENT_PROPERTY)
            parentProp = obj.value;
        else if (obj.name == S_REGISTER_ENUM_CLASSES_UNSCOPED && obj.value == S_FALSE)
            registerEnumClassesScoped = true;
    }

    collectInterfaces(classDef);
    collectSuperClasses(classDef, types, foreign, TopLevel, defaultRevision);
}

void QmlTypesClassDescription::collect(
        const MetaType &classDef, const QVector<MetaType> &types,
        const QVector<MetaType> &foreign, CollectMode mode, QTypeRevision defaultRevision)
{
    if (file.isEmpty())
        file = classDef.inputFile();

    const QAnyStringView classDefName = classDef.className();
    const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(classDef);

    QAnyStringView foreignTypeName;
    bool foreignIsNamespace = false;
    bool isConstructible = false;
    for (const ClassInfo &obj : classDef.classInfos()) {
        const QAnyStringView name = obj.name;
        const QAnyStringView value = obj.value;

        if (name == S_DEFAULT_PROPERTY) {
            if (mode != RelatedType && defaultProp.isEmpty())
                defaultProp = value;
            continue;
        }

        if (name == S_PARENT_PROPERTY) {
            if (mode != RelatedType && parentProp.isEmpty())
                parentProp = value;
            continue;
        }

        if (name == S_REGISTER_ENUM_CLASSES_UNSCOPED) {
            if (mode != RelatedType && value == S_FALSE)
                registerEnumClassesScoped = true;
            continue;
        }

        if (name == S_ADDED_IN_VERSION) {
            const QTypeRevision revision = handleInMinorVersion(
                    QTypeRevision::fromEncodedVersion(toInt(value)),
                    defaultRevision.majorVersion());
            revisions.append(revision);
            if (mode == TopLevel)
                addedInRevision = revision;
            continue;
        }

        if (mode != TopLevel)
            continue;

        if (name == S_REMOVED_IN_VERSION) {
            removedInRevision = handleInMinorVersion(
                    QTypeRevision::fromEncodedVersion(toInt(value)),
                    defaultRevision.majorVersion());
            continue;
        }

        // These only apply to the original class
        if (name == S_ELEMENT) {
            if (value == S_AUTO)
                elementNames.append(classDefName);
            else if (value != S_ANONYMOUS)
                elementNames.append(value);
        } else if (name == S_CREATABLE) {
            isCreatable = (value != S_FALSE);
        } else if (name == S_CREATION_METHOD) {
            isStructured = (value == S_STRUCTURED);
            isConstructible = isStructured || (value == S_CONSTRUCT);
        } else if (name == S_ATTACHED) {
            if (const FoundType attached = collectRelated(
                        value, types, foreign, defaultRevision, namespaces)) {
                attachedType = attached.select(classDef, "Attached").qualifiedClassName();
            }
        } else if (name == S_EXTENDED) {
            if (const FoundType extension = collectRelated(
                        value, types, foreign, defaultRevision, namespaces)) {
                javaScriptExtensionType = extension.javaScript.qualifiedClassName();
                nativeExtensionType = extension.native.qualifiedClassName();
            }
        } else if (name == S_EXTENSION_IS_JAVA_SCRIPT) {
            if (value == S_TRUE)
                extensionIsJavaScript = true;
        } else if (name == S_EXTENSION_IS_NAMESPACE) {
            if (value == S_TRUE)
                extensionIsNamespace = true;
        } else if (name == S_SEQUENCE) {
            if (const FoundType element = collectRelated(
                        value, types, foreign, defaultRevision, namespaces)) {
                sequenceValueType = element.select(classDef, "Sequence value").qualifiedClassName();
            } else {
                // TODO: get rid of this once we have JSON data for the builtins.
                sequenceValueType = value;
            }
        } else if (name == S_SINGLETON) {
            if (value == S_TRUE)
                isSingleton = true;
        } else if (name == S_FOREIGN) {
            foreignTypeName = value;
        } else if (name == S_FOREIGN_IS_NAMESPACE) {
            foreignIsNamespace = (value == S_TRUE);
        } else if (name == S_PRIMITIVE_ALIAS) {
            primitiveAliases.append(value);
        } else if (name == S_ROOT) {
            isRootClass = (value == S_TRUE);
        } else if (name == S_HAS_CUSTOM_PARSER) {
            if (value == S_TRUE)
                hasCustomParser = true;
        } else if (name == S_DEFERRED_PROPERTY_NAMES) {
            deferredNames = split(value, QLatin1StringView(","));
        } else if (name == S_IMMEDIATE_PROPERTY_NAMES) {
            immediateNames = split(value, QLatin1StringView(","));
        }
    }

    if (addedInRevision.isValid() && !elementNames.isEmpty())
        revisions.append(addedInRevision);

    // If the local type is a namespace the result can only be a namespace,
    // no matter what the foreign type is.
    const bool isNamespace = foreignIsNamespace || classDef.kind() == MetaType::Kind::Namespace;

    MetaType resolved = classDef;
    if (!foreignTypeName.isEmpty()) {
        // We can re-use a type with own QML.* macros as target of QML.Foreign
        if (const FoundType found = findType(foreign, types, foreignTypeName, namespaces)) {
            resolved = found.select(classDef, "Foreign");

            // Default properties and enum classes are always local.
            defaultProp = {};
            registerEnumClassesScoped = false;

            // Foreign type can have a default property or an attached type,
            // or RegisterEnumClassesUnscoped classinfo.
            for (const ClassInfo &obj : resolved.classInfos()) {
                const QAnyStringView foreignName = obj.name;
                const QAnyStringView foreignValue = obj.value;
                if (defaultProp.isEmpty() && foreignName == S_DEFAULT_PROPERTY) {
                    defaultProp = foreignValue;
                } else if (parentProp.isEmpty() && foreignName == S_PARENT_PROPERTY) {
                    parentProp = foreignValue;
                } else if (foreignName == S_REGISTER_ENUM_CLASSES_UNSCOPED) {
                    if (foreignValue == S_FALSE)
                        registerEnumClassesScoped = true;
                } else if (foreignName == S_ATTACHED) {
                    if (const FoundType attached = collectRelated(
                                foreignValue, types, foreign, defaultRevision, namespaces)) {
                        attachedType = attached.select(resolved, "Attached").qualifiedClassName();
                    }
                } else if (foreignName == S_EXTENDED) {
                    if (const FoundType extension = collectRelated(
                                foreignValue, types, foreign, defaultRevision, namespaces)) {
                        nativeExtensionType = extension.native.qualifiedClassName();
                        javaScriptExtensionType = extension.javaScript.qualifiedClassName();
                    }
                } else if (foreignName == S_EXTENSION_IS_JAVA_SCRIPT) {
                    if (foreignValue == S_TRUE)
                        extensionIsJavaScript = true;
                } else if (foreignName == S_EXTENSION_IS_NAMESPACE) {
                    if (foreignValue == S_TRUE)
                        extensionIsNamespace = true;
                } else if (foreignName == S_SEQUENCE) {
                    if (const FoundType element = collectRelated(
                                foreignValue, types, foreign, defaultRevision, namespaces)) {
                        sequenceValueType
                                = element.select(resolved, "Sequence value").qualifiedClassName();
                    }
                }
            }
        } else {
            className = foreignTypeName;
            resolved = MetaType();
        }
    }

    if (!resolved.isEmpty()) {
        if (mode == RelatedType || !elementNames.isEmpty()) {
            collectExtraVersions(resolved.properties(), revisions);
            collectExtraVersions(resolved.methods(), revisions);
            collectExtraVersions(resolved.sigs(), revisions);
        }

        collectSuperClasses(resolved, types, foreign, mode, defaultRevision);
    }

    if (mode != TopLevel)
        return;

    if (!resolved.isEmpty())
        collectInterfaces(resolved);

    if (!addedInRevision.isValid()) {
        addedInRevision = defaultRevision;
    }
    if (addedInRevision <= defaultRevision
        && (!removedInRevision.isValid() || defaultRevision < removedInRevision)) {
        revisions.append(defaultRevision);
    }

    std::sort(revisions.begin(), revisions.end());
    const auto end = std::unique(revisions.begin(), revisions.end());
    revisions.erase(QList<QTypeRevision>::const_iterator(end), revisions.constEnd());

    resolvedClass = resolved;
    if (className.isEmpty() && !resolved.isEmpty())
        className = resolved.qualifiedClassName();

    if (!sequenceValueType.isEmpty()) {
        isCreatable = false;
        accessSemantics = DotQmltypes::S_SEQUENCE;
    } else if (isNamespace) {
        isCreatable = false;
        accessSemantics = DotQmltypes::S_NONE;
    } else if (resolved.kind() == MetaType::Kind::Object) {
        accessSemantics = DotQmltypes::S_REFERENCE;
    } else {
        isCreatable = isConstructible;

        if (resolved.isEmpty()) {
            if (elementNames.isEmpty()) {
                // If no resolved, we generally assume it's a value type defined by the
                // foreign/extended trick.
                accessSemantics = DotQmltypes::S_VALUE;
            }

            for (auto elementName = elementNames.begin(); elementName != elementNames.end();) {
                if (elementName->isEmpty() || elementName->front().isLower()) {
                    // If no resolved, we generally assume it's a value type defined by the
                    // foreign/extended trick.
                    accessSemantics = DotQmltypes::S_VALUE;
                    ++elementName;
                } else {
                    // Objects and namespaces always have metaobjects and therefore classDefs.
                    // However, we may not be able to resolve the metaobject at compile time. See
                    // the "Invisible" test case. In that case, we must not assume anything about
                    // access semantics.

                    warning(classDef)
                            << "Refusing to generate non-lowercase name"
                            << *elementName << "for unknown foreign type";
                    elementName = elementNames.erase(elementName);

                    if (elementNames.isEmpty()) {
                        // Make it completely inaccessible.
                        // We cannot get enums from anonymous types after all.
                        accessSemantics = DotQmltypes::S_NONE;
                    }
                }
            }
        } else if (resolved.kind() == MetaType::Kind::Gadget) {
            accessSemantics = DotQmltypes::S_VALUE;
        } else {
            accessSemantics = DotQmltypes::S_NONE;
        }
    }
}

FoundType QmlTypesClassDescription::collectRelated(
        QAnyStringView related, const QVector<MetaType> &types, const QVector<MetaType> &foreign,
        QTypeRevision defaultRevision, const QList<QAnyStringView> &namespaces)
{
    if (FoundType other = findType(types, foreign, related, namespaces)) {
        if (!other.native.isEmpty())
            collect(other.native, types, foreign, RelatedType, defaultRevision);
        if (!other.javaScript.isEmpty())
            collect(other.javaScript, types, foreign, RelatedType, defaultRevision);
        return other;
    }
    return FoundType();
}

struct UsingCompare {
    bool operator()(const UsingDeclaration &a, QAnyStringView b) const
    {
        return a.alias < b;
    }

    bool operator()(QAnyStringView a, const UsingDeclaration &b) const
    {
        return a < b.alias;
    }
};

ResolvedTypeAlias::ResolvedTypeAlias(
        QAnyStringView alias, const QList<UsingDeclaration> &usingDeclarations)
    : type(alias)
{
    handleVoid();
    if (type.isEmpty())
        return;

    handleList();

    if (!isList) {
        handlePointer();
        handleConst();
    }

    while (true) {
        const auto usingDeclaration = std::equal_range(
                usingDeclarations.begin(), usingDeclarations.end(), type, UsingCompare());
        if (usingDeclaration.first == usingDeclaration.second)
            break;

        type = usingDeclaration.first->original;
        handleVoid();
        if (type.isEmpty())
            return;

        if (isPointer) {
            handleConst();
            continue;
        }

        if (!isList) {
            handleList();
            if (!isList) {
                handlePointer();
                handleConst();
            }
        }
    }
}

void ResolvedTypeAlias::handleVoid()
{
    if (!isPointer && type == "void")
        type = "";
}

void ResolvedTypeAlias::handleList()
{
    for (QLatin1StringView list : {"QQmlListProperty<"_L1, "QList<"_L1}) {
        if (!startsWith(type, list) || type.back() != '>'_L1)
            continue;

        const int listSize = list.size();
        const QAnyStringView elementType = trimmed(type.mid(listSize, type.size() - listSize - 1));

               // QQmlListProperty internally constructs the pointer. Passing an explicit '*' will
               // produce double pointers. QList is only for value types. We can't handle QLists
               // of pointers (unless specially registered, but then they're not isList).
        if (elementType.back() == '*'_L1)
            continue;

        isList = true;
        type = elementType;
        return;
    }
}

void ResolvedTypeAlias::handlePointer()
{
    if (type.back() == '*'_L1) {
        isPointer = true;
        type = type.chopped(1);
    }
}

void ResolvedTypeAlias::handleConst()
{
    if (startsWith(type, "const "_L1)) {
        isConstant = true;
        type = type.sliced(strlen("const "));
    }
}

QT_END_NAMESPACE
