/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "prototype/codegenerator.h"
#include "prototype/qml2cppdefaultpasses.h"
#include "qmltcpropertyutils.h"
#include "qmltccodewriter.h"
#include "qmltccompilerpieces.h"

#include "qmltccompiler.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>
#include <QtCore/qregularexpression.h>

#include <QtCore/qloggingcategory.h>

#include <private/qqmljsutils_p.h>

#include <optional>
#include <utility>
#include <numeric>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcCodeGenerator, "qml.qmltc.compiler", QtWarningMsg);

static QString figureReturnType(const QQmlJSMetaMethod &m)
{
    const bool isVoidMethod =
            m.returnTypeName() == u"void" || m.methodType() == QQmlJSMetaMethod::Signal;
    Q_ASSERT(isVoidMethod || m.returnType());
    // TODO: should really be m.returnTypeName(). for now, avoid inconsistencies
    // due to QML/C++ name collisions and unique C++ name generation
    QString type;
    if (isVoidMethod) {
        type = u"void"_s;
    } else {
        type = m.returnType()->augmentedInternalName();
    }
    return type;
}

static QList<QmltcVariable>
compileMethodParameters(const QStringList &names,
                        const QList<QSharedPointer<const QQmlJSScope>> &types,
                        bool allowUnnamed = false)
{
    QList<QmltcVariable> paramList;
    const auto size = names.size();
    paramList.reserve(size);
    for (qsizetype i = 0; i < size; ++i) {
        Q_ASSERT(types[i]); // assume verified
        QString name = names[i];
        Q_ASSERT(allowUnnamed || !name.isEmpty()); // assume verified
        if (name.isEmpty() && allowUnnamed)
            name = u"unnamed_" + QString::number(i);
        paramList.emplaceBack(QmltcVariable { types[i]->augmentedInternalName(), name, QString() });
    }
    return paramList;
}

// returns an (absolute) index into a QV4::Function array of the compilation
// unit which is used by QQmlEngine's executeRuntimeFunction() and friends
static qsizetype relativeToAbsoluteRuntimeIndex(const QmlIR::Object *irObject, qsizetype relative)
{
    return irObject->runtimeFunctionIndices.at(relative);
}

// finds property for given scope and returns it together with the absolute
// property index in the property array of the corresponding QMetaObject.
static std::pair<QQmlJSMetaProperty, int> getMetaPropertyIndex(const QQmlJSScope::ConstPtr &scope,
                                                               const QString &propertyName)
{
    auto owner = QQmlJSScope::ownerOfProperty(scope, propertyName).scope;
    Q_ASSERT(owner);
    auto p = owner->ownProperty(propertyName);
    if (!p.isValid())
        return { p, -1 };
    int index = p.index();
    if (index < 0) // this property doesn't have index - comes from QML
        return { p, -1 };
    // owner is not included in absolute index
    for (QQmlJSScope::ConstPtr base = owner->baseType(); base; base = base->baseType())
        index += int(base->ownProperties().size());
    return { p, index };
}

struct QmlIrBindingCompare
{
private:
    using T = QmlIR::Binding;
    using I = typename QmlIR::PoolList<T>::Iterator;
    static QHash<uint, qsizetype> orderTable;

public:
    bool operator()(const I &x, const I &y) const
    {
        return orderTable[x->type()] < orderTable[y->type()];
    }
    bool operator()(const T &x, const T &y) const
    {
        return orderTable[x.type()] < orderTable[y.type()];
    }
};

QHash<uint, qsizetype> QmlIrBindingCompare::orderTable = {
    { QmlIR::Binding::Type_Invalid, 100 },
    // value assignments (and object bindings) are "equal"
    { QmlIR::Binding::Type_Boolean, 0 },
    { QmlIR::Binding::Type_Number, 0 },
    { QmlIR::Binding::Type_String, 0 },
    { QmlIR::Binding::Type_Object, 0 },
    { QmlIR::Binding::Type_Null, 0 },
    // translations are also "equal" between themselves, but come after
    // assignments
    { QmlIR::Binding::Type_Translation, 1 },
    { QmlIR::Binding::Type_TranslationById, 1 },
    // attached properties and group properties might contain assignments
    // inside, so come next
    { QmlIR::Binding::Type_AttachedProperty, 2 },
    { QmlIR::Binding::Type_GroupProperty, 2 },
    // JS bindings come last because they can use values from other categories
    { QmlIR::Binding::Type_Script, 3 },
};

QList<typename QmlIR::PoolList<QmlIR::Binding>::Iterator>
CodeGenerator::toOrderedSequence(typename QmlIR::PoolList<QmlIR::Binding>::Iterator first,
                                 typename QmlIR::PoolList<QmlIR::Binding>::Iterator last,
                                 qsizetype n)
{
    // bindings actually have to sorted so that e.g. value assignments come
    // before script bindings. this is important for the code generator as it
    // would just emit instructions one by one, regardless of the ordering
    using I = typename QmlIR::PoolList<QmlIR::Binding>::Iterator;
    QList<I> sorted;
    sorted.reserve(n);
    for (auto it = first; it != last; ++it)
        sorted << it;
    // NB: use stable sort for bindings, because this matters: relative order of
    // bindings must be preserved - this might affect UI
    std::stable_sort(sorted.begin(), sorted.end(),
                     QmlIrBindingCompare {}); // TODO: use stable_partition instead
    return sorted;
}

