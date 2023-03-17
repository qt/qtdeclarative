// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltypesclassdescription_p.h"
#include "qqmltypescreator_p.h"

#include <QtCore/qjsonarray.h>

QT_BEGIN_NAMESPACE

static void collectExtraVersions(const QJsonObject *component, const QString &key,
                                 QList<QTypeRevision> &extraVersions)
{
    const QJsonArray &items = component->value(key).toArray();
    for (const QJsonValue item : items) {
        const QJsonObject obj = item.toObject();
        const auto revision = obj.find(QLatin1String("revision"));
        if (revision != obj.end()) {
            const auto extraVersion = QTypeRevision::fromEncodedVersion(revision.value().toInt());
            if (!extraVersions.contains(extraVersion))
                extraVersions.append(extraVersion);
        }
    }
}

const QJsonObject *QmlTypesClassDescription::findType(const QVector<QJsonObject> &types,
                                                      const QString &name)
{
    static const QLatin1String qualifiedClassNameKey("qualifiedClassName");
    auto it = std::lower_bound(types.begin(), types.end(), name,
                               [&](const QJsonObject &type, const QString &typeName) {
        return type.value(qualifiedClassNameKey).toString() < typeName;
    });

    return (it != types.end() && it->value(qualifiedClassNameKey) == name) ? &(*it) : nullptr;
}

void QmlTypesClassDescription::collectSuperClasses(
        const QJsonObject *classDef, const QVector<QJsonObject> &types,
        const QVector<QJsonObject> &foreign, CollectMode mode,  QTypeRevision defaultRevision)
{
    const auto supers = classDef->value(QLatin1String("superClasses")).toArray();
    for (const QJsonValue superValue : supers) {
        const QJsonObject superObject = superValue.toObject();
        if (superObject[QLatin1String("access")].toString() == QLatin1String("public")) {
            const QString superName = superObject[QLatin1String("name")].toString();

            const CollectMode superMode = (mode == TopLevel) ? SuperClass : RelatedType;
            if (const QJsonObject *other = findType(types, superName))
                collect(other, types, foreign, superMode, defaultRevision);
            else if (const QJsonObject *other = findType(foreign, superName))
                collect(other, types, foreign, superMode, defaultRevision);
            else // If we cannot locate a type for it, there is no point in recording the superClass
                continue;

            if (mode == TopLevel && superClass.isEmpty())
                superClass = superName;
        }
    }
}

void QmlTypesClassDescription::collectInterfaces(const QJsonObject *classDef)
{
    if (classDef->contains(QLatin1String("interfaces"))) {
        const QJsonArray array = classDef->value(QLatin1String("interfaces")).toArray();
        for (const QJsonValue value : array) {
            auto object = value.toArray()[0].toObject();
            implementsInterfaces << object[QLatin1String("className")].toString();
        }
    }
}

void QmlTypesClassDescription::collectLocalAnonymous(
        const QJsonObject *classDef, const QVector<QJsonObject> &types,
        const QVector<QJsonObject> &foreign, QTypeRevision defaultRevision)
{
    file = classDef->value(QLatin1String("inputFile")).toString();

    resolvedClass = classDef;
    className = classDef->value(QLatin1String("qualifiedClassName")).toString();

    if (classDef->value(QStringLiteral("object")).toBool())
        accessSemantics = QStringLiteral("reference");
    else if (classDef->value(QStringLiteral("gadget")).toBool())
        accessSemantics = QStringLiteral("value");
    else
        accessSemantics = QStringLiteral("none");

    const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
    for (const QJsonValue classInfo : classInfos) {
        const QJsonObject obj = classInfo.toObject();
        if (obj[QStringLiteral("name")].toString() == QStringLiteral("DefaultProperty"))
            defaultProp = obj[QStringLiteral("value")].toString();
        if (obj[QStringLiteral("name")].toString() == QStringLiteral("ParentProperty"))
            parentProp = obj[QStringLiteral("value")].toString();
    }

    collectInterfaces(classDef);
    collectSuperClasses(classDef, types, foreign, TopLevel, defaultRevision);
}

