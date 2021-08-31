/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "metatypesjsonprocessor.h"

#include <QtCore/qfile.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qqueue.h>


bool MetaTypesJsonProcessor::processTypes(const QStringList &files)
{
    for (const QString &source: files) {
        QJsonDocument metaObjects;
        {
            QFile f(source);
            if (!f.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "Error opening %s for reading\n", qPrintable(source));
                return false;
            }
            QJsonParseError error = {0, QJsonParseError::NoError};
            metaObjects = QJsonDocument::fromJson(f.readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                fprintf(stderr, "Error %d while parsing %s: %s\n", error.error, qPrintable(source),
                        qPrintable(error.errorString()));
                return false;
            }
        }

        if (metaObjects.isArray()) {
            const QJsonArray metaObjectsArray = metaObjects.array();
            for (const QJsonValue metaObject : metaObjectsArray) {
                if (!metaObject.isObject()) {
                    fprintf(stderr, "Error parsing %s: JSON is not an object\n",
                            qPrintable(source));
                    return false;
                }

                processTypes(metaObject.toObject());
            }
        } else if (metaObjects.isObject()) {
            processTypes(metaObjects.object());
        } else {
            fprintf(stderr, "Error parsing %s: JSON is not an object or an array\n",
                    qPrintable(source));
            return false;
        }
    }

    return true;
}

bool MetaTypesJsonProcessor::processForeignTypes(const QStringList &foreignTypesFiles)
{
    bool success = true;

    for (const QString &types : foreignTypesFiles) {
        QFile typesFile(types);
        if (!typesFile.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Cannot open foreign types file %s\n", qPrintable(types));
            success = false;
            continue;
        }

        QJsonParseError error = {0, QJsonParseError::NoError};
        QJsonDocument foreignMetaObjects = QJsonDocument::fromJson(typesFile.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            fprintf(stderr, "Error %d while parsing %s: %s\n", error.error, qPrintable(types),
                    qPrintable(error.errorString()));
            success = false;
            continue;
        }

        const QJsonArray foreignObjectsArray = foreignMetaObjects.array();
        for (const QJsonValue metaObject : foreignObjectsArray) {
            if (!metaObject.isObject()) {
                fprintf(stderr, "Error parsing %s: JSON is not an object\n",
                        qPrintable(types));
                success = false;
                continue;
            }

            processForeignTypes(metaObject.toObject());
        }
    }
    return success;
}

static void sortStringList(QStringList *list)
{
    std::sort(list->begin(), list->end());
    const auto newEnd = std::unique(list->begin(), list->end());
    list->erase(QStringList::const_iterator(newEnd), list->constEnd());
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
    sortTypes(m_types);
}

MetaTypesJsonProcessor::RegistrationMode MetaTypesJsonProcessor::qmlTypeRegistrationMode(
        const QJsonObject &classDef)
{
    const QJsonArray classInfos = classDef[QLatin1String("classInfos")].toArray();
    for (const QJsonValue info : classInfos) {
        const QString name = info[QLatin1String("name")].toString();
        if (name == QLatin1String("QML.Element")) {
            if (classDef[QLatin1String("object")].toBool())
                return ObjectRegistration;
            if (classDef[QLatin1String("gadget")].toBool())
                return GadgetRegistration;
            if (classDef[QLatin1String("namespace")].toBool())
                return NamespaceRegistration;
            qWarning() << "Not registering classInfo which is neither an object, "
                          "nor a gadget, nor a namespace:"
                       << name;
            break;
        }
    }
    return NoRegistration;
}

