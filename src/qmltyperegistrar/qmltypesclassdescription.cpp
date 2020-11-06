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

#include "qmltypesclassdescription.h"
#include "qmltypescreator.h"

#include <QtCore/qjsonarray.h>

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

void QmlTypesClassDescription::collect(const QJsonObject *classDef,
                                           const QVector<QJsonObject> &types,
                                           const QVector<QJsonObject> &foreign,
                                           CollectMode mode, QTypeRevision defaultRevision)
{
    const QJsonObject *origClassDef = classDef; // if we find QML.Foreign, classDef changes.
    if (file.isEmpty() && classDef->value(QLatin1String("registerable")).toBool())
        file = classDef->value(QLatin1String("inputFile")).toString();
    const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
    for (const QJsonValue classInfo : classInfos) {
        const QJsonObject obj = classInfo.toObject();
        const QString name = obj[QLatin1String("name")].toString();
        const QString value = obj[QLatin1String("value")].toString();

        if (name == QLatin1String("DefaultProperty")) {
            if (mode != AttachedType && defaultProp.isEmpty())
                defaultProp = value;
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
                elementName = classDef->value(QLatin1String("className")).toString();
            else if (value != QLatin1String("anonymous"))
                elementName = value;
        } else if (name == QLatin1String("QML.RemovedInVersion")) {
            removedInRevision = QTypeRevision::fromEncodedVersion(value.toInt());
        } else if (name == QLatin1String("QML.Creatable")) {
            isCreatable = (value != QLatin1String("false"));
        } else if (name == QLatin1String("QML.Attached")) {
            attachedType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == QLatin1String("QML.Sequence")) {
            sequenceValueType = value;
            collectRelated(value, types, foreign, defaultRevision);
        } else if (name == QLatin1String("QML.Singleton")) {
            if (value == QLatin1String("true"))
                isSingleton = true;
        } else if (name == QLatin1String("QML.Foreign")) {
            if (const QJsonObject *other = findType(foreign, value)) {
                classDef = other;
                // Foreign type can have a default property or an attached types
                const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
                for (const QJsonValue classInfo : classInfos) {
                    const QJsonObject obj = classInfo.toObject();
                    const QString foreignName = obj[QLatin1String("name")].toString();
                    const QString foreignValue = obj[QLatin1String("value")].toString();
                    if (defaultProp.isEmpty() && foreignName == QLatin1String("DefaultProperty")) {
                        defaultProp = foreignValue;
                    } else if (foreignName == QLatin1String("QML.Attached")) {
                        attachedType = foreignValue;
                        collectRelated(foreignValue, types, foreign, defaultRevision);
                    } else if (foreignName == QLatin1String("QML.Sequence")) {
                        sequenceValueType = foreignValue;
                        collectRelated(foreignValue, types, foreign, defaultRevision);
                    }
                }
            } else {
                // The foreign type does not have a meta object: We only override the name.
                className = value;
            }
        } else if (name == QLatin1String("QML.Root")) {
            isRootClass = true;
        }
    }

    if (mode == AttachedType || !elementName.isEmpty()) {
        collectExtraVersions(classDef, QString::fromLatin1("properties"), revisions);
        collectExtraVersions(classDef, QString::fromLatin1("slots"), revisions);
        collectExtraVersions(classDef, QString::fromLatin1("methods"), revisions);
        collectExtraVersions(classDef, QString::fromLatin1("signals"), revisions);
    }

    auto supers = classDef->value(QLatin1String("superClasses")).toArray();
    if (classDef != origClassDef) {
        const QJsonArray origSupers = origClassDef->value(QLatin1String("superClasses")).toArray();
        for (const QJsonValue origSuper : origSupers)
            supers.append(origSuper);
    }

    for (const QJsonValue superValue : qAsConst(supers)) {
        const QJsonObject superObject = superValue.toObject();
        if (superObject[QLatin1String("access")].toString() == QLatin1String("public")) {
            const QString superName = superObject[QLatin1String("name")].toString();

            const CollectMode superMode = (mode == TopLevel) ? SuperClass : AttachedType;
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

    if (mode != TopLevel)
        return;

    if (!addedInRevision.isValid()) {
        revisions.append(defaultRevision);
        addedInRevision = defaultRevision;
    } else if (addedInRevision < defaultRevision) {
        revisions.append(defaultRevision);
    }

    std::sort(revisions.begin(), revisions.end());
    const auto end = std::unique(revisions.begin(), revisions.end());
    revisions.erase(end, revisions.end());

    resolvedClass = classDef;
    if (className.isEmpty() && mode == TopLevel)
        className = classDef->value(QLatin1String("qualifiedClassName")).toString();

    if (!sequenceValueType.isEmpty()) {
        isCreatable = false;
        accessSemantics = QLatin1String("sequence");
    } else if (classDef->value(QLatin1String("object")).toBool()) {
        accessSemantics = QLatin1String("reference");
    } else {
        isCreatable = false;
        accessSemantics = classDef->value(QLatin1String("gadget")).toBool()
                ? QLatin1String("value")
                : QLatin1String("none");
    }
}

void QmlTypesClassDescription::collectRelated(const QString &related,
                                               const QVector<QJsonObject> &types,
                                               const QVector<QJsonObject> &foreign,
                                               QTypeRevision defaultRevision)
{
    if (const QJsonObject *other = findType(types, related))
        collect(other, types, foreign, AttachedType, defaultRevision);
    else if (const QJsonObject *other = findType(foreign, related))
        collect(other, types, foreign, AttachedType, defaultRevision);
}
