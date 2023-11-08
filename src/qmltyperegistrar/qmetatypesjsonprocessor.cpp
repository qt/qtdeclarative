// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmetatypesjsonprocessor_p.h"

#include "qanystringviewutils_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qqmltypesclassdescription_p.h"

#include <QtCore/qcborarray.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qqueue.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace Constants;
using namespace Constants::MetatypesDotJson;
using namespace Constants::MetatypesDotJson::Qml;
using namespace QAnyStringViewUtils;

static QCborValue fromJson(const QByteArray &json, QJsonParseError *error)
{
    const QJsonDocument jsonValue = QJsonDocument::fromJson(json, error);
    if (jsonValue.isArray())
        return QCborValue::fromJsonValue(jsonValue.array());
    if (jsonValue.isObject())
        return QCborValue::fromJsonValue(jsonValue.object());
    return QCborValue();
}

QList<QAnyStringView> MetaTypesJsonProcessor::namespaces(const QCborMap &classDef)
{
    const QAnyStringView unqualified = toStringView(classDef, S_CLASS_NAME);
    const QAnyStringView qualified = toStringView(classDef, S_QUALIFIED_CLASS_NAME);

    QList<QAnyStringView> namespaces;
    if (qualified != unqualified) {
        namespaces = split(qualified, "::"_L1);
        Q_ASSERT(namespaces.last() == unqualified);
        namespaces.pop_back();
    }

    return namespaces;
}

bool MetaTypesJsonProcessor::processTypes(const QStringList &files)
{
    for (const QString &source: files) {
        QCborValue metaObjects;
        {
            QFile f(source);
            if (!f.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "Error opening %s for reading\n", qPrintable(source));
                return false;
            }
            QJsonParseError error = {0, QJsonParseError::NoError};
            metaObjects = fromJson(f.readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                fprintf(stderr, "Error %d while parsing %s: %s\n", error.error, qPrintable(source),
                        qPrintable(error.errorString()));
                return false;
            }
        }

        if (metaObjects.isArray()) {
            const QCborArray metaObjectsArray = metaObjects.toArray();
            for (const QCborValue &metaObject : metaObjectsArray) {
                if (!metaObject.isMap()) {
                    fprintf(stderr, "Error parsing %s: JSON is not an object\n",
                            qPrintable(source));
                    return false;
                }

                processTypes(metaObject.toMap());
            }
        } else if (metaObjects.isMap()) {
            processTypes(metaObjects.toMap());
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
        QCborValue foreignMetaObjects = fromJson(typesFile.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            fprintf(stderr, "Error %d while parsing %s: %s\n", error.error, qPrintable(types),
                    qPrintable(error.errorString()));
            success = false;
            continue;
        }

        const QCborArray foreignObjectsArray = foreignMetaObjects.toArray();
        for (const QCborValue &metaObject : foreignObjectsArray) {
            if (!metaObject.isMap()) {
                fprintf(stderr, "Error parsing %s: JSON is not an object\n",
                        qPrintable(types));
                success = false;
                continue;
            }

            processForeignTypes(metaObject.toMap());
        }
    }
    return success;
}