/* code generator helper functions: */
static QStringList generate_assignToSpecialAlias(const QQmlJSScope::ConstPtr &type,
                                                 const QString &propertyName,
                                                 const QQmlJSMetaProperty &p, const QString &value,
                                                 const QString &accessor)
{
    Q_UNUSED(type);
    Q_UNUSED(propertyName);

    Q_ASSERT(p.isValid());
    Q_ASSERT(!p.isList()); // NB: this code does not handle list properties
    Q_ASSERT(p.isAlias());
    Q_ASSERT(p.type());

    QStringList code;
    code.reserve(6); // should always be enough
    // pretend there's a WRITE function
    QmltcPropertyData data(p);
    auto [prologue, wrappedValue, epilogue] =
            QmltcCodeGenerator::wrap_mismatchingTypeConversion(p, value);
    code += prologue;
    code << QmltcCodeGenerator::wrap_privateClass(accessor, p) + u"->" + data.write + u"("
                    + wrappedValue + u");";
    code += epilogue;
    return code;
}

// magic variable, necessary for correct handling of object bindings: since
// object creation and binding setup are separated across functions, we
// fetch a subobject by: QObject::children().at(offset + localIndex)
//                                              ^
//                                              childrenOffsetVariable
// this variable would often be equal to 0, but there's no guarantee. and it
// is required due to non-trivial aliases dependencies: aliases can
// reference any object in the document by id, which automatically means
// that all ids have to be set up before we get to finalization (and the
// only place for it is init)
// NB: this variable would behave correctly as long as QML init and QML finalize
// are non-virtual functions
static const QmltcVariable childrenOffsetVariable { u"qsizetype"_s, u"QML_choffset"_s,
                                                    QString() };

// represents QV4::ExecutableCompilationUnit
static const QmltcVariable compilationUnitVariable { u"QV4::ExecutableCompilationUnit *"_s,
                                                     u"QML_cu"_s, QString() };

Q_LOGGING_CATEGORY(lcCodeGen, "qml.compiler.CodeGenerator", QtWarningMsg);

CodeGenerator::CodeGenerator(const QString &url, QQmlJSLogger *logger, QmlIR::Document *doc,
                             const QmltcTypeResolver *localResolver, const QmltcVisitor *visitor,
                             const QmltcCompilerInfo *info)
    : m_url(url),
      m_logger(logger),
      m_doc(doc),
      m_localTypeResolver(localResolver),
      m_visitor(visitor),
      m_info(info)
{
    Q_ASSERT(m_info);
    Q_ASSERT(!m_info->outputHFile.isEmpty());
    Q_ASSERT(!m_info->outputCppFile.isEmpty());
}

void CodeGenerator::constructObjects(QSet<QString> &requiredCppIncludes)
{
    const auto &objects = m_doc->objects;
    m_objects.reserve(objects.size());

    m_typeToObjectIndex.reserve(objects.size());

    for (qsizetype objectIndex = 0; objectIndex != objects.size(); ++objectIndex) {
        QmlIR::Object *irObject = objects[objectIndex];
        if (!irObject) {
            recordError(QQmlJS::SourceLocation {},
                        u"Internal compiler error: IR object is null"_s);
            return;
        }
        QQmlJSScope::Ptr object = m_localTypeResolver->scopeForLocation(irObject->location);
        if (!object) {
            recordError(irObject->location, u"Object of unknown type"_s);
            return;
        }
        m_typeToObjectIndex.insert(object, objectIndex);
        m_objects.emplaceBack(CodeGenObject { irObject, object });
    }

    // objects are constructed, now we can run compiler passes to make sure the
    // objects are in good state
    Qml2CppCompilerPassExecutor executor(m_doc, m_localTypeResolver, m_url, m_objects,
                                         m_typeToObjectIndex);
    executor.addPass(&verifyTypes);
    executor.addPass(&checkForNamingCollisionsWithCpp);
    executor.addPass([this](const Qml2CppContext &context, QList<Qml2CppObject> &objects) {
        m_typeCounts = makeUniqueCppNames(context, objects);
    });
    const auto setupQmlBaseTypes = [&](const Qml2CppContext &context,
                                       QList<Qml2CppObject> &objects) {
        m_qmlCompiledBaseTypes = setupQmlCppTypes(context, objects);
    };
    executor.addPass(setupQmlBaseTypes);
    const auto resolveAliases = [&](const Qml2CppContext &context, QList<Qml2CppObject> &objects) {
        m_aliasesToIds = deferredResolveValidateAliases(context, objects);
    };
    executor.addPass(resolveAliases);
    const auto populateCppIncludes = [&](const Qml2CppContext &context,
                                         QList<Qml2CppObject> &objects) {
        requiredCppIncludes = findCppIncludes(context, objects);
    };
    executor.addPass(populateCppIncludes);
    const auto resolveExplicitComponents = [&](const Qml2CppContext &context,
                                               QList<Qml2CppObject> &objects) {
        m_componentIndices.insert(findAndResolveExplicitComponents(context, objects));
    };
    const auto resolveImplicitComponents = [&](const Qml2CppContext &context,
                                               QList<Qml2CppObject> &objects) {
        m_componentIndices.insert(findAndResolveImplicitComponents(context, objects));
    };
    executor.addPass(resolveExplicitComponents);
    executor.addPass(resolveImplicitComponents);
    executor.addPass(&setObjectIds); // NB: must be after Component resolution
    const auto setImmediateParents = [&](const Qml2CppContext &context,
                                         QList<Qml2CppObject> &objects) {
        m_immediateParents = findImmediateParents(context, objects);
    };
    executor.addPass(setImmediateParents);
    const auto setIgnoredTypes = [&](const Qml2CppContext &context, QList<Qml2CppObject> &objects) {
        m_ignoredTypes = collectIgnoredTypes(context, objects);
    };
    executor.addPass(setIgnoredTypes);

    // run all passes:
    executor.run(m_logger);
}

