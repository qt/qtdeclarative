// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltypesclassdescription_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qanystringviewutils_p.h"

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
    for (const QCborValue item : items) {
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
        const QVector<QCborMap> &types, const QAnyStringView &name)
{
    auto it = std::lower_bound(types.begin(), types.end(), name,
                               [&](const QCborMap &type, const QAnyStringView &typeName) {
                                   return toStringView(type, S_QUALIFIED_CLASS_NAME) < typeName;
    });

    return (it != types.end() && toStringView(*it, S_QUALIFIED_CLASS_NAME) == name)
            ? &(*it)
            : nullptr;
}

void QmlTypesClassDescription::collectSuperClasses(
        const QCborMap *classDef, const QVector<QCborMap> &types,
        const QVector<QCborMap> &foreign, CollectMode mode,  QTypeRevision defaultRevision)
{
    const auto supers = classDef->value(S_SUPER_CLASSES).toArray();
    for (const QCborValue superValue : supers) {
        const QCborMap superObject = superValue.toMap();
        if (toStringView(superObject, S_ACCESS) == S_PUBLIC) {
            const QAnyStringView superName = toStringView(superObject, S_NAME);

            const CollectMode superMode = (mode == TopLevel) ? SuperClass : RelatedType;
            if (const QCborMap *other = findType(types, superName))
                collect(other, types, foreign, superMode, defaultRevision);
            else if (const QCborMap *other = findType(foreign, superName))
                collect(other, types, foreign, superMode, defaultRevision);
            else // If we cannot locate a type for it, there is no point in recording the superClass
                continue;

            if (mode == TopLevel && superClass.isEmpty())
                superClass = superName;
        }
    }
}

void QmlTypesClassDescription::collectInterfaces(const QCborMap *classDef)
{
    if (classDef->contains(S_INTERFACES)) {
        const QCborArray array = classDef->value(S_INTERFACES).toArray();
        for (const QCborValue value : array) {
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
    for (const QCborValue classInfo : classInfos) {
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
    QAnyStringView foreignTypeName;
    bool explicitCreatable = false;
    for (const QCborValue classInfo : classInfos) {
        const QCborMap obj = classInfo.toMap();
        const QAnyStringView name = toStringView(obj, S_NAME);
        const QAnyStringView value = toStringView(obj, S_VALUE);

        if (name == S_DEFAULT_PROPERTY) {
            if (mode != RelatedType && defaultProp.isEmpty())
                defaultProp = value;
        } else if (name == S_PARENT_PROPERTY) {
            if (mode != RelatedType && parentProp.isEmpty())
                parentProp = value;
        } else if (name == S_ADDED_IN_VERSION) {
            const QTypeRevision revision = QTypeRevision::fromEncodedVersion(toInt(value));
            if (mode == TopLevel) {
                addedInRevision = revision;
                revisions.append(revision);
            } else if (!elementName.isEmpty()) {
                revisions.append(revision);
            }
        }

        if (mode != TopLevel)
            continue;

        // These only apply to the original class
        if (name == S_ELEMENT) {
            if (value == S_AUTO)
                elementName = classDefName;
            else if (value != S_ANONYMOUS)
                elementName = value;
        } else if (name == S_REMOVED_IN_VERSION) {
            removedInRevision = QTypeRevision::fromEncodedVersion(toInt(value));
        } else if (name == S_CREATABLE) {
            isCreatable = (value != S_FALSE);
            explicitCreatable = true;
        } else if (name == S_ATTACHED) {
            attachedType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == S_EXTENDED) {
            extensionType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == S_EXTENSION_IS_NAMESPACE) {
            if (value == S_TRUE)
                extensionIsNamespace = true;
        } else if (name == S_SEQUENCE) {
            sequenceValueType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == S_SINGLETON) {
            if (value == S_TRUE)
                isSingleton = true;
        } else if (name == S_FOREIGN) {
            foreignTypeName = value;
        } else if (name == S_OMIT_FROM_QML_TYPES) {
            if (value == S_TRUE)
                omitFromQmlTypes = true;
        } else if (name == S_HAS_CUSTOM_PARSER) {
            if (value == S_TRUE)
                hasCustomParser = true;
        } else if (name == S_DEFERRED_PROPERTY_NAMES) {
            deferredNames = split(value, QLatin1Char(','));
        } else if (name == S_IMMEDIATE_PROPERTY_NAMES) {
            immediateNames = split(value, QLatin1Char(','));
        }
    }

    // If the local type is a namespace the result can only be a namespace,
    // no matter what the foreign type is.
    const bool isNamespace = classDef->value(S_NAMESPACE).toBool();

    if (!foreignTypeName.isEmpty()) {
        const QCborMap *other = findType(foreign, foreignTypeName);

        // We can re-use a type with own S_* macros as target of S_FOREIGN
        if (!other)
            other = findType(types, foreignTypeName);

        if (other) {
            classDef = other;

            // Default properties are always local.
            defaultProp = {};

            // Foreign type can have a default property or an attached types
            const auto classInfos = classDef->value(S_CLASS_INFOS).toArray();
            for (const QCborValue classInfo : classInfos) {
                const QCborMap obj = classInfo.toMap();
                const QAnyStringView foreignName = toStringView(obj, S_NAME);
                const QAnyStringView foreignValue = toStringView(obj, S_VALUE);
                if (defaultProp.isEmpty() && foreignName == S_DEFAULT_PROPERTY) {
                    defaultProp = foreignValue;
                } else if (parentProp.isEmpty() && foreignName == S_PARENT_PROPERTY) {
                    parentProp = foreignValue;
                } else if (foreignName == S_ATTACHED) {
                    attachedType = foreignValue;
                    collectRelated(foreignValue, types, foreign, defaultRevision);
                } else if (foreignName == S_EXTENDED) {
                    extensionType = foreignValue;
                    collectRelated(foreignValue, types, foreign, defaultRevision);
                } else if (foreignName == S_EXTENSION_IS_NAMESPACE) {
                    if (foreignValue == S_TRUE)
                        extensionIsNamespace = true;
                } else if (foreignName == S_SEQUENCE) {
                    sequenceValueType = foreignValue;
                    collectRelated(foreignValue, types, foreign, defaultRevision);
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
        revisions.append(defaultRevision);
        addedInRevision = defaultRevision;
    } else if (addedInRevision < defaultRevision) {
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
        if (!explicitCreatable)
            isCreatable = false;

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

void QmlTypesClassDescription::collectRelated(
        QAnyStringView related, const QVector<QCborMap> &types, const QVector<QCborMap> &foreign,
        QTypeRevision defaultRevision)
{
    if (const QCborMap *other = findType(types, related))
        collect(other, types, foreign, RelatedType, defaultRevision);
    else if (const QCborMap *other = findType(foreign, related))
        collect(other, types, foreign, RelatedType, defaultRevision);
}

QT_END_NAMESPACE
