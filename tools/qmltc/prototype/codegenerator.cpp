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
        type = u"void"_qs;
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
    auto owner = QQmlJSScope::ownerOfProperty(scope, propertyName);
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
        return orderTable[x->type] < orderTable[y->type];
    }
    bool operator()(const T &x, const T &y) const
    {
        return orderTable[x.type] < orderTable[y.type];
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
                                                 const QString &accessor, bool constructQVariant)
{
    Q_UNUSED(type);
    Q_UNUSED(propertyName);
    Q_UNUSED(constructQVariant);

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
static const QmltcVariable childrenOffsetVariable { u"qsizetype"_qs, u"QML_choffset"_qs,
                                                    QString() };

// represents QV4::ExecutableCompilationUnit
static const QmltcVariable compilationUnitVariable { u"QV4::ExecutableCompilationUnit *"_qs,
                                                     u"QML_cu"_qs, QString() };

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
                        u"Internal compiler error: IR object is null"_qs);
            return;
        }
        QQmlJSScope::Ptr object = m_localTypeResolver->scopeForLocation(irObject->location);
        if (!object) {
            recordError(irObject->location, u"Object of unknown type"_qs);
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

QString buildCallSpecialMethodValue(bool documentRoot, const QString &outerFlagName,
                                    bool overridesInterface)
{
    const QString callInBase = overridesInterface ? u"false"_qs : u"true"_qs;
    if (documentRoot) {
        return outerFlagName + u" && " + callInBase;
    } else {
        return callInBase;
    }
}

static QString generate_callCompilationUnit(const QString &urlMethodName)
{
    // NB: assume `engine` variable always exists
    return u"QQmlEnginePrivate::get(engine)->compilationUnitFromUrl(%1())"_qs.arg(urlMethodName);
}

void CodeGenerator::compileAlias(QmltcType &current, const QQmlJSMetaProperty &alias,
                                 const QQmlJSScope::ConstPtr &owner)
{
    const QString aliasName = alias.propertyName();
    Q_ASSERT(!aliasName.isEmpty());

    QStringList aliasExprBits = alias.aliasExpression().split(u'.');
    Q_ASSERT(!aliasExprBits.isEmpty());
    QString idString = aliasExprBits.front();
    Q_ASSERT(!idString.isEmpty());
    QQmlJSScope::ConstPtr type = m_localTypeResolver->scopeForId(idString, owner);
    Q_ASSERT(type);
    const bool aliasToProperty = aliasExprBits.size() > 1; // and not an object

    QString latestAccessor = u"this"_qs;
    QString latestAccessorNonPrivate; // TODO: crutch for notify
    QStringList prologue;
    QStringList writeSpecificEpilogue;

    if (owner != type) { // cannot start at `this`, need to fetch object through context at run time
        Q_ASSERT(m_typeToObjectIndex.contains(type)
                 && m_typeToObjectIndex[type] <= m_objects.size());
        const int id = m_objects[m_typeToObjectIndex[type]].irObject->id;
        Q_ASSERT(id >= 0); // since the type is found by id, it must have an id

        // TODO: the context fetching must be done once in the notify code
        // (across multiple alias properties). now there's a huge duplication
        prologue << u"auto context = QQmlData::get(this)->outerContext;"_qs;
        // there's a special case: when `this` type has compiled QML type as a base
        // type and it is not a root, it has a non-root first context, so we need to
        // step one level up
        if (m_qmlCompiledBaseTypes.contains(owner->baseTypeName())
            && m_typeToObjectIndex[owner] != 0) {
            Q_ASSERT(!owner->baseTypeName().isEmpty());
            prologue << u"// `this` is special: not a root and its base type is compiled"_qs;
            prologue << u"context = context->parent().data();"_qs;
        }
        // doing the above allows us to lookup id object by integer, which is fast
        latestAccessor = u"alias_objectById_" + idString; // unique enough
        if (aliasToProperty) {
            prologue << u"auto " + latestAccessor + u" = static_cast<" + type->internalName()
                            + u"*>(context->idValue(" + QString::number(id) + u"));";
        } else {
            // kind of a crutch for object aliases: expect that the user knows
            // what to do and so accept QObject*
            prologue << u"QObject *" + latestAccessor + u" = context->idValue("
                            + QString::number(id) + u");";
        }
        prologue << u"Q_ASSERT(" + latestAccessor + u");";
    }

    struct AliasData
    {
        QString underlyingType;
        QString readLine;
        QString writeLine;
        QString bindableLine;
    } info; // note, we only really need this crutch to cover the alias to object case
    QQmlJSMetaProperty resultingProperty;

    if (aliasToProperty) {
        // the following code goes to prologue section. the idea of that is to
        // construct the necessary code to fetch the resulting property
        for (qsizetype i = 1; i < aliasExprBits.size() - 1; ++i) {
            const QString &bit = qAsConst(aliasExprBits)[i];
            Q_ASSERT(!bit.isEmpty());
            Q_ASSERT(type);
            QQmlJSMetaProperty p = type->property(bit);

            QString currAccessor = QmltcCodeGenerator::wrap_privateClass(latestAccessor, p);
            if (p.type()->accessSemantics() == QQmlJSScope::AccessSemantics::Value) {
                // we need to read the property to a local variable and then
                // write the updated value once the actual operation is done
                QString localVariableName = u"alias_" + bit; // should be fairly unique
                prologue << u"auto " + localVariableName + u" = " + currAccessor + u"->" + p.read()
                                + u"();";
                writeSpecificEpilogue
                        << currAccessor + u"->" + p.write() + u"(" + localVariableName + u");";
                // NB: since accessor becomes a value type, wrap it into an
                // addressof operator so that we could access it as a pointer
                currAccessor = QmltcCodeGenerator::wrap_addressof(localVariableName); // reset
            } else {
                currAccessor += u"->" + p.read() + u"()"; // amend
            }

            type = p.type();
            latestAccessor = currAccessor;
        }

        resultingProperty = type->property(aliasExprBits.back());
        latestAccessorNonPrivate = latestAccessor;
        latestAccessor = QmltcCodeGenerator::wrap_privateClass(latestAccessor, resultingProperty);
        info.underlyingType = resultingProperty.type()->internalName();
        if (resultingProperty.isList()) {
            info.underlyingType = u"QQmlListProperty<" + info.underlyingType + u">";
        } else if (resultingProperty.type()->isReferenceType()) {
            info.underlyingType += u"*"_qs;
        }
        // reset to generic type when having alias to id:
        if (m_aliasesToIds.contains(resultingProperty))
            info.underlyingType = u"QObject*"_qs;

        // READ must always be present
        info.readLine = latestAccessor + u"->" + resultingProperty.read() + u"()";
        if (QString setName = resultingProperty.write(); !setName.isEmpty())
            info.writeLine = latestAccessor + u"->" + setName + u"(%1)";
        if (QString bindableName = resultingProperty.bindable(); !bindableName.isEmpty())
            info.bindableLine = latestAccessor + u"->" + bindableName + u"()";
    } else {
        info.underlyingType = u"QObject*"_qs;
        info.readLine = latestAccessor;
        // NB: id-pointing aliases are read-only, also alias to object is not
        // bindable since it's not a QProperty itself
    }

    QStringList mocLines;
    mocLines.reserve(10);
    mocLines << info.underlyingType << aliasName;

    QmltcPropertyData compilationData(aliasName);
    // 1. add setter and getter
    if (!info.readLine.isEmpty()) {
        QmltcMethod getter {};
        getter.returnType = info.underlyingType;
        getter.name = compilationData.read;
        getter.body += prologue;
        getter.body << u"return " + info.readLine + u";";
        // getter.body += writeSpecificEpilogue;
        getter.userVisible = true;
        current.functions.emplaceBack(getter);
        mocLines << u"READ"_qs << getter.name;
    } // else always an error?

    if (!info.writeLine.isEmpty()) {
        QmltcMethod setter {};
        setter.returnType = u"void"_qs;
        setter.name = compilationData.write;

        QList<QQmlJSMetaMethod> methods = type->methods(resultingProperty.write());
        if (methods.isEmpty()) {
            // QmltcVariable
            setter.parameterList.emplaceBack(QQmlJSUtils::constRefify(info.underlyingType),
                                             aliasName + u"_", u""_qs);
        } else {
            setter.parameterList = compileMethodParameters(methods.at(0).parameterNames(),
                                                           methods.at(0).parameterTypes(), true);
        }

        setter.body += prologue;
        if (aliasToProperty) {
            QStringList parameterNames;
            parameterNames.reserve(setter.parameterList.size());
            std::transform(setter.parameterList.cbegin(), setter.parameterList.cend(),
                           std::back_inserter(parameterNames),
                           [](const QmltcVariable &x) { return x.name; });
            QString commaSeparatedParameterNames = parameterNames.join(u", "_qs);
            setter.body << info.writeLine.arg(commaSeparatedParameterNames) + u";";
        } else {
            setter.body << info.writeLine + u";";
        }
        setter.body += writeSpecificEpilogue;
        setter.userVisible = true;
        current.functions.emplaceBack(setter);
        mocLines << u"WRITE"_qs << setter.name;
    }

    // 2. add bindable
    if (!info.bindableLine.isEmpty()) {
        QmltcMethod bindable {};
        bindable.returnType = u"QBindable<" + info.underlyingType + u">";
        bindable.name = compilationData.bindable;
        bindable.body += prologue;
        bindable.body << u"return " + info.bindableLine + u";";
        bindable.userVisible = true;
        current.functions.emplaceBack(bindable);
        mocLines << u"BINDABLE"_qs << bindable.name;
    }
    // 3. add notify - which is pretty special
    if (QString notifyName = resultingProperty.notify(); !notifyName.isEmpty()) {
        // notify is very special (even more than reasonable)
        current.endInit.body << u"{ // alias notify connection:"_qs;
        // TODO: part of the prologue (e.g. QQmlData::get(this)->outerContext)
        // must be shared across different notifies (to speed up finalize)
        current.endInit.body += prologue;
        // TODO: use non-private accessor since signals must exist on the public
        // type, not on the private one -- otherwise, they can't be used from
        // C++ which is not what's wanted (this is a mess)
        current.endInit.body << u"QObject::connect(" + latestAccessorNonPrivate + u", &"
                        + type->internalName() + u"::" + notifyName + u", this, &" + current.cppType
                        + u"::" + compilationData.notify + u");";
        current.endInit.body << u"}"_qs;
    }

    // 4. add moc entry
    // Q_PROPERTY(QString text READ text WRITE setText BINDABLE bindableText NOTIFY textChanged)
    current.mocCode << u"Q_PROPERTY(" + mocLines.join(u" "_qs) + u")";

    // 5. add extra moc entry if this alias is default one
    if (aliasName == owner->defaultPropertyName()) {
        // Q_CLASSINFO("DefaultProperty", propertyName)
        current.mocCode << u"Q_CLASSINFO(\"DefaultProperty\", \"%1\")"_qs.arg(aliasName);
    }
}

template<typename Iterator>
static QString getPropertyOrAliasNameFromIr(const QmlIR::Document *doc, Iterator first, int pos)
{
    std::advance(first, pos); // assume that first is correct - due to earlier check
    return doc->stringAt(first->nameIndex);
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
        if (binding.type == QmlIR::Binding::Type_GroupProperty) {
            // TODO: we should warn about this in QmlCompiler library
            qCWarning(lcCodeGenerator)
                    << QStringLiteral("Binding at line %1 column %2 is not deferred as it is a "
                                      "binding on a group property.")
                               .arg(QString::number(binding.location.line),
                                    QString::number(binding.location.column));
            // we do not support PropertyChanges and other types with similar
            // behavior yet, so this binding is compiled
        } else {
            qCDebug(lcCodeGenerator)
                    << QStringLiteral(
                               "Binding at line %1 column %2 is deferred and thus not compiled")
                               .arg(QString::number(binding.location.line),
                                    QString::number(binding.location.column));
            return;
        }
    }

    const auto addPropertyLine = [&](const QString &propertyName, const QQmlJSMetaProperty &p,
                                     const QString &value, bool constructQVariant = false) {
        // TODO: there mustn't be this special case. instead, alias resolution
        // must be done in QQmlJSImportVisitor subclass, that would handle this
        // mess (see resolveValidateOrSkipAlias() in qml2cppdefaultpasses.cpp)
        if (p.isAlias() && m_qmlCompiledBaseTypes.contains(object.type->baseTypeName())) {
            qCDebug(lcCodeGenerator) << u"Property '" + propertyName + u"' is an alias on type '"
                            + object.type->internalName()
                            + u"' which is a QML type compiled to C++. The assignment is special"
                            + u"in this case";
            current.endInit.body += generate_assignToSpecialAlias(
                    object.type, propertyName, p, value, accessor.name, constructQVariant);
        } else {
            QmltcCodeGenerator::generate_assignToProperty(&current.endInit.body, object.type, p,
                value, accessor.name, constructQVariant);
        }
    };

    switch (binding.type) {
    case QmlIR::Binding::Type_Boolean: {
        addPropertyLine(propertyName, p, binding.value.b ? u"true"_qs : u"false"_qs);
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
        const QString qobjectParent = u"this"_qs;
        const QString objectAddr = accessor.name;

        if (binding.flags & QmlIR::Binding::IsOnAssignment) {
            const QString onAssignmentName = u"onAssign_" + propertyName;
            const auto uniqueId = UniqueStringId(current, onAssignmentName);
            if (!m_onAssignmentObjectsCreated.contains(uniqueId)) {
                m_onAssignmentObjectsCreated.insert(uniqueId);
                current.init.body << u"auto %1 = new %2(creator, engine, %3);"_qs.arg(
                        onAssignmentName, bindingObject.type->internalName(), qobjectParent);
                current.init.body << u"creator->set(%1, %2);"_qs.arg(
                        QString::number(m_visitor->creationIndex(bindingObject.type)),
                        onAssignmentName);

                // static_cast is fine, because we (must) know the exact type
                current.endInit.body << u"auto %1 = creator->get<%2>(%3);"_qs.arg(
                        onAssignmentName, bindingObject.type->internalName(),
                        QString::number(m_visitor->creationIndex(bindingObject.type)));

                m_localChildrenToEndInit.append(onAssignmentName);
                m_localChildrenToFinalize.append(bindingObject.type);
            }

            // NB: we expect one "on" assignment per property, so creating
            // QQmlProperty each time should be fine (unlike QQmlListReference)
            current.endInit.body << u"{"_qs;
            current.endInit.body << u"QQmlProperty qmlprop(" + objectAddr + u", QStringLiteral(\""
                            + propertyName + u"\"));";
            current.endInit.body << u"QT_PREPEND_NAMESPACE(QQmlCppOnAssignmentHelper)::set("
                            + onAssignmentName + u", qmlprop);";
            current.endInit.body << u"}"_qs;
            break;
        }

        if (!propertyType) {
            recordError(binding.location,
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        if (p.isList() || (binding.flags & QmlIR::Binding::IsListItem)) {
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
            if (p.isList() || (binding.flags & QmlIR::Binding::IsListItem)) {
                const QString refName = u"listref_" + propertyName;
                current.endInit.body << refName + u".append(" + value + u");";
            } else {
                addPropertyLine(propertyName, p, value, /* through QVariant = */ true);
            }
        };

        if (bindingObject.irObject->flags & QV4::CompiledData::Object::IsComponent) {
            // NB: this one is special - and probably becomes a view delegate.
            // object binding separation also does not apply here
            const QString objectName = makeGensym(u"sc"_qs);
            Q_ASSERT(m_typeToObjectIndex.contains(bindingObject.type));
            const int objectIndex = int(m_typeToObjectIndex[bindingObject.type]);
            Q_ASSERT(m_componentIndices.contains(objectIndex));
            const int index = m_componentIndices[objectIndex];
            current.endInit.body << u"{"_qs;
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
                return base && base->internalName() == u"QQmlComponent"_qs;
            };
            if (!m_doc->stringAt(bindingObject.irObject->idNameIndex).isEmpty()
                && isComponentBased(bindingObject.type)) {

                QmltcCodeGenerator::generate_setIdValue(
                        &current.endInit.body, u"thisContext"_qs, bindingObject.irObject->id,
                        objectName, m_doc->stringAt(bindingObject.irObject->idNameIndex));
            }
            setObjectBinding(objectName);
            current.endInit.body << u"}"_qs;
            break;
        }

        const QString objectName = makeGensym(u"o"_qs);
        current.init.body << u"auto %1 = new %2(creator, engine, %3);"_qs.arg(
                objectName, bindingObject.type->internalName(), qobjectParent);
        current.init.body << u"creator->set(%1, %2);"_qs.arg(
                QString::number(m_visitor->creationIndex(bindingObject.type)), objectName);

        // refetch the same object during endInit to set the bindings
        current.endInit.body << u"auto %1 = creator->get<%2>(%3);"_qs.arg(
                objectName, bindingObject.type->internalName(),
                QString::number(m_visitor->creationIndex(bindingObject.type)));
        setObjectBinding(objectName);

        m_localChildrenToEndInit.append(objectName);
        m_localChildrenToFinalize.append(bindingObject.type);
        break;
    }
    case QmlIR::Binding::Type_AttachedProperty: {
        Q_ASSERT(accessor.name == u"this"_qs); // TODO: doesn't have to hold, in fact
        const auto attachedObject = m_objects.at(binding.value.objectIndex);
        const auto &irObject = attachedObject.irObject;
        const auto &type = attachedObject.type;
        Q_ASSERT(irObject && type);
        if (propertyName == u"Component"_qs) {
            // TODO: there's a special QQmlComponentAttached, which has to be
            // called? c.f. qqmlobjectcreator.cpp's finalize()
            const auto compileComponent = [&](const QmlIR::Binding &b) {
                Q_ASSERT(b.type == QmlIR::Binding::Type_Script);
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
                                              u"nullptr"_qs);
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
        Q_ASSERT(accessor.name == u"this"_qs); // TODO: doesn't have to hold, in fact
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
            return b.type != QmlIR::Binding::Type_Invalid && b.type != QmlIR::Binding::Type_Script;
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
                             [](auto it) { return it->type == QmlIR::Binding::Type_Script; });
        auto it = sortedBindings.cbegin();
        for (; it != scriptBindingsBegin; ++it) {
            Q_ASSERT((*it)->type != QmlIR::Binding::Type_Script);
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
            Q_ASSERT((*it)->type == QmlIR::Binding::Type_Script);
            compile(*it);
        }
        break;
    }
    case QmlIR::Binding::Type_Null: {
        // poor check: null bindings are only supported for var and objects
        if (propertyType != m_localTypeResolver->varType()
            && propertyType->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            // TODO: this should really be done before the compiler here
            recordError(binding.location, u"Cannot assign null to incompatible property"_qs);
        } else if (propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            addPropertyLine(p.propertyName(), p, u"nullptr"_qs);
        } else {
            addPropertyLine(p.propertyName(), p, u"QVariant::fromValue(nullptr)"_qs);
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
            QmltcVariable { pointerToObject, u"m_self"_qs, u"nullptr"_qs });
    bindingFunctor.baselineCtor.name = functorCppType;
    bindingFunctor.baselineCtor.parameterList.emplaceBack(
            QmltcVariable { pointerToObject, u"self"_qs, QString() });
    bindingFunctor.baselineCtor.initializerList.emplaceBack(u"m_self(self)"_qs);

    // call operator:
    QmltcMethod callOperator {};
    callOperator.returnType = u"void"_qs;
    callOperator.name = u"operator()"_qs;
    callOperator.parameterList = slotParameters;
    callOperator.modifiers << u"const"_qs;
    QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
            &callOperator.body, urlMethodName + u"()",
            relativeToAbsoluteRuntimeIndex(irObject, binding.value.compiledScriptIndex),
            u"m_self"_qs, u"void"_qs, slotParameters);

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
        } else if (propertyName.endsWith(u"Changed"_qs)) {
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
            bindingTarget = u"this"_qs; // TODO: not necessarily "this"?
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
                u"this"_qs, // NB: always using enclosing object as a scope for the binding
                relativeToAbsoluteRuntimeIndex(object.irObject, binding.value.compiledScriptIndex),
                bindingTarget, // binding target
                absoluteIndex, property, valueTypeIndex, accessor.name);
        break;
    }
    case BindingKind::SignalHandler: {
        QString This_signal = u"this"_qs;
        QString This_slot = u"this"_qs;
        QString objectClassName_signal = objectType->internalName();
        QString objectClassName_slot = objectType->internalName();
        // TODO: ugly crutch to make stuff work
        if (accessor.name != u"this"_qs) { // e.g. if attached property
            This_signal = accessor.name;
            This_slot = u"this"_qs; // still
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
                u"this"_qs, // Note: because script bindings always use current QML object scope
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
        QRegularExpression onChangedRegExp(u"on(\\w+)Changed"_qs);
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
            &slotMethod.body, m_urlMethodName + u"()", runtimeIndex, u"this"_qs, signalReturnType);
    slotMethod.type = QQmlJSMetaMethod::Slot;

    // TODO: there's actually an attached type, which has completed/destruction
    // signals that are typically emitted -- do we care enough about supporting
    // that? see QQmlComponentAttached
    if (signalName == u"completed"_qs) {
        current.handleOnCompleted.body << slotName + u"();";
    } else if (signalName == u"destruction"_qs) {
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
    recordError(QQmlJS::SourceLocation { 0, 0, location.line, location.column }, message);
}

QT_END_NAMESPACE