void CodeGenerator::prepare(QSet<QString> *cppIncludes)
{
    const QString rootClassName = QFileInfo(m_url).baseName();
    Q_ASSERT(!rootClassName.isEmpty());
    m_isAnonymous = rootClassName.at(0).isLower();

    constructObjects(*cppIncludes); // this populates all the codegen objects
}

bool CodeGenerator::ignoreObject(const CodeGenObject &object) const
{
    if (m_ignoredTypes.contains(object.type)) {
        // e.g. object.type is a view delegate
        qCDebug(lcCodeGenerator) << u"Scope '" + object.type->internalName()
                        + u"' is a QQmlComponent sub-component. It won't be compiled to "
                          u"C++.";
        return true;
    }
    return false;
}

static QString generate_callCompilationUnit(const QString &urlMethodName)
{
    // NB: assume `engine` variable always exists
    return u"QQmlEnginePrivate::get(engine)->compilationUnitFromUrl(%1())"_s.arg(urlMethodName);
}

void CodeGenerator::compileBinding(QmltcType &current, const QmlIR::Binding &binding,
                                   const CodeGenObject &object,
                                   const CodeGenerator::AccessorData &accessor)
{
    // TODO: cache property name somehow, so we don't need to look it up again
    QString propertyName = m_doc->stringAt(binding.propertyNameIndex);
    if (propertyName.isEmpty()) {
        // if empty, try default property
        for (QQmlJSScope::ConstPtr t = object.type->baseType(); t && propertyName.isEmpty();
             t = t->baseType())
            propertyName = t->defaultPropertyName();
    }
    Q_ASSERT(!propertyName.isEmpty());
    QQmlJSMetaProperty p = object.type->property(propertyName);
    QQmlJSScope::ConstPtr propertyType = p.type();
    // Q_ASSERT(propertyType); // TODO: doesn't work with signals

    // Note: unlike QQmlObjectCreator, we don't have to do a complicated
    // deferral logic for bindings: if a binding is deferred, it is not compiled
    // (potentially, with all the bindings inside of it), period.
    if (object.type->isNameDeferred(propertyName)) {
        if (binding.type() == QmlIR::Binding::Type_GroupProperty) {
            // TODO: we should warn about this in QmlCompiler library
            qCWarning(lcCodeGenerator)
                    << QStringLiteral("Binding at line %1 column %2 is not deferred as it is a "
                                      "binding on a group property.")
                               .arg(QString::number(binding.location.line()),
                                    QString::number(binding.location.column()));
            // we do not support PropertyChanges and other types with similar
            // behavior yet, so this binding is compiled
        } else {
            qCDebug(lcCodeGenerator)
                    << QStringLiteral(
                               "Binding at line %1 column %2 is deferred and thus not compiled")
                               .arg(QString::number(binding.location.line()),
                                    QString::number(binding.location.column()));
            return;
        }
    }

    const auto addPropertyLine = [&](const QString &propertyName, const QQmlJSMetaProperty &p,
                                     const QString &value, bool constructFromQObject = false) {
        // TODO: there mustn't be this special case. instead, alias resolution
        // must be done in QQmlJSImportVisitor subclass, that would handle this
        // mess (see resolveValidateOrSkipAlias() in qml2cppdefaultpasses.cpp)
        if (p.isAlias() && m_qmlCompiledBaseTypes.contains(object.type->baseTypeName())) {
            qCDebug(lcCodeGenerator) << u"Property '" + propertyName + u"' is an alias on type '"
                            + object.type->internalName()
                            + u"' which is a QML type compiled to C++. The assignment is special"
                            + u"in this case";
            current.endInit.body += generate_assignToSpecialAlias(object.type, propertyName, p,
                                                                  value, accessor.name);
        } else {
            QmltcCodeGenerator::generate_assignToProperty(&current.endInit.body, object.type, p,
                                                          value, accessor.name,
                                                          constructFromQObject);
        }
    };

    switch (binding.type()) {
    case QmlIR::Binding::Type_Boolean: {
        addPropertyLine(propertyName, p, binding.value.b ? u"true"_s : u"false"_s);
        break;
    }
    case QmlIR::Binding::Type_Number: {
        QString value = m_doc->javaScriptCompilationUnit.bindingValueAsString(&binding);
        addPropertyLine(propertyName, p, value);
        break;
    }
    case QmlIR::Binding::Type_String: {
        const QString str = m_doc->stringAt(binding.stringIndex);
        addPropertyLine(propertyName, p, QQmlJSUtils::toLiteral(str));
        break;
    }
    case QmlIR::Binding::Type_TranslationById: { // TODO: add test
        const QV4::CompiledData::TranslationData &translation =
                m_doc->javaScriptCompilationUnit.unitData()
                        ->translations()[binding.value.translationDataIndex];
        const QString id = m_doc->stringAt(binding.stringIndex);
        addPropertyLine(propertyName, p,
                        u"qsTrId(\"" + id + u"\", " + QString::number(translation.number) + u")");
        break;
    }
    case QmlIR::Binding::Type_Translation: { // TODO: add test
        const QV4::CompiledData::TranslationData &translation =
                m_doc->javaScriptCompilationUnit.unitData()
                        ->translations()[binding.value.translationDataIndex];
        int lastSlash = m_url.lastIndexOf(QChar(u'/'));
        const QStringView context = (lastSlash > -1)
                ? QStringView { m_url }.mid(lastSlash + 1, m_url.length() - lastSlash - 5)
                : QStringView();
        const QString comment = m_doc->stringAt(translation.commentIndex);
        const QString text = m_doc->stringAt(translation.stringIndex);

        addPropertyLine(propertyName, p,
                        u"QCoreApplication::translate(\"" + context + u"\", \"" + text + u"\", \""
                                + comment + u"\", " + QString::number(translation.number) + u")");
        break;
    }
    case QmlIR::Binding::Type_Script: {
        QString bindingSymbolName = object.type->internalName() + u'_' + propertyName + u"_binding";
        bindingSymbolName.replace(u'.', u'_'); // can happen with group properties
        compileScriptBinding(current, binding, bindingSymbolName, object, propertyName,
                             propertyType, accessor);
        break;
    }
    case QmlIR::Binding::Type_Object: {
        // NB: object is compiled with compileObject, here just need to use it
        const CodeGenObject &bindingObject = m_objects.at(binding.value.objectIndex);

        // Note: despite a binding being set for `accessor`, we use "this" as a
        // parent of a created object. Both attached and grouped properties are
        // parented by "this", so lifetime-wise we should be fine. If not, we're
        // in a fairly deep trouble, actually, since it's essential to use
        // `this` (at least at present it seems to be so) - e.g. the whole logic
        // of separating object binding into object creation and object binding
        // setup relies on the fact that we use `this`
        const QString qobjectParent = u"this"_s;
        const QString objectAddr = accessor.name;

        if (binding.hasFlag(QmlIR::Binding::IsOnAssignment)) {
            const QString onAssignmentName = u"onAssign_" + propertyName;
            const auto uniqueId = UniqueStringId(current, onAssignmentName);
            if (!m_onAssignmentObjectsCreated.contains(uniqueId)) {
                m_onAssignmentObjectsCreated.insert(uniqueId);
                current.init.body << u"auto %1 = new %2(creator, engine, %3);"_s.arg(
                        onAssignmentName, bindingObject.type->internalName(), qobjectParent);
                current.init.body << u"creator->set(%1, %2);"_s.arg(
                        QString::number(m_visitor->creationIndex(bindingObject.type)),
                        onAssignmentName);

                // static_cast is fine, because we (must) know the exact type
                current.endInit.body << u"auto %1 = creator->get<%2>(%3);"_s.arg(
                        onAssignmentName, bindingObject.type->internalName(),
                        QString::number(m_visitor->creationIndex(bindingObject.type)));

                m_localChildrenToEndInit.append(onAssignmentName);
                m_localChildrenToFinalize.append(bindingObject.type);
            }

            // NB: we expect one "on" assignment per property, so creating
            // QQmlProperty each time should be fine (unlike QQmlListReference)
            current.endInit.body << u"{"_s;
            current.endInit.body << u"QQmlProperty qmlprop(" + objectAddr + u", QStringLiteral(\""
                            + propertyName + u"\"));";
            current.endInit.body << u"QT_PREPEND_NAMESPACE(QQmlCppOnAssignmentHelper)::set("
                            + onAssignmentName + u", qmlprop);";
            current.endInit.body << u"}"_s;
            break;
        }

        if (!propertyType) {
            recordError(binding.location,
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        if (p.isList() || binding.hasFlag(QmlIR::Binding::IsListItem)) {
            const QString refName = u"listref_" + propertyName;
            const auto uniqueId = UniqueStringId(current, refName);
            if (!m_listReferencesCreated.contains(uniqueId)) {
                m_listReferencesCreated.insert(uniqueId);
                // TODO: figure if Unicode support is needed here
                current.endInit.body << u"QQmlListReference " + refName + u"(" + objectAddr + u", "
                                + QQmlJSUtils::toLiteral(propertyName, u"QByteArrayLiteral")
                                + u");";
                current.endInit.body << u"Q_ASSERT(" + refName + u".canAppend());";
            }
        }

        const auto setObjectBinding = [&](const QString &value) {
            if (p.isList() || binding.hasFlag(QmlIR::Binding::IsListItem)) {
                const QString refName = u"listref_" + propertyName;
                current.endInit.body << refName + u".append(" + value + u");";
            } else {
                addPropertyLine(propertyName, p, value, /* fromQObject = */ true);
            }
        };

        if (bindingObject.irObject->flags & QV4::CompiledData::Object::IsComponent) {
            // NB: this one is special - and probably becomes a view delegate.
            // object binding separation also does not apply here
            const QString objectName = makeGensym(u"sc"_s);
            Q_ASSERT(m_typeToObjectIndex.contains(bindingObject.type));
            const int objectIndex = int(m_typeToObjectIndex[bindingObject.type]);
            Q_ASSERT(m_componentIndices.contains(objectIndex));
            const int index = m_componentIndices[objectIndex];
            current.endInit.body << u"{"_s;
            current.endInit.body << QStringLiteral(
                                            "auto thisContext = QQmlData::get(%1)->outerContext;")
                                            .arg(qobjectParent);
            current.endInit.body << QStringLiteral(
                                            "auto %1 = QQmlObjectCreator::createComponent(engine, "
                                            "%2, %3, %4, thisContext);")
                                            .arg(objectName,
                                                 generate_callCompilationUnit(m_urlMethodName),
                                                 QString::number(index), qobjectParent);
            current.endInit.body << QStringLiteral("thisContext->installContext(QQmlData::get(%1), "
                                                   "QQmlContextData::OrdinaryObject);")
                                            .arg(objectName);
            const auto isComponentBased = [](const QQmlJSScope::ConstPtr &type) {
                auto base = QQmlJSScope::nonCompositeBaseType(type);
                return base && base->internalName() == u"QQmlComponent"_s;
            };
            if (!m_doc->stringAt(bindingObject.irObject->idNameIndex).isEmpty()
                && isComponentBased(bindingObject.type)) {

                QmltcCodeGenerator::generate_setIdValue(
                        &current.endInit.body, u"thisContext"_s, bindingObject.irObject->id,
                        objectName, m_doc->stringAt(bindingObject.irObject->idNameIndex));
            }
            setObjectBinding(objectName);
            current.endInit.body << u"}"_s;
            break;
        }

        const QString objectName = makeGensym(u"o"_s);
        current.init.body << u"auto %1 = new %2(creator, engine, %3);"_s.arg(
                objectName, bindingObject.type->internalName(), qobjectParent);
        current.init.body << u"creator->set(%1, %2);"_s.arg(
                QString::number(m_visitor->creationIndex(bindingObject.type)), objectName);

        // refetch the same object during endInit to set the bindings
        current.endInit.body << u"auto %1 = creator->get<%2>(%3);"_s.arg(
                objectName, bindingObject.type->internalName(),
                QString::number(m_visitor->creationIndex(bindingObject.type)));
        setObjectBinding(objectName);

        m_localChildrenToEndInit.append(objectName);
        m_localChildrenToFinalize.append(bindingObject.type);
        break;
    }
    case QmlIR::Binding::Type_AttachedProperty: {
        Q_ASSERT(accessor.name == u"this"_s); // TODO: doesn't have to hold, in fact
        const auto attachedObject = m_objects.at(binding.value.objectIndex);
        const auto &irObject = attachedObject.irObject;
        const auto &type = attachedObject.type;
        Q_ASSERT(irObject && type);
        if (propertyName == u"Component"_s) {
            // TODO: there's a special QQmlComponentAttached, which has to be
            // called? c.f. qqmlobjectcreator.cpp's finalize()
            const auto compileComponent = [&](const QmlIR::Binding &b) {
                Q_ASSERT(b.type() == QmlIR::Binding::Type_Script);
                compileScriptBindingOfComponent(current, irObject, type, b,
                                                m_doc->stringAt(b.propertyNameIndex));
            };
            std::for_each(irObject->bindingsBegin(), irObject->bindingsEnd(), compileComponent);
        } else {
            const QString attachingTypeName = propertyName; // acts as an identifier
            auto attachingType = m_localTypeResolver->typeForName(attachingTypeName);
            Q_ASSERT(attachingType); // an error somewhere else

            QString attachedTypeName = type->attachedTypeName(); // TODO: check if == internalName?
            if (attachedTypeName.isEmpty()) // TODO: shouldn't happen ideally
                attachedTypeName = type->baseTypeName();
            const QString attachedMemberName = u"m_" + attachingTypeName;
            const auto uniqueId = UniqueStringId(current, attachedMemberName);
            // add attached type as a member variable to allow noop lookup
            if (!m_attachedTypesAlreadyRegistered.contains(uniqueId)) {
                m_attachedTypesAlreadyRegistered.insert(uniqueId);
                current.variables.emplaceBack(attachedTypeName + u" *", attachedMemberName,
                                              u"nullptr"_s);
                // Note: getting attached property is fairly expensive
                const QString getAttachedPropertyLine = u"qobject_cast<" + attachedTypeName
                        + u" *>(qmlAttachedPropertiesObject<" + attachingType->internalName()
                        + u">(this, /* create = */ true))";
                current.endInit.body
                        << attachedMemberName + u" = " + getAttachedPropertyLine + u";";
            }

            // compile bindings of the attached property
            auto sortedBindings = toOrderedSequence(
                    irObject->bindingsBegin(), irObject->bindingsEnd(), irObject->bindingCount());
            for (auto it : qAsConst(sortedBindings)) {
                compileBinding(current, *it, attachedObject,
                               { object.type, attachedMemberName, propertyName, false });
            }
        }
        break;
    }
    case QmlIR::Binding::Type_GroupProperty: {
        Q_ASSERT(accessor.name == u"this"_s); // TODO: doesn't have to hold, in fact
        if (p.read().isEmpty()) {
            recordError(binding.location,
                        u"READ function of group property '" + propertyName + u"' is unknown");
            return;
        }

        const auto groupedObject = m_objects.at(binding.value.objectIndex);
        const auto &irObject = groupedObject.irObject;
        const auto &type = groupedObject.type;
        Q_ASSERT(irObject && type);

        const bool isValueType =
                propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Value;
        if (!isValueType
            && propertyType->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            recordError(binding.location,
                        u"Group property '" + propertyName + u"' has unsupported access semantics");
            return;
        }

        QString groupAccessor =
                QmltcCodeGenerator::wrap_privateClass(accessor.name, p) + u"->" + p.read() + u"()";
        // NB: used when isValueType == true
        QString groupPropertyVarName = accessor.name + u"_group_" + propertyName;

        const auto isGroupAffectingBinding = [](const QmlIR::Binding &b) {
            // script bindings do not require value type group property variable
            // to actually be present.
            switch (b.type()) {
            case QmlIR::Binding::Type_Invalid:
            case QmlIR::Binding::Type_Script:
                return false;
            default:
                return true;
            }
        };
        const bool generateValueTypeCode = isValueType
                && std::any_of(irObject->bindingsBegin(), irObject->bindingsEnd(),
                               isGroupAffectingBinding);

        // value types are special
        if (generateValueTypeCode) {
            if (p.write().isEmpty()) { // just reject this
                recordError(binding.location,
                            u"Group property '" + propertyName
                                    + u"' is a value type without a setter");
                return;
            }

            current.endInit.body << u"auto " + groupPropertyVarName + u" = " + groupAccessor + u";";
            // TODO: addressof operator is a crutch to make the binding logic
            // work, which expects that `accessor.name` is a pointer type.
            groupAccessor = QmltcCodeGenerator::wrap_addressof(groupPropertyVarName);
        }

        // compile bindings of the grouped property
        auto sortedBindings = toOrderedSequence(irObject->bindingsBegin(), irObject->bindingsEnd(),
                                                irObject->bindingCount());
        const auto compile = [&](typename QmlIR::PoolList<QmlIR::Binding>::Iterator it) {
            compileBinding(current, *it, groupedObject,
                           { object.type, groupAccessor, propertyName, isValueType });
        };

        // NB: can't use lower_bound since it only accepts a value, not a unary
        // predicate
        auto scriptBindingsBegin =
                std::find_if(sortedBindings.cbegin(), sortedBindings.cend(),
                             [](auto it) { return it->type() == QmlIR::Binding::Type_Script; });
        auto it = sortedBindings.cbegin();
        for (; it != scriptBindingsBegin; ++it) {
            Q_ASSERT((*it)->type() != QmlIR::Binding::Type_Script);
            compile(*it);
        }

        // NB: script bindings are special on group properties. if our group is
        // a value type, the binding would be installed on the *object* that
        // holds the value type and not on the value type itself. this may cause
        // subtle side issues (esp. when script binding is actually a simple
        // enum value assignment - which is not recognized specially):
        //
        // auto valueTypeGroupProperty = getCopy();
        // installBinding(valueTypeGroupProperty, "subproperty1"); // changes subproperty1 value
        // setCopy(valueTypeGroupProperty); // oops, subproperty1 value changed to old again
        if (generateValueTypeCode) { // write the value type back
            current.endInit.body << QmltcCodeGenerator::wrap_privateClass(accessor.name, p) + u"->"
                            + p.write() + u"(" + groupPropertyVarName + u");";
        }

        // once the value is written back, process the script bindings
        for (; it != sortedBindings.cend(); ++it) {
            Q_ASSERT((*it)->type() == QmlIR::Binding::Type_Script);
            compile(*it);
        }
        break;
    }
    case QmlIR::Binding::Type_Null: {
        // poor check: null bindings are only supported for var and objects
        if (propertyType != m_localTypeResolver->varType()
            && propertyType->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            // TODO: this should really be done before the compiler here
            recordError(binding.location, u"Cannot assign null to incompatible property"_s);
        } else if (propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            addPropertyLine(p.propertyName(), p, u"nullptr"_s);
        } else {
            addPropertyLine(p.propertyName(), p, u"QVariant::fromValue(nullptr)"_s);
        }
        break;
    }
    case QmlIR::Binding::Type_Invalid: {
        recordError(binding.valueLocation,
                    u"Binding on property '" + propertyName + u"' is invalid or unsupported");
        break;
    }
    }
}

QString CodeGenerator::makeGensym(const QString &base)
{
    QString gensym = base;
    gensym.replace(QLatin1String("."), QLatin1String("_"));
    while (gensym.startsWith(QLatin1Char('_')) && gensym.size() >= 2
           && (gensym[1].isUpper() || gensym[1] == QLatin1Char('_'))) {
        gensym.remove(0, 1);
    }

    if (!m_typeCounts.contains(gensym)) {
        m_typeCounts.insert(gensym, 1);
    } else {
        gensym += u"_" + QString::number(m_typeCounts[gensym]++);
    }

    return gensym;
}

// returns compiled script binding for "property changed" handler in a form of object type
static QmltcType compileScriptBindingPropertyChangeHandler(
        const QmlIR::Binding &binding, const QmlIR::Object *irObject,
        const QString &urlMethodName, const QString &functorCppType, const QString &objectCppType,
        const QList<QmltcVariable> &slotParameters)
{
    QmltcType bindingFunctor {};
    bindingFunctor.cppType = functorCppType;
    bindingFunctor.ignoreInit = true;

    // default member variable and ctor:
    const QString pointerToObject = objectCppType + u" *";
    bindingFunctor.variables.emplaceBack(
            QmltcVariable { pointerToObject, u"m_self"_s, u"nullptr"_s });
    bindingFunctor.baselineCtor.name = functorCppType;
    bindingFunctor.baselineCtor.parameterList.emplaceBack(
            QmltcVariable { pointerToObject, u"self"_s, QString() });
    bindingFunctor.baselineCtor.initializerList.emplaceBack(u"m_self(self)"_s);

    // call operator:
    QmltcMethod callOperator {};
    callOperator.returnType = u"void"_s;
    callOperator.name = u"operator()"_s;
    callOperator.parameterList = slotParameters;
    callOperator.modifiers << u"const"_s;
    QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
            &callOperator.body, urlMethodName + u"()",
            relativeToAbsoluteRuntimeIndex(irObject, binding.value.compiledScriptIndex),
            u"m_self"_s, u"void"_s, slotParameters);

    bindingFunctor.functions.emplaceBack(std::move(callOperator));

    return bindingFunctor;
}

static std::optional<QQmlJSMetaProperty>
propertyForChangeHandler(const QQmlJSScope::ConstPtr &scope, QString name)
{
    if (!name.endsWith(QLatin1String("Changed")))
        return {};
    constexpr int length = int(sizeof("Changed") / sizeof(char)) - 1;
    name.chop(length);
    auto p = scope->property(name);
    const bool isBindable = !p.bindable().isEmpty();
    const bool canNotify = !p.notify().isEmpty();
    if (p.isValid() && (isBindable || canNotify))
        return p;
    return {};
}

void CodeGenerator::compileScriptBinding(QmltcType &current, const QmlIR::Binding &binding,
                                         const QString &bindingSymbolName,
                                         const CodeGenObject &object, const QString &propertyName,
                                         const QQmlJSScope::ConstPtr &propertyType,
                                         const CodeGenerator::AccessorData &accessor)
{
    auto objectType = object.type;

    // returns signal by name. signal exists if std::optional<> has value
    const auto signalByName = [&](const QString &name) -> std::optional<QQmlJSMetaMethod> {
        const auto signalMethods = objectType->methods(name, QQmlJSMetaMethod::Signal);
        if (signalMethods.isEmpty())
            return {};
        // if (signalMethods.size() != 1) return {}; // error somewhere else
        QQmlJSMetaMethod s = signalMethods.at(0);
        Q_ASSERT(s.methodType() == QQmlJSMetaMethod::Signal);
        return s;
    };

    const auto resolveSignal = [&](const QString &name) -> std::optional<QQmlJSMetaMethod> {
        auto signal = signalByName(name);
        if (signal) // found signal
            return signal;
        auto property = propertyForChangeHandler(objectType, name);
        if (!property) // nor signal nor property change handler
            return {}; // error somewhere else
        if (auto notify = property->notify(); !notify.isEmpty())
            return signalByName(notify);
        return {};
    };

    // TODO: add Invalid case which would reject garbage handlers
    enum BindingKind {
        JustProperty, // is a binding on property
        SignalHandler, // is a slot related to some signal
        BindablePropertyChangeHandler, // is a slot related to property's bindable
    };

    // these only make sense when binding is on signal handler
    QList<QmltcVariable> slotParameters;
    QString signalName;
    QString signalReturnType;

    const BindingKind kind = [&]() -> BindingKind {
        if (!QmlIR::IRBuilder::isSignalPropertyName(propertyName))
            return BindingKind::JustProperty;

        if (auto name = QQmlJSUtils::signalName(propertyName); name.has_value())
            signalName = *name;

        std::optional<QQmlJSMetaMethod> possibleSignal = resolveSignal(signalName);
        if (possibleSignal) { // signal with signalName exists
            QQmlJSMetaMethod s = possibleSignal.value();

            // Note: update signal name since it may be different from the one
            // used to resolve a signal
            signalName = s.methodName();

            const auto paramNames = s.parameterNames();
            const auto paramTypes = s.parameterTypes();
            Q_ASSERT(paramNames.size() == paramTypes.size());
            slotParameters = compileMethodParameters(paramNames, paramTypes, true);
            signalReturnType = figureReturnType(s);
            return BindingKind::SignalHandler;
        } else if (propertyName.endsWith(u"Changed"_s)) {
            return BindingKind::BindablePropertyChangeHandler;
        }
        return BindingKind::JustProperty;
    }();

    switch (kind) {
    case BindingKind::JustProperty: {
        if (!propertyType) {
            recordError(binding.location,
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        auto [property, absoluteIndex] = getMetaPropertyIndex(object.type, propertyName);
        if (absoluteIndex < 0) {
            recordError(binding.location, u"Binding on unknown property '" + propertyName + u"'");
            return;
        }

        QString bindingTarget = accessor.name;

        int valueTypeIndex = -1;
        if (accessor.isValueType) {
            Q_ASSERT(accessor.scope != object.type);
            bindingTarget = u"this"_s; // TODO: not necessarily "this"?
            auto [groupProperty, groupPropertyIndex] =
                    getMetaPropertyIndex(accessor.scope, accessor.propertyName);
            if (groupPropertyIndex < 0) {
                recordError(binding.location,
                            u"Binding on group property '" + accessor.propertyName
                                    + u"' of unknown type");
                return;
            }
            valueTypeIndex = absoluteIndex;
            absoluteIndex = groupPropertyIndex; // e.g. index of accessor.name
        }

        QmltcCodeGenerator::generate_createBindingOnProperty(
                &current.endInit.body, generate_callCompilationUnit(m_urlMethodName),
                u"this"_s, // NB: always using enclosing object as a scope for the binding
                relativeToAbsoluteRuntimeIndex(object.irObject, binding.value.compiledScriptIndex),
                bindingTarget, // binding target
                absoluteIndex, property, valueTypeIndex, accessor.name);
        break;
    }
    case BindingKind::SignalHandler: {
        QString This_signal = u"this"_s;
        QString This_slot = u"this"_s;
        QString objectClassName_signal = objectType->internalName();
        QString objectClassName_slot = objectType->internalName();
        // TODO: ugly crutch to make stuff work
        if (accessor.name != u"this"_s) { // e.g. if attached property
            This_signal = accessor.name;
            This_slot = u"this"_s; // still
            objectClassName_signal = objectType->baseTypeName();
            objectClassName_slot = current.cppType; // real base class where slot would go
        }
        Q_ASSERT(!objectClassName_signal.isEmpty());
        Q_ASSERT(!objectClassName_slot.isEmpty());
        const QString slotName = makeGensym(signalName + u"_slot");

        // SignalHander specific:
        QmltcMethod slotMethod {};
        slotMethod.returnType = signalReturnType;
        slotMethod.name = slotName;
        slotMethod.parameterList = slotParameters;

        QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
                &slotMethod.body, m_urlMethodName + u"()",
                relativeToAbsoluteRuntimeIndex(object.irObject,
                                               binding.value.compiledScriptIndex),
                u"this"_s, // Note: because script bindings always use current QML object scope
                signalReturnType, slotParameters);
        slotMethod.type = QQmlJSMetaMethod::Slot;

        current.functions << std::move(slotMethod);
        current.endInit.body << u"QObject::connect(" + This_signal + u", "
                        + QmltcCodeGenerator::wrap_qOverload(slotParameters,
                                                             u"&" + objectClassName_signal + u"::"
                                                                     + signalName)
                        + u", " + This_slot + u", &" + objectClassName_slot + u"::" + slotName
                        + u");";
        break;
    }
    case BindingKind::BindablePropertyChangeHandler: {
        const QString objectClassName = objectType->internalName();
        const QString bindingFunctorName = makeGensym(bindingSymbolName + u"Functor");

        QString actualPropertyName;
        QRegularExpression onChangedRegExp(u"on(\\w+)Changed"_s);
        QRegularExpressionMatch match = onChangedRegExp.match(propertyName);
        if (match.hasMatch()) {
            actualPropertyName = match.captured(1);
            actualPropertyName[0] = actualPropertyName.at(0).toLower();
        } else {
            // an error somewhere else
            return;
        }
        if (!objectType->hasProperty(actualPropertyName)) {
            recordError(binding.location, u"No property named " + actualPropertyName);
            return;
        }
        QQmlJSMetaProperty actualProperty = objectType->property(actualPropertyName);
        const auto actualPropertyType = actualProperty.type();
        if (!actualPropertyType) {
            recordError(binding.location,
                        u"Binding on property '" + actualPropertyName + u"' of unknown type");
            return;
        }

        QString bindableString = actualProperty.bindable();
        if (bindableString.isEmpty()) { // TODO: always should come from prop.bindalbe()
            recordError(binding.location,
                        u"Property '" + actualPropertyName
                                + u"' is non-bindable. Change handlers for such properties are not "
                                  u"supported");
            break;
        }

        QString typeOfQmlBinding =
                u"std::unique_ptr<QPropertyChangeHandler<" + bindingFunctorName + u">>";

        current.children << compileScriptBindingPropertyChangeHandler(
                binding, object.irObject, m_urlMethodName, bindingFunctorName,
                objectClassName, slotParameters);

        // TODO: this could be dropped if QQmlEngine::setContextForObject() is
        // done before currently generated C++ object is constructed
        current.endInit.body << bindingSymbolName + u".reset(new QPropertyChangeHandler<"
                        + bindingFunctorName + u">("
                        + QmltcCodeGenerator::wrap_privateClass(accessor.name, actualProperty)
                        + u"->" + bindableString + u"().onValueChanged(" + bindingFunctorName + u"("
                        + accessor.name + u"))));";

        current.variables.emplaceBack(
                QmltcVariable { typeOfQmlBinding, bindingSymbolName, QString() });
        break;
    }
    }
}

// TODO: should use "compileScriptBinding" instead of custom code
void CodeGenerator::compileScriptBindingOfComponent(QmltcType &current,
                                                    const QmlIR::Object *irObject,
                                                    const QQmlJSScope::ConstPtr objectType,
                                                    const QmlIR::Binding &binding,
                                                    const QString &propertyName)
{
    // returns signal by name. signal exists if std::optional<> has value
    const auto signalByName = [&](const QString &name) -> std::optional<QQmlJSMetaMethod> {
        const QList<QQmlJSMetaMethod> signalMethods = objectType->methods(name);
        // TODO: no clue how to handle redefinition, so just record an error
        if (signalMethods.size() != 1) {
            recordError(binding.location, u"Binding on redefined signal '" + name + u"'");
            return {};
        }
        QQmlJSMetaMethod s = signalMethods.at(0);
        if (s.methodType() != QQmlJSMetaMethod::Signal)
            return {};
        return s;
    };

    QString signalName;
    QString signalReturnType;

    Q_ASSERT(QmlIR::IRBuilder::isSignalPropertyName(propertyName));
    const auto offset = static_cast<uint>(strlen("on"));
    signalName = propertyName[offset].toLower() + propertyName.mid(offset + 1);

    std::optional<QQmlJSMetaMethod> possibleSignal = signalByName(signalName);
    if (!possibleSignal) {
        recordError(binding.location, u"Component signal '" + signalName + u"' is not recognized");
        return;
    }
    Q_ASSERT(possibleSignal); // Component's script binding is always a signal handler
    QQmlJSMetaMethod s = possibleSignal.value();
    Q_ASSERT(s.parameterNames().isEmpty()); // Component signals do not have parameters
    signalReturnType = figureReturnType(s);

    const QString slotName = makeGensym(signalName + u"_slot");

    // SignalHander specific:
    QmltcMethod slotMethod {};
    slotMethod.returnType = signalReturnType;
    slotMethod.name = slotName;

    // Component is special:
    int runtimeIndex =
            relativeToAbsoluteRuntimeIndex(irObject, binding.value.compiledScriptIndex);
    QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
            &slotMethod.body, m_urlMethodName + u"()", runtimeIndex, u"this"_s, signalReturnType);
    slotMethod.type = QQmlJSMetaMethod::Slot;

    // TODO: there's actually an attached type, which has completed/destruction
    // signals that are typically emitted -- do we care enough about supporting
    // that? see QQmlComponentAttached
    if (signalName == u"completed"_s) {
        current.handleOnCompleted.body << slotName + u"();";
    } else if (signalName == u"destruction"_s) {
        if (!current.dtor) {
            // TODO: double-check that this stuff is actually correct now:
            current.dtor = QmltcDtor {};
            current.dtor->name = u"~" + current.cppType;
        }
        current.dtor->body << slotName + u"();";
    }
    current.functions << std::move(slotMethod);
}

void CodeGenerator::recordError(const QQmlJS::SourceLocation &location, const QString &message)
{
    m_logger->log(message, Log_Compiler, location);
}

void CodeGenerator::recordError(const QV4::CompiledData::Location &location, const QString &message)
{
    recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message);
}

QT_END_NAMESPACE
