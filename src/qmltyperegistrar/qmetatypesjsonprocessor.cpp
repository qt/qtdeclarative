// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmetatypesjsonprocessor_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qqueue.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QStringList MetaTypesJsonProcessor::namespaces(const QJsonObject &classDef)
{
    const QString unqualified = classDef.value("className"_L1).toString();
    const QString qualified = classDef.value("qualifiedClassName"_L1).toString();
    QStringList namespaces;
    if (qualified != unqualified) {
        namespaces = qualified.split("::"_L1);
        Q_ASSERT(namespaces.last() == unqualified);
        namespaces.pop_back();
    }
    return namespaces;
}

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
}

QString MetaTypesJsonProcessor::extractRegisteredTypes() const
{
    QString registrationHelper;
    for (const auto &obj: m_types) {
        const QString className = obj[u"className"].toString();
        const QString foreignClassName = className+ u"Foreign";
        const auto classInfos = obj[u"classInfos"].toArray();
        QString qmlElement;
        QString qmlUncreatable;
        QString qmlAttached;
        bool isSingleton = false;
        bool isExplicitlyUncreatable = false;
        for (QJsonValue entry: classInfos) {
            const auto name = entry[u"name"].toString();
            const auto value = entry[u"value"].toString();
            if (name == u"QML.Element") {
                if (value == u"auto") {
                    qmlElement = u"QML_NAMED_ELEMENT("_s + className + u")"_s;
                } else if (value == u"anonymous") {
                    qmlElement = u"QML_ANONYMOUS"_s;
                } else {
                    qmlElement = u"QML_NAMED_ELEMENT(" + value + u")";
                }
            } else if (name == u"QML.Creatable" && value == u"false") {
                isExplicitlyUncreatable = true;
            } else if (name == u"QML.UncreatableReason") {
                qmlUncreatable = u"QML_UNCREATABLE(\"" + value + u"\")";
            } else if (name == u"QML.Attached") {
                qmlAttached = u"QML_ATTACHED("_s + value + u")";
            } else if (name == u"QML.Singleton") {
                isSingleton = true;
            }
        }
        if (qmlElement.isEmpty())
            continue; // no relevant entries found
        const QString spaces = u"    "_s;
        registrationHelper += u"\nstruct "_s + foreignClassName + u"{\n    Q_GADGET\n"_s;
        registrationHelper += spaces + u"QML_FOREIGN(" + className + u")\n"_s;
        registrationHelper += spaces + qmlElement + u"\n"_s;
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
        registrationHelper += u"};\n";
    }
    return registrationHelper;
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

static bool qualifiedClassNameLessThan(const QJsonObject &a, const QJsonObject &b)
{
    const QLatin1String qualifiedClassNameKey("qualifiedClassName");
    return a.value(qualifiedClassNameKey).toString() <
            b.value(qualifiedClassNameKey).toString();
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

    const auto addRelatedName = [&](const QString &relatedName, const QStringList &namespaces) {
        if (const QJsonObject *related = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, relatedName, namespaces)) {
            processedRelatedNames.insert(related->value(qualifiedClassNameKey).toString());
        }
    };

    // First mark all classes registered from this module as already processed.
    for (const QJsonObject &type : m_types) {
        processedRelatedNames.insert(type.value(qualifiedClassNameKey).toString());
        const auto classInfos = type.value(classInfosKey).toArray();
        for (const QJsonValue classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            if (obj.value(nameKey).toString() == qmlForeignName) {
                addRelatedName(obj.value(valueKey).toString(), namespaces(type));
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
                addRelatedName(obj.value(valueKey).toString(), namespaces(foreignType));
                break;
            }
        }
    }

    auto addType = [&](const QString &typeName, const QStringList &namespaces) {
        if (const QJsonObject *other = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, typeName, namespaces)) {
            const QString qualifiedName = other->value(qualifiedClassNameKey).toString();
            m_referencedTypes.append(qualifiedName);
            if (!processedRelatedNames.contains(qualifiedName)) {
                processedRelatedNames.insert(qualifiedName);
                m_types.insert(
                        std::lower_bound(
                                m_types.begin(), m_types.end(), *other, qualifiedClassNameLessThan),
                        *other);
                typeQueue.enqueue(*other);
            }
            return true;
        }
        processedRelatedNames.insert(typeName);
        return false;
    };

    // Then recursively iterate the super types and attached types, marking the
    // ones we are interested in as related.
    while (!typeQueue.isEmpty()) {
        const QJsonObject classDef = typeQueue.dequeue();
        const QStringList namespaces = MetaTypesJsonProcessor::namespaces(classDef);

        const auto classInfos = classDef.value(classInfosKey).toArray();
        for (const QJsonValue classInfo : classInfos) {
            const QJsonObject obj = classInfo.toObject();
            const QString objNameValue = obj.value(nameKey).toString();
            if (objNameValue == qmlAttachedName || objNameValue == qmlSequenceName
                    || objNameValue == qmlExtendedName) {
                addType(obj.value(valueKey).toString(), namespaces);
            } else if (objNameValue == qmlForeignName) {
                const QString foreignClassName = obj.value(valueKey).toString();
                if (const QJsonObject *other = QmlTypesClassDescription::findType(
                            m_foreignTypes, {}, foreignClassName, namespaces)) {
                    const auto otherSupers = other->value(superClassesKey).toArray();
                    const QStringList otherNamespaces = MetaTypesJsonProcessor::namespaces(*other);
                    if (!otherSupers.isEmpty()) {
                        const QJsonObject otherSuperObject = otherSupers.first().toObject();
                        if (otherSuperObject.value(accessKey).toString() == publicAccess)
                            addType(otherSuperObject.value(nameKey).toString(), otherNamespaces);
                    }

                    const auto otherClassInfos = other->value(classInfosKey).toArray();
                    for (const QJsonValue otherClassInfo : otherClassInfos) {
                        const QJsonObject obj = otherClassInfo.toObject();
                        const QString objNameValue = obj.value(nameKey).toString();
                        if (objNameValue == qmlAttachedName || objNameValue == qmlSequenceName
                                || objNameValue == qmlExtendedName) {
                            addType(obj.value(valueKey).toString(), otherNamespaces);
                            break;
                        }
                        // No, you cannot chain QML_FOREIGN declarations. Sorry.
                    }
                }
            }
        }

        const auto supers = classDef.value(superClassesKey).toArray();
        for (const QJsonValue super : supers) {
            const QJsonObject superObject = super.toObject();
            if (superObject.value(accessKey).toString() == publicAccess)
                addType(superObject.value(nameKey).toString(), namespaces);
        }
    }
}

void MetaTypesJsonProcessor::sortTypes(QVector<QJsonObject> &types)
{
    std::sort(types.begin(), types.end(), qualifiedClassNameLessThan);
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
                    && !include.endsWith(QLatin1String(".hh"))
                    && !include.endsWith(u".py")
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

QT_END_NAMESPACE
