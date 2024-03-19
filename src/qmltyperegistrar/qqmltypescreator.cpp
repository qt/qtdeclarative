// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qanystringviewutils_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qqmltyperegistrarutils_p.h"
#include "qqmltypesclassdescription_p.h"
#include "qqmltypescreator_p.h"

#include <QtCore/qset.h>
#include <QtCore/qcborarray.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qsavefile.h>
#include <QtCore/qfile.h>
#include <QtCore/qversionnumber.h>

#include <QtCore/private/qstringalgorithms_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace Constants;
using namespace Constants::DotQmltypes;
using namespace QAnyStringViewUtils;

static QString convertPrivateClassToUsableForm(const QCborValue &s)
{
    // typical privateClass entry in MOC looks like: ClassName::d_func(), where
    // ClassName is a non-private class name. we don't need "::d_func()" piece
    // so that could be removed, but we need "Private" so that ClassName becomes
    // ClassNamePrivate (at present, simply consider this correct)
    return s.toString().replace("::d_func()"_L1, "Private"_L1);
}

void QmlTypesCreator::writeClassProperties(const QmlTypesClassDescription &collector)
{
    if (!collector.file.isEmpty())
        m_qml.writeStringBinding(S_FILE, collector.file);
    m_qml.writeStringBinding(S_NAME, collector.className);

    if (!collector.accessSemantics.isEmpty())
        m_qml.writeStringBinding(S_ACCESS_SEMANTICS, collector.accessSemantics);

    if (!collector.defaultProp.isEmpty())
        m_qml.writeStringBinding(S_DEFAULT_PROPERTY, collector.defaultProp);

    if (!collector.parentProp.isEmpty())
        m_qml.writeStringBinding(S_PARENT_PROPERTY, collector.parentProp);

    if (!collector.superClass.isEmpty())
        m_qml.writeStringBinding(S_PROTOTYPE, collector.superClass);

    if (!collector.sequenceValueType.isEmpty()) {
        const QAnyStringView name = collector.sequenceValueType.back() == '*'_L1
                ? collector.sequenceValueType.chopped(1)
                : collector.sequenceValueType;
        m_qml.writeStringBinding(S_VALUE_TYPE, name);
    }

    if (collector.extensionIsJavaScript) {
        if (!collector.javaScriptExtensionType.isEmpty()) {
            m_qml.writeStringBinding(S_EXTENSION, collector.javaScriptExtensionType);
            m_qml.writeBooleanBinding(S_EXTENSION_IS_JAVA_SCRIPT, true);
        } else {
            warning(collector.file)
                    << "JavaScript extension type for" << collector.className
                    << "does not exist";
        }

        if (collector.extensionIsNamespace) {
            warning(collector.file)
                    << "Extension type for" << collector.className
                    << "cannot be both a JavaScript type and a namespace";
            if (!collector.nativeExtensionType.isEmpty()) {
                m_qml.writeStringBinding(S_EXTENSION, collector.nativeExtensionType);
                m_qml.writeBooleanBinding(S_EXTENSION_IS_NAMESPACE, true);
            }
        }
    } else if (!collector.nativeExtensionType.isEmpty()) {
        m_qml.writeStringBinding(S_EXTENSION, collector.nativeExtensionType);
        if (collector.extensionIsNamespace)
            m_qml.writeBooleanBinding(S_EXTENSION_IS_NAMESPACE, true);
    } else if (collector.extensionIsNamespace) {
        warning(collector.file)
                << "Extension namespace for" << collector.className << "does not exist";
        m_qml.writeBooleanBinding(S_EXTENSION_IS_NAMESPACE, true);
    }

    if (!collector.implementsInterfaces.isEmpty())
        m_qml.writeStringListBinding(S_INTERFACES, collector.implementsInterfaces);

    if (!collector.deferredNames.isEmpty())
        m_qml.writeStringListBinding(S_DEFERRED_NAMES, collector.deferredNames);

    if (!collector.immediateNames.isEmpty())
        m_qml.writeStringListBinding(S_IMMEDIATE_NAMES, collector.immediateNames);

    if (collector.elementNames.isEmpty()) // e.g. if QML_ANONYMOUS
        return;

    if (!collector.sequenceValueType.isEmpty()) {
        warning(collector.file) << "Ignoring names of sequential container:";
        for (const QAnyStringView &name : std::as_const(collector.elementNames))
            warning(collector.file) << " - " << name.toString();
        warning(collector.file)
                << "Sequential containers are anonymous. Use QML_ANONYMOUS to register them.";
        return;
    }

    QByteArrayList exports;
    QByteArrayList metaObjects;

    for (auto it = collector.revisions.begin(), end = collector.revisions.end(); it != end; ++it) {
        const QTypeRevision revision = *it;
        if (revision < collector.addedInRevision)
            continue;
        if (collector.removedInRevision.isValid() && !(revision < collector.removedInRevision))
            break;
        if (revision.hasMajorVersion() && revision.majorVersion() > m_version.majorVersion())
            break;

        for (const QAnyStringView &elementName : std::as_const(collector.elementNames)) {
            QByteArray exportEntry = m_module + '/';

            elementName.visit([&](auto view) {
                processAsUtf8(view, [&](QByteArrayView view) { exportEntry.append(view); });
            });
            exportEntry += ' ' + QByteArray::number(revision.hasMajorVersion()
                                                            ? revision.majorVersion()
                                                            : m_version.majorVersion());
            exportEntry += '.' + QByteArray::number(revision.minorVersion());

            exports.append(exportEntry);
        }
        metaObjects.append(QByteArray::number(revision.toEncodedVersion<quint16>()));
    }

    QList<QAnyStringView> exportStrings;
    exportStrings.reserve(exports.length());
    for (const QByteArray &entry: exports)
        exportStrings.append(QUtf8StringView(entry));

    m_qml.writeStringListBinding(S_EXPORTS, exportStrings);

    if (!collector.isCreatable || collector.isSingleton)
        m_qml.writeBooleanBinding(S_IS_CREATABLE, false);

    if (collector.isStructured)
        m_qml.writeScriptBinding(QLatin1String("isStructured"), QLatin1String("true"));

    if (collector.isSingleton)
        m_qml.writeBooleanBinding(S_IS_SINGLETON, true);

    if (collector.hasCustomParser)
        m_qml.writeBooleanBinding(S_HAS_CUSTOM_PARSER, true);

    m_qml.writeArrayBinding(S_EXPORT_META_OBJECT_REVISIONS, metaObjects);

    if (!collector.attachedType.isEmpty())
        m_qml.writeStringBinding(S_ATTACHED_TYPE, collector.attachedType);
}