void QmlTypesClassDescription::collect(
        const QJsonObject *classDef, const QVector<QJsonObject> &types,
        const QVector<QJsonObject> &foreign, CollectMode mode, QTypeRevision defaultRevision)
{
    if (file.isEmpty())
        file = classDef->value(QLatin1String("inputFile")).toString();

    const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
    const QString classDefName = classDef->value(QLatin1String("className")).toString();
    QString foreignTypeName;
    for (const QJsonValue classInfo : classInfos) {
        const QJsonObject obj = classInfo.toObject();
        const QString name = obj[QLatin1String("name")].toString();
        const QString value = obj[QLatin1String("value")].toString();

        if (name == QLatin1String("DefaultProperty")) {
            if (mode != RelatedType && defaultProp.isEmpty())
                defaultProp = value;
        } else if (name == QLatin1String("ParentProperty")) {
            if (mode != RelatedType && parentProp.isEmpty())
                parentProp = value;
        } else if (name == QLatin1String("QML.AddedInVersion")) {
            const QTypeRevision revision = QTypeRevision::fromEncodedVersion(value.toInt());
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
        if (name == QLatin1String("QML.Element")) {
            if (value == QLatin1String("auto"))
                elementName = classDefName;
            else if (value != QLatin1String("anonymous"))
                elementName = value;
        } else if (name == QLatin1String("QML.RemovedInVersion")) {
            removedInRevision = QTypeRevision::fromEncodedVersion(value.toInt());
        } else if (name == QLatin1String("QML.Creatable")) {
            isCreatable = (value != QLatin1String("false"));
        } else if (name == QLatin1String("QML.Attached")) {
            attachedType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == QLatin1String("QML.Extended")) {
            extensionType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == QLatin1String("QML.ExtensionIsNamespace")) {
            if (value == QLatin1String("true"))
                extensionIsNamespace = true;
        } else if (name == QLatin1String("QML.Sequence")) {
            sequenceValueType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == QLatin1String("QML.Singleton")) {
            if (value == QLatin1String("true"))
                isSingleton = true;
        } else if (name == QLatin1String("QML.Foreign")) {
            foreignTypeName = value;
        } else if (name == QLatin1String("QML.OmitFromQmlTypes")) {
            if (value == QLatin1String("true"))
                omitFromQmlTypes = true;
        } else if (name == QLatin1String("QML.HasCustomParser")) {
            if (value == QLatin1String("true"))
                hasCustomParser = true;
        } else if (name == QLatin1String("DeferredPropertyNames")) {
            deferredNames = value.split(u',');
        } else if (name == QLatin1String("ImmediatePropertyNames")) {
            immediateNames = value.split(u',');
        }
    }

    // If the local type is a namespace the result can only be a namespace,
    // no matter what the foreign type is.
    const bool isNamespace = classDef->value(QLatin1String("namespace")).toBool();

    if (!foreignTypeName.isEmpty()) {
        const QJsonObject *other = findType(foreign, foreignTypeName);

        // We can re-use a type with own QML_* macros as target of QML_FOREIGN
        if (!other)
            other = findType(types, foreignTypeName);

        if (other) {
            classDef = other;

            // Default properties are always local.
            defaultProp.clear();

            // Foreign type can have a default property or an attached types
            const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
            for (const QJsonValue classInfo : classInfos) {
                const QJsonObject obj = classInfo.toObject();
                const QString foreignName = obj[QLatin1String("name")].toString();
                const QString foreignValue = obj[QLatin1String("value")].toString();
                if (defaultProp.isEmpty() && foreignName == QLatin1String("DefaultProperty")) {
                    defaultProp = foreignValue;
                } else if (parentProp.isEmpty() && foreignName == QLatin1String("ParentProperty")) {
                    parentProp = foreignValue;
                } else if (foreignName == QLatin1String("QML.Attached")) {
                    attachedType = foreignValue;
                    collectRelated(foreignValue, types, foreign, defaultRevision);
                } else if (foreignName == QLatin1String("QML.Extended")) {
                    extensionType = foreignValue;
                    collectRelated(foreignValue, types, foreign, defaultRevision);
                } else if (foreignName == QLatin1String("QML.ExtensionIsNamespace")) {
                    if (foreignValue == QLatin1String("true"))
                        extensionIsNamespace = true;
                } else if (foreignName == QLatin1String("QML.Sequence")) {
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
            collectExtraVersions(classDef, QString::fromLatin1("properties"), revisions);
            collectExtraVersions(classDef, QString::fromLatin1("slots"), revisions);
            collectExtraVersions(classDef, QString::fromLatin1("methods"), revisions);
            collectExtraVersions(classDef, QString::fromLatin1("signals"), revisions);
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
        className = classDef->value(QLatin1String("qualifiedClassName")).toString();

    if (!sequenceValueType.isEmpty()) {
        isCreatable = false;
        accessSemantics = QLatin1String("sequence");
    } else if (isNamespace) {
        isCreatable = false;
        accessSemantics = QLatin1String("none");
    } else if (classDef && classDef->value(QLatin1String("object")).toBool()) {
        accessSemantics = QLatin1String("reference");
    } else {
        isCreatable = false;

        if (!classDef) {
            if (elementName.isEmpty() || elementName[0].isLower()) {
                // If no classDef, we generally assume it's a value type defined by the
                // foreign/extended trick.
                accessSemantics = QLatin1String("value");
            } else {
                // Objects and namespaces always have metaobjects and therefore classDefs.
                // However, we may not be able to resolve the metaobject at compile time. See
                // the "Invisible" test case. In that case, we must not assume anything about
                // access semantics.

                qWarning() << "Warning: Refusing to generate non-lowercase name"
                           << elementName << "for unknown foreign type";
                elementName.clear();

                // Make it completely inaccessible.
                // We cannot get enums from anonymous types after all.
                accessSemantics = QLatin1String("none");
            }
        } else if (classDef->value(QLatin1String("gadget")).toBool()) {
            accessSemantics = QLatin1String("value");
        } else {
            accessSemantics = QLatin1String("none");
        }
    }
}

void QmlTypesClassDescription::collectRelated(const QString &related,
                                               const QVector<QJsonObject> &types,
                                               const QVector<QJsonObject> &foreign,
                                               QTypeRevision defaultRevision)
{
    if (const QJsonObject *other = findType(types, related))
        collect(other, types, foreign, RelatedType, defaultRevision);
    else if (const QJsonObject *other = findType(foreign, related))
        collect(other, types, foreign, RelatedType, defaultRevision);
}

QT_END_NAMESPACE