template<typename String>
static void sortStringList(QList<String> *list)
{
    std::sort(list->begin(), list->end());
    const auto newEnd = std::unique(list->begin(), list->end());
    list->erase(typename QList<String>::const_iterator(newEnd), list->constEnd());
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
        const QString className = obj[S_CLASS_NAME].toString();
        const QString foreignClassName = className + u"Foreign";
        const auto classInfos = obj[S_CLASS_INFOS].toArray();
        QStringList qmlElements;
        QString qmlUncreatable;
        QString qmlAttached;
        bool isSingleton = false;
        bool isExplicitlyUncreatable = false;
        for (const QCborValue &info: classInfos) {
            const QCborMap entry = info.toMap();
            const auto name = toStringView(entry, S_NAME);
            const auto value = toStringView(entry, S_VALUE);
            if (name == S_ELEMENT) {
                if (value == S_AUTO) {
                    qmlElements.append(u"QML_NAMED_ELEMENT("_s + className + u")"_s);
                } else if (value == S_ANONYMOUS) {
                    qmlElements.append(u"QML_ANONYMOUS"_s);
                } else {
                    qmlElements.append(u"QML_NAMED_ELEMENT("_s + value.toString() + u")");
                }
            } else if (name == S_CREATABLE && value == S_FALSE) {
                isExplicitlyUncreatable = true;
            } else if (name == S_UNCREATABLE_REASON) {
                qmlUncreatable = u"QML_UNCREATABLE(\""_s + value.toString() + u"\")";
            } else if (name == S_ATTACHED) {
                qmlAttached = u"QML_ATTACHED("_s + value.toString() + u")";
            } else if (name == S_SINGLETON) {
                isSingleton = true;
            }
        }
        if (qmlElements.isEmpty())
            continue; // no relevant entries found
        const QString spaces = u"    "_s;
        registrationHelper += u"\nstruct "_s + foreignClassName + u"{\n    Q_GADGET\n"_s;
        registrationHelper += spaces + u"QML_FOREIGN(" + className + u")\n"_s;
        registrationHelper += spaces + qmlElements.join(u"\n"_s) + u"\n"_s;
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
        const QCborMap &classDef)
{
    const QCborArray classInfos = classDef[S_CLASS_INFOS].toArray();
    for (const QCborValue &info : classInfos) {
        const QCborMap entry = info.toMap();
        const QAnyStringView name = toStringView(entry, S_NAME);
        if (name == S_ELEMENT) {
            if (classDef[S_OBJECT].toBool())
                return ObjectRegistration;
            if (classDef[S_GADGET].toBool())
                return GadgetRegistration;
            if (classDef[S_NAMESPACE].toBool())
                return NamespaceRegistration;
            qWarning() << "Not registering classInfo which is neither an object, "
                          "nor a gadget, nor a namespace:"
                       << name.toString();
            break;
        }
    }
    return NoRegistration;
}

// TODO: Remove this when QAnyStringView gets a proper qHash()
static size_t qHash(QAnyStringView string, size_t seed = 0)
{
    return string.visit([seed](auto view) {
        if constexpr (std::is_same_v<decltype(view), QStringView>)
            return qHash(view, seed);
        if constexpr (std::is_same_v<decltype(view), QLatin1StringView>)
            return qHash(view, seed);
        if constexpr (std::is_same_v<decltype(view), QUtf8StringView>)
            return qHash(QByteArrayView(view.data(), view.length()), seed);
    });
}

static bool qualifiedClassNameLessThan(const QCborMap &a, const QCborMap &b)
{
    return toStringView(a, S_QUALIFIED_CLASS_NAME) <
            toStringView(b, S_QUALIFIED_CLASS_NAME);
}

