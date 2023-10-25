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

static void collectExtraVersions(const QCborMap *component, QLatin1StringView key,
                                 QList<QTypeRevision> &extraVersions)
{
    const QCborArray &items = component->value(key).toArray();
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

const QCborMap *QmlTypesClassDescription::findType(
        const QVector<QCborMap> &types, const QVector<QCborMap> &foreign,
        const QAnyStringView &name, const QList<QAnyStringView> &namespaces)
{
    const auto compare = [](const QCborMap &type, const QAnyStringView &typeName) {
        return toStringView(type, S_QUALIFIED_CLASS_NAME) < typeName;
    };

    const auto tryFindType = [&](QAnyStringView qualifiedName) -> const QCborMap * {
        for (const QVector<QCborMap> &t : {types, foreign}) {
            const auto it = std::lower_bound(t.begin(), t.end(), qualifiedName, compare);
            if (it != t.end() && toStringView(*it, S_QUALIFIED_CLASS_NAME) == qualifiedName)
                return &(*it);
        }

        return nullptr;
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
        if (const QCborMap *found = tryFindType(qualified))
            return found;

        qualified.truncate(0);
    }

    return tryFindType(name);
}

void QmlTypesClassDescription::collectSuperClasses(
        const QCborMap *classDef, const QVector<QCborMap> &types,
        const QVector<QCborMap> &foreign, CollectMode mode,  QTypeRevision defaultRevision)
{
    const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(*classDef);
    const auto supers = classDef->value(S_SUPER_CLASSES).toArray();
    for (const QCborValue &superValue : supers) {
        const QCborMap superObject = superValue.toMap();
        if (toStringView(superObject, S_ACCESS) == S_PUBLIC) {
            const QAnyStringView superName = toStringView(superObject, S_NAME);

            const CollectMode superMode = (mode == TopLevel) ? SuperClass : RelatedType;
            if (const QCborMap *other = findType(types, foreign, superName, namespaces)) {
                collect(other, types, foreign, superMode, defaultRevision);
                if (mode == TopLevel && superClass.isEmpty())
                    superClass = toStringView(*other, S_QUALIFIED_CLASS_NAME);
            }

            // If we cannot locate a type for it, there is no point in recording the superClass
        }
    }
}

void QmlTypesClassDescription::collectInterfaces(const QCborMap *classDef)
{
    if (classDef->contains(S_INTERFACES)) {
        const QCborArray array = classDef->value(S_INTERFACES).toArray();
        for (const QCborValue &value : array) {
            auto object = value.toArray()[0].toMap();
            implementsInterfaces << toStringView(object, S_CLASS_NAME);
        }
    }
}

void QmlTypesClassDescription::collectLocalAnonymous(
        const QCborMap *classDef, const QVector<QCborMap> &types,
        const QVector<QCborMap> &foreign, QTypeRevision defaultRevision)
{
    file = toStringView(*classDef, S_INPUT_FILE);

    resolvedClass = classDef;
    className = toStringView(*classDef, S_QUALIFIED_CLASS_NAME);

    if (classDef->value(S_OBJECT).toBool())
        accessSemantics = DotQmltypes::S_REFERENCE;
    else if (classDef->value(S_GADGET).toBool())
        accessSemantics = DotQmltypes::S_VALUE;
    else
        accessSemantics = DotQmltypes::S_NONE;

    const auto classInfos = classDef->value(S_CLASS_INFOS).toArray();
    for (const QCborValue &classInfo : classInfos) {
        const QCborMap obj = classInfo.toMap();
        if (obj[S_NAME] == S_DEFAULT_PROPERTY)
            defaultProp = toStringView(obj, S_VALUE);
        if (obj[S_NAME] == S_PARENT_PROPERTY)
            parentProp = toStringView(obj, S_VALUE);
    }

    collectInterfaces(classDef);
    collectSuperClasses(classDef, types, foreign, TopLevel, defaultRevision);
}

void QmlTypesClassDescription::collect(
        const QCborMap *classDef, const QVector<QCborMap> &types,
        const QVector<QCborMap> &foreign, CollectMode mode, QTypeRevision defaultRevision)
{
    if (file.isEmpty())
        file = toStringView(*classDef, S_INPUT_FILE);

    const auto classInfos = classDef->value(S_CLASS_INFOS).toArray();
    const QAnyStringView classDefName = toStringView(*classDef, S_CLASS_NAME);
    const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(*classDef);

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

        if (const bool added = (name == S_ADDED_IN_VERSION);
            added || name == S_REMOVED_IN_VERSION) {
            QTypeRevision revision = QTypeRevision::fromEncodedVersion(toInt(value));
            revision = handleInMinorVersion(revision, defaultRevision.majorVersion());
            if (mode == TopLevel) {
                (added ? addedInRevision : removedInRevision) = revision;
            }
            continue;
        }

        if (mode != TopLevel)
            continue;

        // These only apply to the original class
        if (name == S_ELEMENT) {
            if (value == S_AUTO)
                elementName = classDefName;
            else if (value != S_ANONYMOUS)
                elementName = value;
        } else if (name == S_CREATABLE) {
            isCreatable = (value != S_FALSE);
        } else if (name == S_CREATION_METHOD) {
            isStructured = (value == S_STRUCTURED);
            isConstructible = isStructured || (value == S_CONSTRUCT);
        } else if (name == S_ATTACHED) {
            if (const QCborMap *attached = collectRelated(
                        value, types, foreign, defaultRevision, namespaces)) {
                attachedType = toStringView(*attached, S_QUALIFIED_CLASS_NAME);
            }
        } else if (name == S_EXTENDED) {
            if (const QCborMap *extension = collectRelated(
                        value, types, foreign, defaultRevision, namespaces)) {
                extensionType = toStringView(*extension, S_QUALIFIED_CLASS_NAME);
            }
        } else if (name == S_EXTENSION_IS_NAMESPACE) {
            if (value == S_TRUE)
                extensionIsNamespace = true;
        } else if (name == S_SEQUENCE) {
            if (const QCborMap *element = collectRelated(
                        value, types, foreign, defaultRevision, namespaces)) {
                sequenceValueType = toStringView(*element, S_QUALIFIED_CLASS_NAME);
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
        } else if (name == S_OMIT_FROM_QML_TYPES) {
            if (value == S_TRUE)
                omitFromQmlTypes = true;
        } else if (name == S_HAS_CUSTOM_PARSER) {
            if (value == S_TRUE)
                hasCustomParser = true;
        } else if (name == S_DEFERRED_PROPERTY_NAMES) {
            deferredNames = split(value, ',');
        } else if (name == S_IMMEDIATE_PROPERTY_NAMES) {
            immediateNames = split(value, ',');
        }
    }

    if (addedInRevision.isValid() && !elementName.isEmpty())
        revisions.append(addedInRevision);

    // If the local type is a namespace the result can only be a namespace,
    // no matter what the foreign type is.
    const bool isNamespace = foreignIsNamespace || classDef->value(S_NAMESPACE).toBool();

    if (!foreignTypeName.isEmpty()) {
        // We can re-use a type with own QML.* macros as target of QML.Foreign
        if (const QCborMap *other = findType(foreign, types, foreignTypeName, namespaces)) {
            classDef = other;

            // Default properties are always local.
            defaultProp = {};

            // Foreign type can have a default property or an attached types
            const auto classInfos = classDef->value(S_CLASS_INFOS).toArray();
            for (const QCborValue &classInfo : classInfos) {
                const QCborMap obj = classInfo.toMap();
                const QAnyStringView foreignName = toStringView(obj, S_NAME);
                const QAnyStringView foreignValue = toStringView(obj, S_VALUE);
                if (defaultProp.isEmpty() && foreignName == S_DEFAULT_PROPERTY) {
                    defaultProp = foreignValue;
                } else if (parentProp.isEmpty() && foreignName == S_PARENT_PROPERTY) {
                    parentProp = foreignValue;
                } else if (foreignName == S_ATTACHED) {
                    if (const QCborMap *attached = collectRelated(
                                foreignValue, types, foreign, defaultRevision, namespaces)) {
                        attachedType = toStringView(*attached, S_QUALIFIED_CLASS_NAME);
                    }
                } else if (foreignName == S_EXTENDED) {
                    if (const QCborMap *extension = collectRelated(
                                foreignValue, types, foreign, defaultRevision, namespaces)) {
                        extensionType = toStringView(*extension, S_QUALIFIED_CLASS_NAME);
                    }
                } else if (foreignName == S_EXTENSION_IS_NAMESPACE) {
                    if (foreignValue == S_TRUE)
                        extensionIsNamespace = true;
                } else if (foreignName == S_SEQUENCE) {
                    if (const QCborMap *element = collectRelated(
                                foreignValue, types, foreign, defaultRevision, namespaces)) {
                        sequenceValueType = toStringView(*element, S_QUALIFIED_CLASS_NAME);
                    }
                }
            }
        } else {
            className = foreignTypeName;
            classDef = nullptr;
        }
    }

    if (classDef) {
        if (mode == RelatedType || !elementName.isEmpty()) {
            collectExtraVersions(classDef, S_PROPERTIES, revisions);
            collectExtraVersions(classDef, S_SLOTS, revisions);
            collectExtraVersions(classDef, S_METHODS, revisions);
            collectExtraVersions(classDef, S_SIGNALS, revisions);
        }

        collectSuperClasses(classDef, types, foreign, mode, defaultRevision);
    }

    if (mode != TopLevel)
        return;

    if (classDef)
        collectInterfaces(classDef);

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

    resolvedClass = classDef;
    if (className.isEmpty() && classDef)
        className = toStringView(*classDef, S_QUALIFIED_CLASS_NAME);

    if (!sequenceValueType.isEmpty()) {
        isCreatable = false;
        accessSemantics = DotQmltypes::S_SEQUENCE;
    } else if (isNamespace) {
        isCreatable = false;
        accessSemantics = DotQmltypes::S_NONE;
    } else if (classDef && classDef->value(S_OBJECT).toBool()) {
        accessSemantics = DotQmltypes::S_REFERENCE;
    } else {
        isCreatable = isConstructible;

        if (!classDef) {
            if (elementName.isEmpty() || elementName.front().isLower()) {
                // If no classDef, we generally assume it's a value type defined by the
                // foreign/extended trick.
                accessSemantics = DotQmltypes::S_VALUE;
            } else {
                // Objects and namespaces always have metaobjects and therefore classDefs.
                // However, we may not be able to resolve the metaobject at compile time. See
                // the "Invisible" test case. In that case, we must not assume anything about
                // access semantics.

                qWarning() << "Warning: Refusing to generate non-lowercase name"
                           << elementName.toString() << "for unknown foreign type";
                elementName = {};

                // Make it completely inaccessible.
                // We cannot get enums from anonymous types after all.
                accessSemantics = DotQmltypes::S_NONE;
            }
        } else if (classDef->value(S_GADGET).toBool()) {
            accessSemantics = DotQmltypes::S_VALUE;
        } else {
            accessSemantics = DotQmltypes::S_NONE;
        }
    }
}

const QCborMap *QmlTypesClassDescription::collectRelated(
        QAnyStringView related, const QVector<QCborMap> &types, const QVector<QCborMap> &foreign,
        QTypeRevision defaultRevision, const QList<QAnyStringView> &namespaces)
{
    if (const QCborMap *other = findType(types, foreign, related, namespaces)) {
        collect(other, types, foreign, RelatedType, defaultRevision);
        return other;
    }
    return nullptr;
}

QT_END_NAMESPACE
