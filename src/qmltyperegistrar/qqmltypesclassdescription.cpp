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

using namespace Constants;
using namespace Constants::MetatypesDotJson;
using namespace Constants::MetatypesDotJson::Qml;
using namespace QAnyStringViewUtils;


static void collectExtraVersions(const QCborMap &component, QLatin1StringView key,
                                 QList<QTypeRevision> &extraVersions)
{
    const QCborArray &items = component.value(key).toArray();
    for (const QCborValue &item : items) {
        const QCborMap obj = item.toMap();
        const auto revision = obj.find(S_REVISION);
        if (revision != obj.end()) {
            const auto extraVersion = QTypeRevision::fromEncodedVersion(revision.value().toInteger());
            if (!extraVersions.contains(extraVersion))
                extraVersions.append(extraVersion);
        }
    }
}

struct Compare {
    bool operator()(const QAnyStringView &typeName, const QCborMap &type) const
    {
        return typeName < toStringView(type, S_QUALIFIED_CLASS_NAME);
    }

    bool operator()(const QCborMap &type, const QAnyStringView &typeName) const
    {
        return toStringView(type, S_QUALIFIED_CLASS_NAME) < typeName;
    }
};

FoundType::FoundType(const QCborMap &single, FoundType::Origin origin)
{
    if (toStringView(single, S_INPUT_FILE).isEmpty()) {
        javaScript = single;
        javaScriptOrigin = origin;
    } else {
        native = single;
        nativeOrigin = origin;
    }
}

QCborMap FoundType::select(const QCborMap &category, QAnyStringView relation) const
{
    if (toStringView(category, S_INPUT_FILE).isEmpty()) {
        if (javaScript.isEmpty()) {
            warning(category)
                    << relation << "type of" << toStringView(category, S_QUALIFIED_CLASS_NAME)
                    << "is not a JavaScript type";
        }
        return javaScript;
    }

    if (native.isEmpty()) {
        warning(category)
                << relation << "of" << toStringView(category, S_QUALIFIED_CLASS_NAME)
                << "is not a native type";
    }
    return native;
}