void MetaTypesJsonProcessor::addRelatedTypes()
{
    QSet<QAnyStringView> processedRelatedNames;
    QQueue<QCborMap> typeQueue;
    typeQueue.append(m_types);

    const auto addRelatedName
            = [&](QAnyStringView relatedName, const QList<QAnyStringView> &namespaces) {
        if (const QCborMap *related = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, relatedName, namespaces)) {
            processedRelatedNames.insert(toStringView(*related, S_QUALIFIED_CLASS_NAME));
        }
    };

    // First mark all classes registered from this module as already processed.
    for (const QCborMap &type : m_types) {
        processedRelatedNames.insert(toStringView(type, S_QUALIFIED_CLASS_NAME));
        const auto classInfos = type.value(S_CLASS_INFOS).toArray();
        for (const QCborValue &classInfo : classInfos) {
            const QCborMap obj = classInfo.toMap();
            if (obj.value(S_NAME) == S_FOREIGN) {
                addRelatedName(toStringView(obj, S_VALUE), namespaces(type));
                break;
            }
        }
    }

    // Then mark all classes registered from other modules as already processed.
    // We don't want to generate them again for this module.
    for (const QCborMap &foreignType : m_foreignTypes) {
        const auto classInfos = foreignType.value(S_CLASS_INFOS).toArray();
        bool seenQmlPrefix = false;
        for (const QCborValue &classInfo : classInfos) {
            const QCborMap obj = classInfo.toMap();
            const QAnyStringView name = toStringView(obj, S_NAME);
            if (!seenQmlPrefix && startsWith(name, "QML."_L1)) {
                processedRelatedNames.insert(
                        toStringView(foreignType, S_QUALIFIED_CLASS_NAME));
                seenQmlPrefix = true;
            }
            if (name == S_FOREIGN) {
                addRelatedName(toStringView(obj, S_VALUE), namespaces(foreignType));
                break;
            }
        }
    }

    auto addType = [&](QAnyStringView typeName, const QList<QAnyStringView> &namespaces) {
        if (const QCborMap *other = QmlTypesClassDescription::findType(
                    m_types, m_foreignTypes, typeName, namespaces)) {
            QAnyStringView qualifiedName = toStringView(*other, S_QUALIFIED_CLASS_NAME);
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
        const QCborMap classDef = typeQueue.dequeue();
        const QList<QAnyStringView> namespaces = MetaTypesJsonProcessor::namespaces(classDef);

        const auto classInfos = classDef.value(S_CLASS_INFOS).toArray();
        for (const QCborValue &classInfo : classInfos) {
            const QCborMap obj = classInfo.toMap();
            const QAnyStringView objNameValue = toStringView(obj, S_NAME);
            if (objNameValue == S_ATTACHED || objNameValue == S_SEQUENCE
                    || objNameValue == S_EXTENDED) {
                addType(toStringView(obj, S_VALUE), namespaces);
            } else if (objNameValue == S_FOREIGN) {
                const QAnyStringView foreignClassName = toStringView(obj, S_VALUE);
                if (const QCborMap *other = QmlTypesClassDescription::findType(
                            m_foreignTypes, {}, foreignClassName, namespaces)) {
                    const auto otherSupers = other->value(S_SUPER_CLASSES).toArray();
                    const QList<QAnyStringView> otherNamespaces
                            = MetaTypesJsonProcessor::namespaces(*other);
                    if (!otherSupers.isEmpty()) {
                        const QCborMap otherSuperObject = otherSupers.first().toMap();
                        if (otherSuperObject.value(S_ACCESS) == S_PUBLIC)
                            addType(toStringView(otherSuperObject, S_NAME), otherNamespaces);
                    }

                    const auto otherClassInfos = other->value(S_CLASS_INFOS).toArray();
                    for (const QCborValue &otherClassInfo : otherClassInfos) {
                        const QCborMap obj = otherClassInfo.toMap();
                        const QAnyStringView objNameValue = toStringView(obj, S_NAME);
                        if (objNameValue == S_ATTACHED || objNameValue == S_SEQUENCE
                                || objNameValue == S_EXTENDED) {
                            addType(toStringView(obj, S_VALUE), otherNamespaces);
                            break;
                        }
                        // No, you cannot chain S_FOREIGN declarations. Sorry.
                    }
                }
            }
        }

        const auto supers = classDef.value(S_SUPER_CLASSES).toArray();
        for (const QCborValue &super : supers) {
            const QCborMap superObject = super.toMap();
            if (superObject.value(S_ACCESS) == S_PUBLIC)
                addType(toStringView(superObject, S_NAME), namespaces);
        }
    }
}

void MetaTypesJsonProcessor::sortTypes(QVector<QCborMap> &types)
{
    std::sort(types.begin(), types.end(), qualifiedClassNameLessThan);
}

QString MetaTypesJsonProcessor::resolvedInclude(QAnyStringView include)
{
    return (m_privateIncludes && endsWith(include, "_p.h"_L1))
            ? QLatin1String("private/") + include.toString()
            : include.toString();
}

void MetaTypesJsonProcessor::processTypes(const QCborMap &types)
{
    const QString include = resolvedInclude(toStringView(types, S_INPUT_FILE));
    const QCborArray classes = types[S_CLASSES].toArray();
    for (const QCborValue &cls : classes) {
        QCborMap classDef = cls.toMap();
        classDef.insert(S_INPUT_FILE, include);

        switch (qmlTypeRegistrationMode(classDef)) {
        case NamespaceRegistration:
        case GadgetRegistration:
        case ObjectRegistration: {
            if (!endsWith(include, QLatin1String(".h"))
                    && !endsWith(include, QLatin1String(".hpp"))
                    && !endsWith(include, QLatin1String(".hxx"))
                    && !endsWith(include, QLatin1String(".hh"))
                    && !endsWith(include, QLatin1String(".py"))
                    && contains(include, QLatin1Char('.'))) {
                fprintf(stderr,
                        "Class %s is declared in %s, which appears not to be a header.\n"
                        "The compilation of its registration to QML may fail.\n",
                        qPrintable(classDef.value(S_QUALIFIED_CLASS_NAME).toString()),
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

void MetaTypesJsonProcessor::processForeignTypes(const QCborMap &types)
{
    const QString include = resolvedInclude(toStringView(types, S_INPUT_FILE));
    const QCborArray classes = types[S_CLASSES].toArray();
    for (const QCborValue &cls : classes) {
        QCborMap classDef = cls.toMap();
        classDef.insert(S_INPUT_FILE, include);
        m_foreignTypes.append(classDef);
    }
}

QT_END_NAMESPACE