void QmlTypesCreator::writeType(const QCborMap &property, QLatin1StringView key)
{
    auto it = property.find(key);
    if (it == property.end())
        return;

    QAnyStringView type = toStringView(it.value());
    if (type.isEmpty() || type == "void")
        return;

    bool isList = false;
    bool isPointer = false;
    // This is a best effort approach (like isPointer) and will not return correct results in the
    // presence of typedefs.
    bool isConstant = false;

    auto handleList = [&](QAnyStringView list) {
        if (!startsWith(type, list) || type.back() != '>'_L1)
            return false;

        const int listSize = list.size();
        const QAnyStringView elementType = trimmed(type.mid(listSize, type.size() - listSize - 1));

        // QQmlListProperty internally constructs the pointer. Passing an explicit '*' will
        // produce double pointers. QList is only for value types. We can't handle QLists
        // of pointers (unless specially registered, but then they're not isList).
        if (elementType.back() == '*'_L1)
            return false;

        isList = true;
        type = elementType;
        return true;
    };

    if (!handleList("QQmlListProperty<"_L1) && !handleList("QList<"_L1)) {
        if (type.back() == '*'_L1) {
            isPointer = true;
            type = type.chopped(1);
        }
        if (startsWith(type, "const "_L1)) {
            isConstant = true;
            type = type.sliced(strlen("const "));
        }
    }

    if (type == "qreal") {
#ifdef QT_COORD_TYPE_STRING
        type = QT_COORD_TYPE_STRING;
#else
        type = "double";
#endif
    } else if (type == "int8_t") {
        // TODO: What can we do with "char"? It's ambiguous.
        type = "qint8";
    } else if (type == "uchar" || type == "uint8_t") {
        type = "quint8";
    } else if (type == "qint16" || type == "int16_t") {
        type = "short";
    } else if (type == "quint16" || type == "uint16_t") {
        type = "ushort";
    } else if (type == "qint32" || type == "int32_t") {
        type = "int";
    } else if (type == "quint32" || type == "uint32_t") {
        type = "uint";
    } else if (type == "qint64" || type == "int64_t") {
        type = "qlonglong";
    } else if (type == "quint64" || type == "uint64_t") {
        type = "qulonglong";
    } else if (type == "QList<QObject*>") {
        type = "QObjectList";
    }

    m_qml.writeStringBinding(S_TYPE, type);
    if (isList)
        m_qml.writeBooleanBinding(S_IS_LIST, true);
    if (isPointer)
        m_qml.writeBooleanBinding(S_IS_POINTER, true);
    if (isConstant)
        m_qml.writeBooleanBinding(S_IS_CONSTANT, true);
}