FoundType QmlTypesClassDescription::findType(
        const QVector<QCborMap> &types, const QVector<QCborMap> &foreign,
        const QAnyStringView &name, const QList<QAnyStringView> &namespaces)
{
    const auto tryFindType = [&](QAnyStringView qualifiedName) -> FoundType {
        FoundType result;
        for (const QVector<QCborMap> &t : {types, foreign}) {
            const auto [first, last] = std::equal_range(
                    t.begin(), t.end(), qualifiedName, Compare());
            for (auto it = first; it != last; ++it) {
                Q_ASSERT(toStringView(*it, S_QUALIFIED_CLASS_NAME) == qualifiedName);

                if (toStringView(*it, S_INPUT_FILE).isEmpty()) {
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
        const QCborMap &classDef, const QVector<QCborMap> &types,
        const QVector<QCborMap> &foreign, CollectMode mode,  QTypeRevision defaultRevision)
{
    const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(classDef);
    const auto supers = classDef.value(S_SUPER_CLASSES).toArray();
    for (const QCborValue &superValue : supers) {
        const QCborMap superObject = superValue.toMap();
        if (toStringView(superObject, S_ACCESS) == S_PUBLIC) {
            const QAnyStringView superName = toStringView(superObject, S_NAME);

            const CollectMode superMode = (mode == TopLevel) ? SuperClass : RelatedType;
            if (const FoundType found = findType(types, foreign, superName, namespaces)) {
                const QCborMap other = found.select(classDef, "Base");
                collect(other, types, foreign, superMode, defaultRevision);
                if (mode == TopLevel && superClass.isEmpty())
                    superClass = toStringView(other, S_QUALIFIED_CLASS_NAME);
            }

            // If we cannot locate a type for it, there is no point in recording the superClass
        }
    }
}

void QmlTypesClassDescription::collectInterfaces(const QCborMap &classDef)
{
    if (classDef.contains(S_INTERFACES)) {
        const QCborArray ifaces = classDef.value(S_INTERFACES).toArray();
        for (const QCborValue &iface : ifaces)
            implementsInterfaces << interfaceName(iface);
    }
}

void QmlTypesClassDescription::collectLocalAnonymous(
        const QCborMap &classDef, const QVector<QCborMap> &types,
        const QVector<QCborMap> &foreign, QTypeRevision defaultRevision)
{
    file = toStringView(classDef, S_INPUT_FILE);

    resolvedClass = classDef;
    className = toStringView(classDef, S_QUALIFIED_CLASS_NAME);

    if (classDef.value(S_OBJECT).toBool())
        accessSemantics = DotQmltypes::S_REFERENCE;
    else if (classDef.value(S_GADGET).toBool())
        accessSemantics = DotQmltypes::S_VALUE;
    else
        accessSemantics = DotQmltypes::S_NONE;

    const auto classInfos = classDef.value(S_CLASS_INFOS).toArray();
    for (const QCborValue &classInfo : classInfos) {
        const QCborMap obj = classInfo.toMap();
        const QCborValue name = obj[S_NAME];
        if (name == S_DEFAULT_PROPERTY)
            defaultProp = toStringView(obj, S_VALUE);
        else if (name == S_PARENT_PROPERTY)
            parentProp = toStringView(obj, S_VALUE);
        else if (name == S_REGISTER_ENUM_CLASSES_UNSCOPED && toStringView(obj, S_VALUE) == S_FALSE)
            registerEnumClassesScoped = true;
    }

    collectInterfaces(classDef);
    collectSuperClasses(classDef, types, foreign, TopLevel, defaultRevision);
}

void QmlTypesClassDescription::collect(
        const QCborMap &classDef, const QVector<QCborMap> &types,
        const QVector<QCborMap> &foreign, CollectMode mode, QTypeRevision defaultRevision)
{
    if (file.isEmpty())
        file = toStringView(classDef, S_INPUT_FILE);

    const auto classInfos = classDef.value(S_CLASS_INFOS).toArray();
    const QAnyStringView classDefName = toStringView(classDef, S_CLASS_NAME);
    const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(classDef);

    QAnyStringView foreignTypeName;
    bool foreignIsNamespace = false;
    bool isConstructible = false;
    for (const QCborValue &classInfo : classInfos) {
        const QCborMap obj = classInfo.toMap();
        const QAnyStringView name = toStringView(obj, S_NAME);
        const QAnyStringView value = toStringView(obj, S_VALUE);

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
                attachedType = toStringView(
                        attached.select(classDef, "Attached"), S_QUALIFIED_CLASS_NAME);
            }
        } else if (name == S_EXTENDED) {
            if (const FoundType extension = collectRelated(
                        value, types, foreign, defaultRevision, namespaces)) {
                javaScriptExtensionType
                        = toStringView(extension.javaScript, S_QUALIFIED_CLASS_NAME);
                nativeExtensionType
                        = toStringView(extension.native, S_QUALIFIED_CLASS_NAME);
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
                sequenceValueType = toStringView(
                        element.select(classDef, "Sequence value"), S_QUALIFIED_CLASS_NAME);
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
        } else if (name == S_ROOT) {
            isRootClass = (value == S_TRUE);
        } else if (name == S_HAS_CUSTOM_PARSER) {
            if (value == S_TRUE)
                hasCustomParser = true;
        } else if (name == S_DEFERRED_PROPERTY_NAMES) {
            deferredNames = split(value, ',');
        } else if (name == S_IMMEDIATE_PROPERTY_NAMES) {
            immediateNames = split(value, ',');
        }
    }

    if (addedInRevision.isValid() && !elementNames.isEmpty())
        revisions.append(addedInRevision);

    // If the local type is a namespace the result can only be a namespace,
    // no matter what the foreign type is.
    const bool isNamespace = foreignIsNamespace || classDef.value(S_NAMESPACE).toBool();

    QCborMap resolved = classDef;
    if (!foreignTypeName.isEmpty()) {
        // We can re-use a type with own QML.* macros as target of QML.Foreign
        if (const FoundType found = findType(foreign, types, foreignTypeName, namespaces)) {
            resolved = found.select(classDef, "Foreign");

            // Default properties and enum classes are always local.
            defaultProp = {};
            registerEnumClassesScoped = false;

            // Foreign type can have a default property or an attached type,
            // or RegisterEnumClassesUnscoped classinfo.
            const auto classInfos = resolved.value(S_CLASS_INFOS).toArray();
            for (const QCborValue &classInfo : classInfos) {
                const QCborMap obj = classInfo.toMap();
                const QAnyStringView foreignName = toStringView(obj, S_NAME);
                const QAnyStringView foreignValue = toStringView(obj, S_VALUE);
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
                        attachedType = toStringView(
                                attached.select(resolved, "Attached"), S_QUALIFIED_CLASS_NAME);
                    }
                } else if (foreignName == S_EXTENDED) {
                    if (const FoundType extension = collectRelated(
                                foreignValue, types, foreign, defaultRevision, namespaces)) {
                        nativeExtensionType
                                = toStringView(extension.native, S_QUALIFIED_CLASS_NAME);
                        javaScriptExtensionType
                                = toStringView(extension.javaScript, S_QUALIFIED_CLASS_NAME);
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
                        sequenceValueType = toStringView(
                                element.select(resolved, "Sequence value"),
                                S_QUALIFIED_CLASS_NAME);
                    }
                }
            }
        } else {
            className = foreignTypeName;
            resolved.clear();
        }
    }

    if (!resolved.isEmpty()) {
        if (mode == RelatedType || !elementNames.isEmpty()) {
            collectExtraVersions(resolved, S_PROPERTIES, revisions);
            collectExtraVersions(resolved, S_SLOTS, revisions);
            collectExtraVersions(resolved, S_METHODS, revisions);
            collectExtraVersions(resolved, S_SIGNALS, revisions);
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
        className = toStringView(resolved, S_QUALIFIED_CLASS_NAME);

    if (!sequenceValueType.isEmpty()) {
        isCreatable = false;
        accessSemantics = DotQmltypes::S_SEQUENCE;
    } else if (isNamespace) {
        isCreatable = false;
        accessSemantics = DotQmltypes::S_NONE;
    } else if (resolved.value(S_OBJECT).toBool()) {
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
        } else if (resolved.value(S_GADGET).toBool()) {
            accessSemantics = DotQmltypes::S_VALUE;
        } else {
            accessSemantics = DotQmltypes::S_NONE;
        }
    }
}

FoundType QmlTypesClassDescription::collectRelated(
        QAnyStringView related, const QVector<QCborMap> &types, const QVector<QCborMap> &foreign,
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

QT_END_NAMESPACE
