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

static QString convertPrivateClassToUsableForm(QAnyStringView s)
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

    if (!collector.primitiveAliases.isEmpty())
        m_qml.writeStringListBinding(S_ALIASES, collector.primitiveAliases);

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

    if (m_generatingJSRoot)
        m_qml.writeBooleanBinding(S_IS_JAVASCRIPT_BUILTIN, true);

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
        m_qml.writeBooleanBinding(S_IS_STRUCTURED, true);

    if (collector.isSingleton)
        m_qml.writeBooleanBinding(S_IS_SINGLETON, true);

    if (collector.hasCustomParser)
        m_qml.writeBooleanBinding(S_HAS_CUSTOM_PARSER, true);

    if (collector.enforcesScopedEnums)
        m_qml.writeBooleanBinding(S_ENFORCES_SCOPED_ENUMS, true);

    m_qml.writeArrayBinding(S_EXPORT_META_OBJECT_REVISIONS, metaObjects);

    if (!collector.attachedType.isEmpty())
        m_qml.writeStringBinding(S_ATTACHED_TYPE, collector.attachedType);
}

void QmlTypesCreator::writeType(QAnyStringView type)
{
    ResolvedTypeAlias resolved(type, m_usingDeclarations);
    if (resolved.type.isEmpty())
        return;

    m_qml.writeStringBinding(S_TYPE, resolved.type);
    if (resolved.isList)
        m_qml.writeBooleanBinding(S_IS_LIST, true);
    if (resolved.isPointer)
        m_qml.writeBooleanBinding(S_IS_POINTER, true);
    if (resolved.isConstant)
        m_qml.writeBooleanBinding(S_IS_TYPE_CONSTANT, true);
}