void MetaTypesJsonProcessor::addRelatedTypes()
{
    const QLatin1String classInfosKey("classInfos");
    const QLatin1String nameKey("name");
    const QLatin1String qualifiedClassNameKey("qualifiedClassName");
    const QLatin1String qmlNamePrefix("QML.");
    const QLatin1String qmlForeignName("QML.Foreign");
    const QLatin1String qmlExtendedName("QML.Extended");
    const QLatin1String qmlAttachedName("QML.Attached");
    const QLatin1String qmlSequenceName("QML.Sequence");
    const QLatin1String valueKey("value");
    const QLatin1String superClassesKey("superClasses");
    const QLatin1String accessKey("access");
    const QLatin1String publicAccess("public");

    QSet<QString> processedRelatedNames;
    QQueue<QJsonObject> typeQueue;
    typeQueue.append(m_types);

    // First mark all classes registered from this module as already processed.
    for (const QJsonObject &type : m_types) {
        processedRelatedNames.insert(type.value(qualifiedClassNameKey).toString());
        const auto classInfos = type.value(classInfosKey).toArray();
        for (const QJsonValue classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            if (obj.value(nameKey).toString() == qmlForeignName) {
                processedRelatedNames.insert(obj.value(valueKey).toString());
                break;
            }
        }
    }

    // Then mark all classes registered from other modules as already processed.
    // We don't want to generate them again for this module.
    for (const QJsonObject &foreignType : m_foreignTypes) {
        const auto classInfos = foreignType.value(classInfosKey).toArray();
        bool seenQmlPrefix = false;
        for (const QJsonValue classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            const QString name = obj.value(nameKey).toString();
            if (!seenQmlPrefix && name.startsWith(qmlNamePrefix)) {
                processedRelatedNames.insert(foreignType.value(qualifiedClassNameKey).toString());
                seenQmlPrefix = true;
            }
            if (name == qmlForeignName) {
                processedRelatedNames.insert(obj.value(valueKey).toString());
                break;
            }
        }
    }

    auto addType = [&](const QString &typeName) {
        m_referencedTypes.append(typeName);
        if (processedRelatedNames.contains(typeName))
            return;
        processedRelatedNames.insert(typeName);
        if (const QJsonObject *other
                = QmlTypesClassDescription::findType(m_foreignTypes, typeName)) {
            m_types.append(*other);
            typeQueue.enqueue(*other);
        }
    };

    // Then recursively iterate the super types and attached types, marking the
    // ones we are interested in as related.
    while (!typeQueue.isEmpty()) {
        const QJsonObject classDef = typeQueue.dequeue();

        const auto classInfos = classDef.value(classInfosKey).toArray();
        for (const QJsonValue classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            const QString objNameValue = obj.value(nameKey).toString();
            if (objNameValue == qmlAttachedName || objNameValue == qmlSequenceName
                    || objNameValue == qmlExtendedName) {
                addType(obj.value(valueKey).toString());
            } else if (objNameValue == qmlForeignName) {
                const QString foreignClassName = obj.value(valueKey).toString();
                if (const QJsonObject *other = QmlTypesClassDescription::findType(
                            m_foreignTypes, foreignClassName)) {
                    const auto otherSupers = other->value(superClassesKey).toArray();
                    if (!otherSupers.isEmpty()) {
                        const QJsonObject otherSuperObject = otherSupers.first().toObject();
                        if (otherSuperObject.value(accessKey).toString() == publicAccess)
                            addType(otherSuperObject.value(nameKey).toString());
                    }

                    const auto otherClassInfos = other->value(classInfosKey).toArray();
                    for (const QJsonValue otherClassInfo : otherClassInfos) {
                        const QJsonObject obj = otherClassInfo.toObject();
                        const QString objNameValue = obj.value(nameKey).toString();
                        if (objNameValue == qmlAttachedName || objNameValue == qmlSequenceName
                                || objNameValue == qmlExtendedName) {
                            addType(obj.value(valueKey).toString());
                            break;
                        }
                        // No, you cannot chain QML_FOREIGN declarations. Sorry.
                    }
                }
            }
        }

        const auto supers = classDef.value(superClassesKey).toArray();
        if (!supers.isEmpty()) {
            const QJsonObject superObject = supers.first().toObject();
            if (superObject.value(accessKey).toString() == publicAccess)
                addType(superObject.value(nameKey).toString());
        }
    }
}

void MetaTypesJsonProcessor::sortTypes(QVector<QJsonObject> &types)
{
    const QLatin1String qualifiedClassNameKey("qualifiedClassName");
    std::sort(types.begin(), types.end(), [&](const QJsonObject &a, const QJsonObject &b) {
        return a.value(qualifiedClassNameKey).toString() <
                b.value(qualifiedClassNameKey).toString();
    });
}

QString MetaTypesJsonProcessor::resolvedInclude(const QString &include)
{
    return (m_privateIncludes && include.endsWith(QLatin1String("_p.h")))
            ? QLatin1String("private/") + include
            : include;
}

void MetaTypesJsonProcessor::processTypes(const QJsonObject &types)
{
    const QString include = resolvedInclude(types[QLatin1String("inputFile")].toString());
    const QJsonArray classes = types[QLatin1String("classes")].toArray();
    for (const QJsonValue cls : classes) {
        QJsonObject classDef = cls.toObject();
        classDef.insert(QLatin1String("inputFile"), include);

        switch (qmlTypeRegistrationMode(classDef)) {
        case NamespaceRegistration:
        case GadgetRegistration:
        case ObjectRegistration: {
            if (!include.endsWith(QLatin1String(".h"))
                    && !include.endsWith(QLatin1String(".hpp"))
                    && !include.endsWith(QLatin1String(".hxx"))
                    && include.contains(QLatin1Char('.'))) {
                fprintf(stderr,
                        "Class %s is declared in %s, which appears not to be a header.\n"
                        "The compilation of its registration to QML may fail.\n",
                        qPrintable(classDef.value(QLatin1String("qualifiedClassName"))
                                   .toString()),
                        qPrintable(include));
            }
            m_includes.append(include);
            m_types.append(classDef);
            break;
        }
        case NoRegistration:
            m_foreignTypes.append(classDef);
            break;
        }
    }
}

void MetaTypesJsonProcessor::processForeignTypes(const QJsonObject &types)
{
    const QString include = resolvedInclude(types[QLatin1String("inputFile")].toString());
    const QJsonArray classes = types[QLatin1String("classes")].toArray();
    for (const QJsonValue cls : classes) {
        QJsonObject classDef = cls.toObject();
        classDef.insert(QLatin1String("inputFile"), include);
        m_foreignTypes.append(classDef);
    }
}
