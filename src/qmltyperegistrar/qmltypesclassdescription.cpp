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

#include <QtCore/qjsonarray.h>

static void collectExtraVersions(const QJsonObject *component, const QString &key,
                                 QList<int> &extraVersions)
{
    const QJsonArray &items = component->value(key).toArray();
    for (const QJsonValue &item : items) {
        const QJsonObject obj = item.toObject();
        const auto revision = obj.find(QLatin1String("revision"));
        if (revision != obj.end()) {
            const int extraVersion = revision.value().toInt();
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
                                           CollectMode mode)
{
    const QJsonObject *origClassDef = classDef; // if we find QML.Foreign, classDef changes.
    if (file.isEmpty() && classDef->value(QLatin1String("registerable")).toBool())
        file = classDef->value(QLatin1String("inputFile")).toString();
    const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
    for (const QJsonValue &classInfo : classInfos) {
        const QJsonObject obj = classInfo.toObject();
        const QString name = obj[QLatin1String("name")].toString();
        const QString value = obj[QLatin1String("value")].toString();

        if (name == QLatin1String("DefaultProperty")) {
            if (mode != AttachedType && defaultProp.isEmpty())
                defaultProp = value;
        } else if (name == QLatin1String("QML.AddedInMinorVersion")) {
            if (mode == TopLevel) {
                addedInRevision = value.toInt();
                revisions.append(value.toInt());
            } else if (!elementName.isEmpty()) {
                revisions.append(value.toInt());
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
        } else if (name == QLatin1String("QML.RemovedInMinorVersion")) {
            removedInRevision = value.toInt();
        } else if (name == QLatin1String("QML.Creatable")) {
            isCreatable = (value != QLatin1String("false"));
        } else if (name == QLatin1String("QML.Attached")) {
            collectAttached(value, types, foreign);
        } else if (name == QLatin1String("QML.Singleton")) {
            if (value == QLatin1String("true"))
                isSingleton = true;
        } else if (name == QLatin1String("QML.Foreign")) {
            if (const QJsonObject *other = findType(foreign, value)) {
                classDef = other;
                // Foreign type can have a default property or an attached types
                const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
                for (const QJsonValue &classInfo : classInfos) {
                    const QJsonObject obj = classInfo.toObject();
                    const QString foreignName = obj[QLatin1String("name")].toString();
                    const QString foreignValue = obj[QLatin1String("value")].toString();
                    if (defaultProp.isEmpty() && foreignName == QLatin1String("DefaultProperty"))
                        defaultProp = foreignValue;
                    else if (foreignName == QLatin1String("QML.Attached"))
                        collectAttached(foreignValue, types, foreign);
                }
            }
        } else if (name == QLatin1String("QML.Root")) {
            isRootClass = true;
            isBuiltin = true;
        } else if (name == QLatin1String("QML.Builtin")) {
            isBuiltin = true;
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
        for (const QJsonValue &origSuper : origSupers)
            supers.append(origSuper);
    }

    for (const QJsonValue &superValue : qAsConst(supers)) {
        const QJsonObject superObject = superValue.toObject();
        if (superObject[QLatin1String("access")].toString() == QLatin1String("public")) {
            const QString superName = superObject[QLatin1String("name")].toString();

            if (const QJsonObject *other = findType(types, superName))
                collect(other, types, foreign, mode == TopLevel ? SuperClass : AttachedType);
            else if (const QJsonObject *other = findType(foreign, superName))
                collect(other, types, foreign, mode == TopLevel ? SuperClass : AttachedType);
            else // If we cannot locate a type for it, there is no point in recording the superClass
                continue;

            if (mode == TopLevel && superClass.isEmpty())
                superClass = superName;
        }
    }

    if (mode != TopLevel)
        return;

    if (addedInRevision == -1) {
        revisions.append(0);
        addedInRevision = 0;
    }

    std::sort(revisions.begin(), revisions.end(),
              [](int a, int b) { return QByteArray::number(a) < QByteArray::number(b); });
    const auto end = std::unique(revisions.begin(), revisions.end());
    revisions.erase(end, revisions.end());

    resolvedClass = classDef;

    // If it's not a QObject, it's not creatable
    isCreatable = isCreatable && classDef->value(QLatin1String("object")).toBool();
}

void QmlTypesClassDescription::collectAttached(const QString &attached,
                                               const QVector<QJsonObject> &types,
                                               const QVector<QJsonObject> &foreign)
{
    attachedType = attached;
    if (const QJsonObject *other = findType(types, attachedType))
        collect(other, types, foreign, AttachedType);
    else if (const QJsonObject *other = findType(foreign, attachedType))
        collect(other, types, foreign, AttachedType);
}