void QmlTypesCreator::writeProperties(const Property::Container &properties)
{
    for (const Property &obj : properties) {
        const QAnyStringView name = obj.name;
        m_qml.writeStartObject(S_PROPERTY);
        m_qml.writeStringBinding(S_NAME, name);
        if (obj.revision.isValid())
            m_qml.writeNumberBinding(S_REVISION, obj.revision.toEncodedVersion<int>());

        writeType(obj.type);

        const auto bindable = obj.bindable;
        if (!bindable.isEmpty())
            m_qml.writeStringBinding(S_BINDABLE, bindable);
        const auto read = obj.read;
        if (!read.isEmpty())
            m_qml.writeStringBinding(S_READ, read);
        const auto write = obj.write;
        if (!write.isEmpty())
            m_qml.writeStringBinding(S_WRITE, write);
        const auto reset = obj.reset;
        if (!reset.isEmpty())
            m_qml.writeStringBinding(S_RESET, reset);
        const auto notify = obj.notify;
        if (!notify.isEmpty())
            m_qml.writeStringBinding(S_NOTIFY, notify);
        const auto index = obj.index;
        if (index != -1) {
            m_qml.writeNumberBinding(S_INDEX, index);
        }
        const auto privateClass = obj.privateClass;
        if (!privateClass.isEmpty()) {
            m_qml.writeStringBinding(
                    S_PRIVATE_CLASS, convertPrivateClassToUsableForm(privateClass));
        }

        if (obj.write.isEmpty() && obj.member.isEmpty())
            m_qml.writeBooleanBinding(S_IS_READONLY, true);

        if (obj.isFinal)
            m_qml.writeBooleanBinding(S_IS_FINAL, true);

        if (obj.isConstant)
            m_qml.writeBooleanBinding(S_IS_PROPERTY_CONSTANT, true);

        if (obj.isRequired)
            m_qml.writeBooleanBinding(S_IS_REQUIRED, true);

        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeMethods(const Method::Container &methods, QLatin1StringView type)
{
    for (const Method &obj : methods) {
        const QAnyStringView name = obj.name;
        if (name.isEmpty())
            continue;

        const auto revision = obj.revision;
        m_qml.writeStartObject(type);
        m_qml.writeStringBinding(S_NAME, name);
        if (revision.isValid())
            m_qml.writeNumberBinding(S_REVISION, revision.toEncodedVersion<int>());
        writeType(obj.returnType);

        if (obj.isCloned)
            m_qml.writeBooleanBinding(S_IS_CLONED, true);
        if (obj.isConstructor)
            m_qml.writeBooleanBinding(S_IS_CONSTRUCTOR, true);
        if (obj.isJavaScriptFunction)
            m_qml.writeBooleanBinding(S_IS_JAVASCRIPT_FUNCTION, true);
        if (obj.isConst)
            m_qml.writeBooleanBinding(S_IS_METHOD_CONSTANT, true);

        const Argument::Container &arguments = obj.arguments;
        for (qsizetype i = 0, end = arguments.size(); i != end; ++i) {
            const Argument &obj = arguments[i];
            m_qml.writeStartObject(S_PARAMETER);
            const QAnyStringView name = obj.name;
            if (!name.isEmpty())
                m_qml.writeStringBinding(S_NAME, name);
            writeType(obj.type);
            m_qml.writeEndObject();
        }
        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeEnums(const Enum::Container &enums)
{
    for (const Enum &obj : enums) {
        m_qml.writeStartObject(S_ENUM);
        m_qml.writeStringBinding(S_NAME, obj.name);
        if (!obj.alias.isEmpty())
            m_qml.writeStringBinding(S_ALIAS, obj.alias);
        if (obj.isFlag)
            m_qml.writeBooleanBinding(S_IS_FLAG, true);
        if (obj.isClass)
            m_qml.writeBooleanBinding(S_IS_SCOPED, true);
        writeType(obj.type);
        m_qml.writeStringListBinding(S_VALUES, obj.values);
        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeRootMethods(const MetaType &classDef)
{
    // Hide destroyed() signals
    Method::Container componentSignals = classDef.sigs();
    for (auto it = componentSignals.begin(); it != componentSignals.end();) {
        if (it->name == "destroyed"_L1)
            it = componentSignals.erase(it);
        else
            ++it;
    }
    writeMethods(componentSignals, S_SIGNAL);

    // Hide deleteLater() methods
    Method::Container componentMethods = classDef.methods();
    for (auto it = componentMethods.begin(); it != componentMethods.end();) {
        if (it->name == "deleteLater"_L1)
            it = componentMethods.erase(it);
        else
            ++it;
    }

    // Add toString()
    Method toStringMethod;
    toStringMethod.index = -2; // See QV4::QObjectMethod
    toStringMethod.name = "toString"_L1;
    toStringMethod.access = Access::Public;
    toStringMethod.returnType = "QString"_L1;
    toStringMethod.isConst = true;
    componentMethods.push_back(std::move(toStringMethod));

    // Add destroy(int)
    Method destroyMethodWithArgument;
    destroyMethodWithArgument.index = -1; // See QV4::QObjectMethod
    destroyMethodWithArgument.name = "destroy"_L1;
    destroyMethodWithArgument.access = Access::Public;
    Argument delayArgument;
    delayArgument.name = "delay"_L1;
    delayArgument.type = "int"_L1;
    destroyMethodWithArgument.arguments.push_back(std::move(delayArgument));
    componentMethods.push_back(std::move(destroyMethodWithArgument));

    // Add destroy()
    Method destroyMethod;
    destroyMethod.index = -1; // See QV4::QObjectMethod
    destroyMethod.name = "destroy"_L1;
    destroyMethod.access = Access::Public;
    destroyMethod.isCloned = true;
    componentMethods.push_back(std::move(destroyMethod));

    writeMethods(componentMethods, S_METHOD);
};

void QmlTypesCreator::writeComponent(const QmlTypesClassDescription &collector)
{
    m_qml.writeStartObject(S_COMPONENT);

    writeClassProperties(collector);

    if (const MetaType &classDef = collector.resolvedClass; !classDef.isEmpty()) {
        writeEnums(classDef.enums());
        writeProperties(classDef.properties());

        if (collector.isRootClass) {
            writeRootMethods(classDef);
        } else {
            writeMethods(classDef.sigs(), S_SIGNAL);
            writeMethods(classDef.methods(), S_METHOD);
        }

        writeMethods(classDef.constructors(), S_METHOD);
    }
    m_qml.writeEndObject();
}

void QmlTypesCreator::writeComponents()
{
    for (const MetaType &component : std::as_const(m_ownTypes)) {
        QmlTypesClassDescription collector;
        collector.collect(component, m_ownTypes, m_foreignTypes,
                          QmlTypesClassDescription::TopLevel, m_version);

        writeComponent(collector);

        if (collector.resolvedClass != component
                && std::binary_search(
                    m_referencedTypes.begin(), m_referencedTypes.end(),
                    component.qualifiedClassName())) {

            // This type is referenced from elsewhere and has a QML_FOREIGN of its own. We need to
            // also generate a description of the local type then. All the QML_* macros are
            // ignored, and the result is an anonymous type.

            QmlTypesClassDescription collector;
            collector.collectLocalAnonymous(component, m_ownTypes, m_foreignTypes, m_version);
            Q_ASSERT(!collector.isRootClass);

            writeComponent(collector);
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