void QmlTypesCreator::writeProperties(const QCborArray &properties)
{
    for (const QCborValue &property : properties) {
        const QCborMap obj = property.toMap();
        const QAnyStringView name = toStringView(obj, MetatypesDotJson::S_NAME);
        m_qml.writeStartObject(S_PROPERTY);
        m_qml.writeStringBinding(S_NAME, name);
        const auto it = obj.find(MetatypesDotJson::S_REVISION);
        if (it != obj.end())
            m_qml.writeNumberBinding(S_REVISION, it.value().toInteger());

        writeType(obj, MetatypesDotJson::S_TYPE);

        const auto bindable = obj.constFind(MetatypesDotJson::S_BINDABLE);
        if (bindable != obj.constEnd())
            m_qml.writeStringBinding(S_BINDABLE, toStringView(bindable.value()));
        const auto read = obj.constFind(MetatypesDotJson::S_READ);
        if (read != obj.constEnd())
            m_qml.writeStringBinding(S_READ, toStringView(read.value()));
        const auto write = obj.constFind(MetatypesDotJson::S_WRITE);
        if (write != obj.constEnd())
            m_qml.writeStringBinding(S_WRITE, toStringView(write.value()));
        const auto reset = obj.constFind(MetatypesDotJson::S_RESET);
        if (reset != obj.constEnd())
            m_qml.writeStringBinding(S_RESET, toStringView(reset.value()));
        const auto notify = obj.constFind(MetatypesDotJson::S_NOTIFY);
        if (notify != obj.constEnd())
            m_qml.writeStringBinding(S_NOTIFY, toStringView(notify.value()));
        const auto index = obj.constFind(MetatypesDotJson::S_INDEX);
        if (index != obj.constEnd()) {
            m_qml.writeNumberBinding(S_INDEX, index.value().toInteger());
        }
        const auto privateClass = obj.constFind(MetatypesDotJson::S_PRIVATE_CLASS);
        if (privateClass != obj.constEnd()) {
            m_qml.writeStringBinding(
                    S_PRIVATE_CLASS, convertPrivateClassToUsableForm(privateClass.value()));
        }

        if (!obj.contains(MetatypesDotJson::S_WRITE) && !obj.contains(MetatypesDotJson::S_MEMBER))
            m_qml.writeBooleanBinding(S_IS_READONLY, true);

        const auto final = obj.constFind(MetatypesDotJson::S_FINAL);
        if (final != obj.constEnd() && final->toBool())
            m_qml.writeBooleanBinding(S_IS_FINAL, true);

        const auto constant = obj.constFind(MetatypesDotJson::S_CONSTANT);
        if (constant != obj.constEnd() && constant->toBool())
            m_qml.writeBooleanBinding(S_IS_CONSTANT, true);

        const auto required = obj.constFind(MetatypesDotJson::S_REQUIRED);
        if (required != obj.constEnd() && required->toBool())
            m_qml.writeBooleanBinding(S_IS_REQUIRED, true);

        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeMethods(const QCborArray &methods, QLatin1StringView type)
{
    const auto writeFlag
            = [this](QLatin1StringView key, QLatin1StringView name, const QCborMap &obj) {
        const auto flag = obj.find(key);
        if (flag != obj.constEnd() && flag->toBool())
            m_qml.writeBooleanBinding(name, true);
    };

    for (const QCborValue &method : methods) {
        const QCborMap obj = method.toMap();
        const QAnyStringView name = toStringView(obj, MetatypesDotJson::S_NAME);
        if (name.isEmpty())
            continue;
        const QCborArray arguments = obj[MetatypesDotJson::S_ARGUMENTS].toArray();
        const auto revision = obj.find(MetatypesDotJson::S_REVISION);
        m_qml.writeStartObject(type);
        m_qml.writeStringBinding(S_NAME, name);
        if (revision != obj.end())
            m_qml.writeNumberBinding(S_REVISION, revision.value().toInteger());
        writeType(obj, MetatypesDotJson::S_RETURN_TYPE);

        writeFlag(MetatypesDotJson::S_IS_CLONED, S_IS_CLONED, obj);
        writeFlag(MetatypesDotJson::S_IS_CONSTRUCTOR, S_IS_CONSTRUCTOR, obj);
        writeFlag(MetatypesDotJson::S_IS_JAVASCRIPT_FUNCTION, S_IS_JAVASCRIPT_FUNCTION, obj);

        for (qsizetype i = 0, end = arguments.size(); i != end; ++i) {
            const QCborMap obj = arguments[i].toMap();
            if (i == 0 && end == 1 && obj[MetatypesDotJson::S_TYPE] == QLatin1String("QQmlV4Function*")) {
                m_qml.writeBooleanBinding(S_IS_JAVASCRIPT_FUNCTION, true);
                break;
            }
            m_qml.writeStartObject(S_PARAMETER);
            const QAnyStringView name = toStringView(obj, MetatypesDotJson::S_NAME);
            if (!name.isEmpty())
                m_qml.writeStringBinding(S_NAME, name);
            writeType(obj, MetatypesDotJson::S_TYPE);
            m_qml.writeEndObject();
        }
        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeEnums(
        const QCborArray &enums, QmlTypesCreator::EnumClassesMode enumClassesMode)
{
    for (const QCborValue &item : enums) {
        const QCborMap obj = item.toMap();
        const QCborArray values = obj.value(MetatypesDotJson::S_VALUES).toArray();
        QList<QAnyStringView> valueList;

        for (const QCborValue &value : values)
            valueList.append(toStringView(value));

        m_qml.writeStartObject(S_ENUM);
        m_qml.writeStringBinding(S_NAME, toStringView(obj, MetatypesDotJson::S_NAME));
        auto alias = obj.find(MetatypesDotJson::S_ALIAS);
        if (alias != obj.end())
            m_qml.writeStringBinding(S_ALIAS, toStringView(alias.value()));
        auto isFlag = obj.find(MetatypesDotJson::S_IS_FLAG);
        if (isFlag != obj.end() && isFlag->toBool())
            m_qml.writeBooleanBinding(S_IS_FLAG, true);

        if (enumClassesMode == EnumClassesMode::Scoped) {
            const auto isClass = obj.find(MetatypesDotJson::S_IS_CLASS);
            if (isClass != obj.end() && isClass->toBool())
                m_qml.writeBooleanBinding(S_IS_SCOPED, true);
        }

        writeType(obj, MetatypesDotJson::S_TYPE);
        m_qml.writeStringListBinding(S_VALUES, valueList);
        m_qml.writeEndObject();
    }
}

static bool isAllowedInMajorVersion(const QCborValue &member, QTypeRevision maxMajorVersion)
{
    const auto memberObject = member.toMap();
    const auto it = memberObject.find(MetatypesDotJson::S_REVISION);
    if (it == memberObject.end())
        return true;

    const QTypeRevision memberRevision = QTypeRevision::fromEncodedVersion(it->toInteger());
    return !memberRevision.hasMajorVersion()
            || memberRevision.majorVersion() <= maxMajorVersion.majorVersion();
}

template<typename Postprocess>
QCborArray members(
        const QCborMap &classDef, QLatin1StringView key, QTypeRevision maxMajorVersion,
        Postprocess &&process)
{
    QCborArray classDefMembers;

    const QCborArray candidates = classDef.value(key).toArray();
    for (QCborValue member : candidates) {
        if (isAllowedInMajorVersion(member, maxMajorVersion))
            classDefMembers.append(process(std::move(member)));
    }

    return classDefMembers;
}

static QCborArray members(
        const QCborMap &classDef, QLatin1StringView key, QTypeRevision maxMajorVersion)
{
    return members(classDef, key, maxMajorVersion, [](QCborValue &&member) { return member; });
}

static QCborArray constructors(
        const QCborMap &classDef, QLatin1StringView key, QTypeRevision maxMajorVersion)
{
    return members(classDef, key, maxMajorVersion, [](QCborValue &&member) {
        QCborMap ctor = member.toMap();
        ctor[MetatypesDotJson::S_IS_CONSTRUCTOR] = true;
        return ctor;
    });
}

void QmlTypesCreator::writeRootMethods(const QCborMap &classDef)
{
    // Hide destroyed() signals
    QCborArray componentSignals = members(classDef, MetatypesDotJson::S_SIGNALS, m_version);
    for (auto it = componentSignals.begin(); it != componentSignals.end();) {
        if (toStringView(it->toMap(), MetatypesDotJson::S_NAME) == "destroyed"_L1)
            it = componentSignals.erase(it);
        else
            ++it;
    }
    writeMethods(componentSignals, S_SIGNAL);

    // Hide deleteLater() methods
    QCborArray componentMethods = members(classDef, MetatypesDotJson::S_METHODS, m_version)
            + members(classDef, MetatypesDotJson::S_SLOTS, m_version);
    for (auto it = componentMethods.begin(); it != componentMethods.end();) {
        if (toStringView(it->toMap(), MetatypesDotJson::S_NAME) == "deleteLater"_L1)
            it = componentMethods.erase(it);
        else
            ++it;
    }

    // Add toString()
    QCborMap toStringMethod;
    toStringMethod.insert(MetatypesDotJson::S_NAME, "toString"_L1);
    toStringMethod.insert(MetatypesDotJson::S_ACCESS, MetatypesDotJson::S_PUBLIC);
    toStringMethod.insert(MetatypesDotJson::S_RETURN_TYPE, "QString"_L1);
    componentMethods.append(toStringMethod);

    // Add destroy(int)
    QCborMap destroyMethodWithArgument;
    destroyMethodWithArgument.insert(MetatypesDotJson::S_NAME, "destroy"_L1);
    destroyMethodWithArgument.insert(MetatypesDotJson::S_ACCESS, MetatypesDotJson::S_PUBLIC);
    QCborMap delayArgument;
    delayArgument.insert(MetatypesDotJson::S_NAME, "delay"_L1);
    delayArgument.insert(MetatypesDotJson::S_TYPE, "int"_L1);
    QCborArray destroyArguments;
    destroyArguments.append(delayArgument);
    destroyMethodWithArgument.insert(MetatypesDotJson::S_ARGUMENTS, destroyArguments);
    componentMethods.append(destroyMethodWithArgument);

    // Add destroy()
    QCborMap destroyMethod;
    destroyMethod.insert(MetatypesDotJson::S_NAME, "destroy"_L1);
    destroyMethod.insert(MetatypesDotJson::S_ACCESS, MetatypesDotJson::S_PUBLIC);
    destroyMethod.insert(MetatypesDotJson::S_IS_CLONED, true);
    componentMethods.append(destroyMethod);

    writeMethods(componentMethods, S_METHOD);
};

void QmlTypesCreator::writeComponents()
{
    for (const QCborMap &component : std::as_const(m_ownTypes)) {
        QmlTypesClassDescription collector;
        collector.collect(component, m_ownTypes, m_foreignTypes,
                          QmlTypesClassDescription::TopLevel, m_version);

        m_qml.writeStartObject(S_COMPONENT);

        writeClassProperties(collector);

        if (const QCborMap &classDef = collector.resolvedClass; !classDef.isEmpty()) {
            writeEnums(
                    members(classDef, MetatypesDotJson::S_ENUMS, m_version),
                    collector.registerEnumClassesScoped
                            ? EnumClassesMode::Scoped
                            : EnumClassesMode::Unscoped);

            writeProperties(members(classDef, MetatypesDotJson::S_PROPERTIES, m_version));

            if (collector.isRootClass) {
                writeRootMethods(classDef);
            } else {
                writeMethods(members(classDef, MetatypesDotJson::S_SIGNALS, m_version), S_SIGNAL);
                writeMethods(members(classDef, MetatypesDotJson::S_SLOTS, m_version), S_METHOD);
                writeMethods(members(classDef, MetatypesDotJson::S_METHODS, m_version), S_METHOD);
            }

            writeMethods(constructors(classDef, MetatypesDotJson::S_CONSTRUCTORS, m_version),
                         S_METHOD);
        }
        m_qml.writeEndObject();

        if (collector.resolvedClass != component
                && std::binary_search(
                    m_referencedTypes.begin(), m_referencedTypes.end(),
                    toStringView(component, MetatypesDotJson::S_QUALIFIED_CLASS_NAME))) {

            // This type is referenced from elsewhere and has a QML_FOREIGN of its own. We need to
            // also generate a description of the local type then. All the QML_* macros are
            // ignored, and the result is an anonymous type.

            m_qml.writeStartObject(S_COMPONENT);

            QmlTypesClassDescription collector;
            collector.collectLocalAnonymous(component, m_ownTypes, m_foreignTypes, m_version);

            writeClassProperties(collector);
            writeEnums(
                    members(component, MetatypesDotJson::S_ENUMS, m_version),
                    collector.registerEnumClassesScoped
                            ? EnumClassesMode::Scoped
                            : EnumClassesMode::Unscoped);

            writeProperties(members(component, MetatypesDotJson::S_PROPERTIES, m_version));

            writeMethods(members(component, MetatypesDotJson::S_SIGNALS, m_version), S_SIGNAL);
            writeMethods(members(component, MetatypesDotJson::S_SLOTS, m_version), S_METHOD);
            writeMethods(members(component, MetatypesDotJson::S_METHODS, m_version), S_METHOD);
            writeMethods(constructors(component, MetatypesDotJson::S_CONSTRUCTORS, m_version),
                         S_METHOD);

            m_qml.writeEndObject();
        }
    }
}

bool QmlTypesCreator::generate(const QString &outFileName)
{
    m_qml.writeStartDocument();
    m_qml.writeLibraryImport("QtQuick.tooling", 1, 2);
    m_qml.write(
            "\n// This file describes the plugin-supplied types contained in the library."
            "\n// It is used for QML tooling purposes only."
            "\n//"
            "\n// This file was auto-generated by qmltyperegistrar.\n\n");
    m_qml.writeStartObject(S_MODULE);

    writeComponents();

    m_qml.writeEndObject();

    QSaveFile file(outFileName);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    if (file.write(m_output) != m_output.size())
        return false;

    return file.commit();
}

QT_END_NAMESPACE

