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
    for (const QJsonValue &item : items) {
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
                                           bool topLevel, QTypeRevision defaultRevision)
{
    const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
    for (const QJsonValue &classInfo : classInfos) {
        const QJsonObject obj = classInfo.toObject();
        const QString name = obj[QLatin1String("name")].toString();
        const QString value = obj[QLatin1String("value")].toString();

        if (name == QLatin1String("DefaultProperty")) {
            if (defaultProp.isEmpty())
                defaultProp = value;
        } else if (name == QLatin1String("QML.AddedInVersion")) {
            const QTypeRevision revision = QTypeRevision::fromEncodedVersion(value.toInt());
            if (topLevel) {
                addedInRevision = revision;
                revisions.append(revision);
            } else if (!elementName.isEmpty()) {
                revisions.append(revision);
            }
        }

        if (!topLevel)
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
            if (const QJsonObject *other = findType(types, attachedType))
                collect(other, types, foreign, false, defaultRevision);
            else if (const QJsonObject *other = findType(foreign, attachedType))
                collect(other, types, foreign, false, defaultRevision);
        } else if (name == QLatin1String("QML.Singleton")) {
            if (value == QLatin1String("true"))
                isSingleton = true;
        } else if (name == QLatin1String("QML.Foreign")) {
            if (const QJsonObject *other = findType(foreign, value)) {
                classDef = other;
                if (defaultProp.isEmpty()) {
                    // Foreign type can have a default property
                    const auto classInfos = classDef->value(QLatin1String("classInfos")).toArray();
                    for (const QJsonValue &classInfo : classInfos) {
                        QJsonObject obj = classInfo.toObject();
                        if (obj[QLatin1String("name")].toString() == QLatin1String("DefaultProperty")) {
                            defaultProp = obj[QLatin1String("value")].toString();
                            break;
                        }
                    }
                }
            }
        } else if (name == QLatin1String("QML.Root")) {
            isRootClass = true;
            isBuiltin = true;
        } else if (name == QLatin1String("QML.Builtin")) {
            isBuiltin = true;
        }
    }

    if (!elementName.isEmpty()) {
        collectExtraVersions(classDef, QString::fromLatin1("properties"), revisions);
        collectExtraVersions(classDef, QString::fromLatin1("slots"), revisions);
        collectExtraVersions(classDef, QString::fromLatin1("methods"), revisions);
        collectExtraVersions(classDef, QString::fromLatin1("signals"), revisions);
    }

    const auto supers = classDef->value(QLatin1String("superClasses")).toArray();
    if (!supers.isEmpty()) {
        const QJsonObject superObject = supers.first().toObject();
        if (superObject[QLatin1String("access")].toString() == QLatin1String("public")) {
            const QString superName = superObject[QLatin1String("name")].toString();
            if (topLevel && superClass.isEmpty())
                superClass = superName;

            if (const QJsonObject *other = findType(types, superName))
                collect(other, types, foreign, false, defaultRevision);
            else if (const QJsonObject *other = findType(foreign, superName))
                collect(other, types, foreign, false, defaultRevision);
        }
    }

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
}
