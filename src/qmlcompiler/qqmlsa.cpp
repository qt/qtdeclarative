// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmlsa.h"
#include "qqmlsa_p.h"
#include "qqmlsasourcelocation.h"

#include "qqmljsscope_p.h"
#include "qqmljslogger_p.h"
#include "qqmljstyperesolver_p.h"
#include "qqmljsimportvisitor_p.h"
#include "qqmljsutils_p.h"
#include "qdeferredpointer_p.h"

#include <QtQmlCompiler/private/qqmlsasourcelocation_p.h>

#include <memory>
#include <new>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QQmlSA {

static_assert(QQmlJSScope::sizeofQQmlSAElement() == sizeof(Element));

/*!
    \namespace QQmlSA
    \inmodule QtQmlCompiler

    \brief Provides tools for static analysis on QML programs.
 */

/*!
    \class QQmlSA::Binding::Bindings
    \inmodule QtQmlCompiler

    \brief Holds multiple property name to property binding associations.
 */

Binding::Bindings::Bindings() : d_ptr{ new BindingsPrivate{ this } } { }

BindingsPrivate::BindingsPrivate(QQmlSA::Binding::Bindings *interface) : q_ptr{ interface } { }

Binding::Bindings::Bindings(const Bindings &other)
    : d_ptr{ new BindingsPrivate{ this, *other.d_func() } }
{
}

Binding::Bindings::~Bindings() = default;

BindingsPrivate::BindingsPrivate(QQmlSA::Binding::Bindings *interface, const BindingsPrivate &other)
    : m_bindings{ other.m_bindings.begin(), other.m_bindings.end() }, q_ptr{ interface }
{
}

BindingsPrivate::BindingsPrivate(QQmlSA::Binding::Bindings *interface, BindingsPrivate &&other)
    : m_bindings{ std::move(other.m_bindings) }, q_ptr{ interface }
{
}

/*!
    Returns an iterator to the beginning of the bindings.
 */
QMultiHash<QString, Binding>::const_iterator Binding::Bindings::constBegin() const
{
    Q_D(const Bindings);
    return d->constBegin();
}

QMultiHash<QString, Binding>::const_iterator BindingsPrivate::constBegin() const
{
    return m_bindings.constBegin();
}

/*!
    Returns an iterator to the end of the bindings.
 */
QMultiHash<QString, Binding>::const_iterator Binding::Bindings::constEnd() const
{
    Q_D(const Bindings);
    return d->constEnd();
}

QMultiHash<QString, Binding>::const_iterator BindingsPrivate::constEnd() const
{
    return m_bindings.constEnd();
}

/*!
    \class QQmlSA::Binding
    \inmodule QtQmlCompiler

    \brief Represents a single QML property binding for a specific type.
 */

Binding::Binding() : d_ptr{ new BindingPrivate{ this } } { }

BindingPrivate::BindingPrivate(Binding *interface) : q_ptr{ interface } { }

Binding::Binding(const Binding &other) : d_ptr{ new BindingPrivate{ this, *other.d_func() } } { }

Binding::Binding(Binding &&other) noexcept
    : d_ptr{ new BindingPrivate{ this, *other.d_func() } } { }

Binding &Binding::operator=(const Binding &other)
{
    if (*this == other)
        return *this;

    d_func()->m_binding = other.d_func()->m_binding;
    d_func()->q_ptr = this;
    return *this;
}

Binding &Binding::operator=(Binding &&other) noexcept
{
    if (*this == other)
        return *this;

    d_func()->m_binding = std::move(other.d_func()->m_binding);
    d_func()->q_ptr = this;
    return *this;
}

Binding::~Binding() = default;

bool Binding::operatorEqualsImpl(const Binding &lhs, const Binding &rhs)
{
    return lhs.d_func()->m_binding == rhs.d_func()->m_binding;
}

BindingPrivate::BindingPrivate(Binding *interface, const BindingPrivate &other)
    : m_binding{ other.m_binding }, q_ptr{ interface }
{
}

QQmlSA::Binding BindingPrivate::createBinding(const QQmlJSMetaPropertyBinding &binding)
{
    QQmlSA::Binding saBinding;
    saBinding.d_func()->m_binding = binding;
    return saBinding;
}

QQmlJSMetaPropertyBinding BindingPrivate::binding(QQmlSA::Binding &binding)
{
    return binding.d_func()->m_binding;
}

const QQmlJSMetaPropertyBinding BindingPrivate::binding(const QQmlSA::Binding &binding)
{
    return binding.d_func()->m_binding;
}

/*!
    Returns the type of the property if this element is a group property,
    otherwise returns an invalid Element.
 */
Element Binding::groupType() const
{
    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(*this).groupType());
}

QQmlSA::BindingType Binding::bindingType() const
{
    return BindingPrivate::binding(*this).bindingType();
}

/*!
    Returns the associated string literal if the content type of this binding is
    StringLiteral, otherwise returns an empty string.
 */
QString Binding::stringValue() const
{
    return BindingPrivate::binding(*this).stringValue();
}

/*!
    Returns the name of the property using this binding.
 */
QString Binding::propertyName() const
{
    return BindingPrivate::binding(*this).propertyName();
}

/*!
    Returns the attached type if the content type of this binding is
    AttachedProperty, otherwise returns an invalid Element.
 */
Element Binding::attachingType() const
{
    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(*this).attachingType());
}

/*!
    Returns the location in the QML code where this binding is defined.
 */
QQmlSA::SourceLocation Binding::sourceLocation() const
{
    return QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
            BindingPrivate::binding(*this).sourceLocation());
}

/*!
    Returns the associated number if the content type of this binding is
    NumberLiteral, otherwise returns 0.
 */
double Binding::numberValue() const
{
    return BindingPrivate::binding(*this).numberValue();
}

/*!
    Returns the kind of associated associated script if the content type of
    this binding is Script, otherwise returns Script_Invalid.
 */
QQmlSA::ScriptBindingKind Binding::scriptKind() const
{
    return BindingPrivate::binding(*this).scriptKind();
}

/*!
    Returns \c true if this binding has an objects, otherwise returns \c false.
 */
bool Binding::hasObject() const
{
    return BindingPrivate::binding(*this).hasObject();
}

/*!
    Returns the type of the associated object if the content type of this
    binding is Object, otherwise returns an invlaid Element.
 */
QQmlSA::Element Binding::objectType() const
{
    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(*this).objectType());
}

Element QQmlSA::Binding::literalType(const QQmlJSTypeResolver *resolver) const
{
    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(*this).literalType(resolver));
}

bool Binding::hasUndefinedScriptValue() const
{
    const auto &jsBinding = BindingPrivate::binding(*this);
    return jsBinding.bindingType() == BindingType::Script
            && jsBinding.scriptValueType() == ScriptValue_Undefined;
}

/*!
    Returns \c true if \a bindingType is a literal type, and \c false
    otherwise. Literal types include strings, booleans, numbers, regular
    expressions among other.
 */
bool QQmlSA::Binding::isLiteralBinding(QQmlSA::BindingType bindingType)
{
    return QQmlJSMetaPropertyBinding::isLiteralBinding(bindingType);
}

QQmlSA::Method::Methods::Methods() : d_ptr{ new MethodsPrivate{ this } } { }

QQmlSA::Method::Methods::Methods(const Methods &other)
    : d_ptr{ new MethodsPrivate{ this, *other.d_func() } }
{
}

QQmlSA::Method::Methods::~Methods() = default;

/*!
    Returns an iterator to the beginning of the methods.
 */
QMultiHash<QString, Method>::const_iterator Method::Methods::constBegin() const
{
    Q_D(const Methods);
    return d->constBegin();
}

QMultiHash<QString, Method>::const_iterator MethodsPrivate::constBegin() const
{
    return m_methods.constBegin();
}

/*!
    Returns an iterator to the end of the methods.
 */
QMultiHash<QString, Method>::const_iterator Method::Methods::constEnd() const
{
    Q_D(const Methods);
    return d->constEnd();
}
QMultiHash<QString, Method>::const_iterator MethodsPrivate::constEnd() const
{
    return m_methods.constEnd();
}

MethodsPrivate::MethodsPrivate(QQmlSA::Method::Methods *interface) : q_ptr{ interface } { }

MethodsPrivate::MethodsPrivate(QQmlSA::Method::Methods *interface, const MethodsPrivate &other)
    : m_methods{ other.m_methods }, q_ptr{ interface }
{
}

MethodsPrivate::MethodsPrivate(QQmlSA::Method::Methods *interface, MethodsPrivate &&other)
    : m_methods{ std::move(other.m_methods) }, q_ptr{ interface }
{
}

MethodPrivate::MethodPrivate(Method *interface) : q_ptr{ interface } { }

MethodPrivate::MethodPrivate(Method *interface, const MethodPrivate &other)
    : m_method{ other.m_method }, q_ptr{ interface }
{
}

QString MethodPrivate::methodName() const
{
    return m_method.methodName();
}

MethodType MethodPrivate::methodType() const
{
    return m_method.methodType();
}

/*!
    \class QQmlSA::Method
    \inmodule QtQmlCompiler

    \brief Represents a QML method.
 */

Method::Method() : d_ptr{ new MethodPrivate{ this } } { }

Method::Method(const Method &other) : d_ptr{ new MethodPrivate{ this, *other.d_func() } } { }

Method::Method(Method &&other) noexcept
    : d_ptr{ new MethodPrivate{ this, std::move(*other.d_func()) } }
{
}

Method &Method::operator=(const Method &other)
{
    if (*this == other)
        return *this;

    d_func()->m_method = other.d_func()->m_method;
    d_func()->q_ptr = this;
    return *this;
}

Method &Method::operator=(Method &&other) noexcept
{
    if (*this == other)
        return *this;

    d_func()->m_method = std::move(other.d_func()->m_method);
    d_func()->q_ptr = this;
    return *this;
}

Method::~Method() = default;

/*!
    Returns the name of the this method.
 */
QString Method::methodName() const
{
    Q_D(const Method);
    return d->methodName();
}

/*!
    Returns the type of this method. For example, Signal, Slot, Method or
    StaticMethod.
 */
MethodType Method::methodType() const
{
    Q_D(const Method);
    return d->methodType();
}

bool Method::operatorEqualsImpl(const Method &lhs, const Method &rhs)
{
    return lhs.d_func()->m_method == rhs.d_func()->m_method;
}

QQmlSA::Method MethodPrivate::createMethod(const QQmlJSMetaMethod &jsMethod)
{
    QQmlSA::Method saMethod;
    auto &wrappedMethod = saMethod.d_func()->m_method;
    wrappedMethod = jsMethod;
    return saMethod;
}

QQmlSA::Method::Methods
MethodsPrivate::createMethods(const QMultiHash<QString, QQmlJSMetaMethod> &hash)
{
    QMultiHash<QString, QQmlSA::Method> saMethods;
    for (const auto &[key, value] : hash.asKeyValueRange()) {
        saMethods.insert(key, MethodPrivate::createMethod(value));
    }

    QQmlSA::Method::Methods methods;
    methods.d_func()->m_methods = std::move(saMethods);
    return methods;
}

QQmlJSMetaMethod MethodPrivate::method(const QQmlSA::Method &method)
{
    return method.d_func()->m_method;
}

PropertyPrivate::PropertyPrivate(Property *interface) : q_ptr{ interface } { }

PropertyPrivate::PropertyPrivate(Property *interface, const PropertyPrivate &other)
    : m_property{ other.m_property }, q_ptr{ interface }
{
}

PropertyPrivate::PropertyPrivate(Property *interface, PropertyPrivate &&other)
    : m_property{ std::move(other.m_property) }, q_ptr{ interface }
{
}

QString PropertyPrivate::typeName() const
{
    return m_property.typeName();
}

bool PropertyPrivate::isValid() const
{
    return m_property.isValid();
}

QQmlJSMetaProperty PropertyPrivate::property(const QQmlSA::Property &property)
{
    return property.d_func()->m_property;
}

QQmlSA::Property PropertyPrivate::createProperty(const QQmlJSMetaProperty &property)
{
    QQmlSA::Property saProperty;
    auto &wrappedProperty = saProperty.d_func()->m_property;
    wrappedProperty = property;
    return saProperty;
}

/*!
    \class QQmlSA::Property
    \inmodule QtQmlCompiler

    \brief Represents a QML property.
 */

Property::Property() : d_ptr{ new PropertyPrivate{ this } } { }

Property::Property(const Property &other)
    : d_ptr{ new PropertyPrivate{ this, *other.d_func() } } { }

Property::Property(Property &&other) noexcept
    : d_ptr{ new PropertyPrivate{ this, std::move(*other.d_func()) } }
{
}

Property &Property::operator=(const Property &other)
{
    if (*this == other)
        return *this;

    d_func()->m_property = other.d_func()->m_property;
    d_func()->q_ptr = this;
    return *this;
}

Property &Property::operator=(Property &&other) noexcept
{
    if (*this == other)
        return *this;

    d_func()->m_property = std::move(other.d_func()->m_property);
    d_func()->q_ptr = this;
    return *this;
}

Property::~Property() = default;

/*!
    Returns the name of the type of this property.
 */
QString Property::typeName() const
{
    Q_D(const Property);
    return d->typeName();
}

bool Property::isValid() const
{
    Q_D(const Property);
    return d->isValid();
}

bool Property::operatorEqualsImpl(const Property &lhs, const Property &rhs)
{
    return lhs.d_func()->m_property == rhs.d_func()->m_property;
}

/*!
    \class QQmlSA::Element
    \inmodule QtQmlCompiler

    \brief Represents a QML type.
 */

Element::Element()
{
    new (m_data) QQmlJSScope::ConstPtr();
}

Element::Element(const Element &other)
{
    new (m_data) QQmlJSScope::ConstPtr(QQmlJSScope::scope(other));
}

Element &Element::operator=(const Element &other)
{
    if (this == &other)
        return *this;

    *reinterpret_cast<QQmlJSScope::ConstPtr *>(m_data) = QQmlJSScope::scope(other);
    return *this;
}

Element::~Element()
{
    (*reinterpret_cast<QQmlJSScope::ConstPtr *>(m_data)).QQmlJSScope::ConstPtr::~ConstPtr();
}

/*!
    Returns the type of Element's scope.
 */
QQmlJSScope::ScopeType Element::scopeType() const
{
    return QQmlJSScope::scope(*this)->scopeType();
}

/*!
    Returns the Element this Element derives from.
 */
Element Element::baseType() const
{
    return QQmlJSScope::createQQmlSAElement(QQmlJSScope::scope(*this)->baseType());
}

/*!
    Returns the name of the Element this Element derives from.
 */
QString Element::baseTypeName() const
{
    return QQmlJSScope::prettyName(QQmlJSScope::scope(*this)->baseTypeName());
}

/*!
    Returns the Element that encloses this Element.
 */
Element Element::parentScope() const
{
    return QQmlJSScope::createQQmlSAElement(QQmlJSScope::scope(*this)->parentScope());
}

/*!
    Returns whether this Element inherits from \a element.
 */
bool Element::inherits(const Element &element) const
{
    return QQmlJSScope::scope(*this)->inherits(QQmlJSScope::scope(element));
}

bool Element::isNull() const
{
    return QQmlJSScope::scope(*this).isNull();
}

/*!
    \internal
 */
QString Element::internalId() const
{
    return QQmlJSScope::scope(*this)->internalName();
}

/*!
    Returns the access semantics of this Element. For example, Reference,
    Value or Sequence.
 */
AccessSemantics Element::accessSemantics() const
{
    return QQmlJSScope::scope(*this)->accessSemantics();
}

/*!
    Returns true for objects defined from Qml, and false for objects declared from C++.
 */
bool QQmlSA::Element::isComposite() const
{
    return QQmlJSScope::scope(*this)->isComposite();
}

/*!
    Returns whether this Element has a property with the name \a propertyName.
 */
bool Element::hasProperty(const QString &propertyName) const
{
    return QQmlJSScope::scope(*this)->hasProperty(propertyName);
}

/*!
    Returns the property with the name \a propertyName if it is found in this
    Element or its base and extension objects, otherwise returns an invalid property.
 */
QQmlSA::Property Element::property(const QString &propertyName) const
{
    return PropertyPrivate::createProperty(QQmlJSScope::scope(*this)->property(propertyName));
}

/*!
    Returns whether the property with the name \a propertyName resolved on this
    Element is required. Returns false if the the property couldn't be found.
 */
bool Element::isPropertyRequired(const QString &propertyName) const
{
    return QQmlJSScope::scope(*this)->isPropertyRequired(propertyName);
}

/*!
    Returns the name of the default property of this Element. If it doesn't
    have one, returns an empty string.
 */
QString Element::defaultPropertyName() const
{
    return QQmlJSScope::scope(*this)->defaultPropertyName();
}

/*!
    Returns whether this Element has a method with the name \a methodName.
 */
bool Element::hasMethod(const QString &methodName) const
{
    return QQmlJSScope::scope(*this)->hasMethod(methodName);
}

/*!
    \class QQmlSA::Method::Methods
    \inmodule QtQmlCompiler

    \brief Holds multiple method name to method associations.
 */

/*!
    Returns this Elements's method which are not defined on its base or
    extension objects.
 */
Method::Methods Element::ownMethods() const
{
    return MethodsPrivate::createMethods(QQmlJSScope::scope(*this)->ownMethods());
}

/*!
    Returns the location in the QML code where this method is defined.
 */
QQmlSA::SourceLocation Element::sourceLocation() const
{
    return QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
            QQmlJSScope::scope(*this)->sourceLocation());
}

/*!
    Returns the file path of the QML code that defines this method.
 */
QString Element::filePath() const
{
    return QQmlJSScope::scope(*this)->filePath();
}

/*!
    Returns whethe this Element has a property binding with the name \a name.
 */
bool Element::hasPropertyBindings(const QString &name) const
{
    return QQmlJSScope::scope(*this)->hasPropertyBindings(name);
}

/*!
    Returns whether this Element has property bindings which are not defined in
    its base or extension objects and that have name \a propertyName.
 */
bool Element::hasOwnPropertyBindings(const QString &propertyName) const
{
    return QQmlJSScope::scope(*this)->hasOwnPropertyBindings(propertyName);
}

/*!
    Returns this Element's property bindings which are not defined on its base
    or extension objects.
 */
Binding::Bindings Element::ownPropertyBindings() const
{
    return BindingsPrivate::createBindings(QQmlJSScope::scope(*this)->ownPropertyBindings());
}

/*!
    Returns this Element's property bindings which are not defined on its base
    or extension objects and that have the name \a propertyName.
 */
Binding::Bindings Element::ownPropertyBindings(const QString &propertyName) const
{
    return BindingsPrivate::createBindings(
            QQmlJSScope::scope(*this)->ownPropertyBindings(propertyName));
}

/*!
    Returns this Element's property bindings that have the name \a propertyName.
 */
QList<Binding> Element::propertyBindings(const QString &propertyName) const
{
    const auto &bindings = QQmlJSScope::scope(*this)->propertyBindings(propertyName);

    QList<Binding> saBindings;
    for (const auto &jsBinding : bindings) {
        saBindings.push_back(BindingPrivate::createBinding(jsBinding));
    }
    return saBindings;
}

QQmlSA::Binding::Bindings
BindingsPrivate::createBindings(const QMultiHash<QString, QQmlJSMetaPropertyBinding> &hash)
{
    QMultiHash<QString, QQmlSA::Binding> saBindings;
    for (const auto &[key, value] : hash.asKeyValueRange()) {
        saBindings.insert(key, BindingPrivate::createBinding(value));
    }

    QQmlSA::Binding::Bindings bindings;
    bindings.d_func()->m_bindings = std::move(saBindings);
    return bindings;
}

QQmlSA::Binding::Bindings BindingsPrivate::createBindings(
        QPair<QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator,
              QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator> iterators)
{
    QMultiHash<QString, QQmlSA::Binding> saBindings;
    for (auto it = iterators.first; it != iterators.second; ++it) {
        saBindings.insert(it.key(), BindingPrivate::createBinding(it.value()));
    }

    QQmlSA::Binding::Bindings bindings;
    bindings.d_func()->m_bindings = std::move(saBindings);
    return bindings;
}

/*!
    Returns an iterator to the beginning of this Element's children.
 */
QQmlJS::ConstPtrWrapperIterator Element::childScopesBegin() const
{
    return QQmlJSScope::scope(*this)->childScopesBegin();
}

/*!
    Returns an iterator to the end of this Element's children.
 */
QQmlJS::ConstPtrWrapperIterator Element::childScopesEnd() const
{
    return QQmlJSScope::scope(*this)->childScopesEnd();
}

Element::operator bool() const
{
    return bool(QQmlJSScope::scope(*this));
}

bool Element::operator!() const
{
    return !QQmlJSScope::scope(*this);
}

/*!
    Returns the name of this Element.
 */
QString Element::name() const
{
    if (isNull())
        return {};
    return QQmlJSScope::prettyName(QQmlJSScope::scope(*this)->internalName());
}

bool Element::operatorEqualsImpl(const Element &lhs, const Element &rhs)
{
    return QQmlJSScope::scope(lhs) == QQmlJSScope::scope(rhs);
}

qsizetype Element::qHashImpl(const Element &key, qsizetype seed) noexcept
{
    return qHash(QQmlJSScope::scope(key), seed);
}

/*!
    \class QQmlSA::GenericPass
    \inmodule QtQmlCompiler

    \brief The base class for static analysis passes.

    This class contains common functionality used by more specific passses.
    Custom passes should not directly derive from it, but rather from one of
    its subclasses.
    \sa ElementPass, PropertyPass
 */

class GenericPassPrivate {
    Q_DECLARE_PUBLIC(GenericPass);

public:
    GenericPassPrivate(GenericPass *interface, PassManager *manager)
        : m_manager{ manager }, q_ptr{ interface }
    {
        Q_ASSERT(manager);
    }

private:
    PassManager *m_manager;

    GenericPass *q_ptr;
};

GenericPass::~GenericPass() = default;

/*!
    Creates a generic pass.
 */
GenericPass::GenericPass(PassManager *manager)
    : d_ptr{ new GenericPassPrivate{ this, manager } } { }

/*!
    Emits a warning message \a diagnostic about an issue of type \a id.
 */
void GenericPass::emitWarning(QAnyStringView diagnostic, LoggerWarningId id)
{
    emitWarning(diagnostic, id, QQmlSA::SourceLocation{});
}

/*!
    Emits warning message \a diagnostic about an issue of type \a id located at
    \a srcLocation.
 */
void GenericPass::emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                              QQmlSA::SourceLocation srcLocation)
{
    Q_D(const GenericPass);
    PassManagerPrivate::visitor(*d->m_manager)
            ->logger()
            ->log(diagnostic.toString(), id,
                  QQmlSA::SourceLocationPrivate::sourceLocation(srcLocation));
}

/*!
    Emits a warning message \a diagnostic about an issue of type \a id located at
    \a srcLocation and with suggested fix \a fix.
 */
void GenericPass::emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                              QQmlSA::SourceLocation srcLocation, const QQmlSA::FixSuggestion &fix)
{
    Q_D(const GenericPass);
    PassManagerPrivate::visitor(*d->m_manager)
            ->logger()
            ->log(diagnostic.toString(), id,
                  QQmlSA::SourceLocationPrivate::sourceLocation(srcLocation), true, true,
                  FixSuggestionPrivate::fixSuggestion(fix));
}

/*!
    Returns the type corresponding to \a typeName inside the
    currently analysed file.
 */
Element GenericPass::resolveTypeInFileScope(QAnyStringView typeName)
{
    Q_D(const GenericPass);
    const auto scope =
            PassManagerPrivate::visitor(*d->m_manager)->imports().type(typeName.toString()).scope;
    return QQmlJSScope::createQQmlSAElement(scope);
}

/*!
    Returns the attached type corresponding to \a typeName used inside
    the currently analysed file.
 */
Element GenericPass::resolveAttachedInFileScope(QAnyStringView typeName)
{
    const auto type = resolveTypeInFileScope(typeName);
    const auto scope = QQmlJSScope::scope(type);

    if (scope.isNull())
        return QQmlJSScope::createQQmlSAElement(QQmlJSScope::ConstPtr(nullptr));

    return QQmlJSScope::createQQmlSAElement(scope->attachedType());
}

/*!
    Returns the type of \a typeName defined in module \a moduleName.
    If an attached type and and a non-attached type share the same
    name (e.g. \c ListView), the \l Element corresponding to the
    non-attached type is returned.
    To obtain the attached type, use \l resolveAttached.
 */
Element GenericPass::resolveType(QAnyStringView moduleName, QAnyStringView typeName)
{
    Q_D(const GenericPass);
    QQmlJSImporter *typeImporter = PassManagerPrivate::visitor(*d->m_manager)->importer();
    const auto module = typeImporter->importModule(moduleName.toString());
    const auto scope = module.type(typeName.toString()).scope;
    return QQmlJSScope::createQQmlSAElement(scope);
}

/*!
    Returns the type of the built-in type identified by \a typeName.
    Built-in types encompasses \c{C++} types which the  QML engine can handle
    without any imports (e.g. \l QDateTime and \l QString), global EcmaScript
    objects like \c Number, as well as the \l {QML Global Object}
    {global Qt object}.
 */
Element GenericPass::resolveBuiltinType(QAnyStringView typeName) const
{
    Q_D(const GenericPass);
    QQmlJSImporter *typeImporter = PassManagerPrivate::visitor(*d->m_manager)->importer();
    auto typeNameString = typeName.toString();
    // we have to check both cpp names
    auto scope = typeImporter->builtinInternalNames().type(typeNameString).scope;
    if (!scope) {
        // and qml names (e.g. for bool) - builtinImportHelper is private, so we can't do it in one call
        auto builtins = typeImporter->importBuiltins();
        scope = builtins.type(typeNameString).scope;
    }
    return QQmlJSScope::createQQmlSAElement(scope);
}

/*!
    Returns the attached type of \a typeName defined in module \a moduleName.
 */
Element GenericPass::resolveAttached(QAnyStringView moduleName, QAnyStringView typeName)
{
    const auto &resolvedType = resolveType(moduleName, typeName);
    return QQmlJSScope::createQQmlSAElement(QQmlJSScope::scope(resolvedType)->attachedType());
}

/*!
    Returns the element representing the type of literal in \a binding. If the
    binding does not contain a literal value, a null Element is returned.
 */
Element GenericPass::resolveLiteralType(const QQmlSA::Binding &binding)
{
    Q_D(const GenericPass);
    return binding.literalType(PassManagerPrivate::resolver(*d->m_manager));
}

/*!
    Returns the element in \a context that has id \a id.
 */
Element GenericPass::resolveIdToElement(QAnyStringView id, const Element &context)
{
    Q_D(const GenericPass);
    const auto scope = PassManagerPrivate::visitor(*d->m_manager)
                               ->addressableScopes()
                               .scope(id.toString(), QQmlJSScope::scope(context));
    return QQmlJSScope::createQQmlSAElement(scope);
}

/*!
    Returns the id of \a element in a given \a context.
 */
QString GenericPass::resolveElementToId(const Element &element, const Element &context)
{
    Q_D(const GenericPass);
    return PassManagerPrivate::visitor(*d->m_manager)
            ->addressableScopes()
            .id(QQmlJSScope::scope(element), QQmlJSScope::scope(context));
}

/*!
    Returns the source code located within \a location.
 */
QString GenericPass::sourceCode(QQmlSA::SourceLocation location)
{
    Q_D(const GenericPass);
    return PassManagerPrivate::visitor(*d->m_manager)
            ->logger()
            ->code()
            .mid(location.offset(), location.length());
}

/*!
    \class QQmlSA::PassManager
    \inmodule QtQmlCompiler

    \brief Can analyze an element and its children with static analysis passes.
 */

/*!
    Constructs a pass manager given an import \a visitor and a type \a resolver.
 */
QQmlSA::PassManager::PassManager(QQmlJSImportVisitor *visitor, QQmlJSTypeResolver *resolver)
    : d_ptr{ new PassManagerPrivate{ this, visitor, resolver } }
{
}

PassManager::~PassManager() = default; // explicitly defaulted out-of-line for PIMPL

/*!
    Registers a static analysis \a pass to be run on all elements.
 */
void PassManager::registerElementPass(std::unique_ptr<ElementPass> pass)
{
    Q_D(PassManager);
    d->registerElementPass(std::move(pass));
}

/*!
   \internal
   \brief PassManager::registerElementPass registers ElementPass
          with the pass manager.
   \param pass The registered pass. Ownership is transferred to the pass manager.
 */
void PassManagerPrivate::registerElementPass(std::unique_ptr<ElementPass> pass)
{
    m_elementPasses.push_back(std::move(pass));
}

enum LookupMode { Register, Lookup };
static QString lookupName(const QQmlSA::Element &element, LookupMode mode = Lookup)
{
    QString name;
    if (element.isNull() || QQmlJSScope::scope(element)->internalName().isEmpty()) {
        // Bail out with an invalid name, this type is so screwed up we can't do anything reasonable
        // with it We should have warned about it in another plac
        if (element.isNull() || element.baseType().isNull())
            return u"$INVALID$"_s;
        name = QQmlJSScope::scope(element.baseType())->internalName();
    } else {
        name = QQmlJSScope::scope(element)->internalName();
    }

    const QString filePath =
            (mode == Register || !element.baseType() ? element : element.baseType()).filePath();

    if (QQmlJSScope::scope(element)->isComposite() && !filePath.endsWith(u".h"))
        name += u'@' + filePath;
    return name;
}

/*!
    Registers a static analysis pass for properties. The \a pass will be run on
    every property matching the \a moduleName, \a typeName and \a propertyName.

    Omitting the \a propertyName will register this pass for all properties
    matching the \a typeName and \a moduleName.

    Setting \a allowInheritance to \c true means that the filtering on the type
    also accepts types deriving from \a typeName.

    \a pass is passed as a \c{std::shared_ptr} to allow reusing the same pass
    on multiple elements:
    \code
    auto titleValiadorPass = std::make_shared<TitleValidatorPass>(manager);
    manager->registerPropertyPass(titleValidatorPass,
                                  "QtQuick", "Window", "title");
    manager->registerPropertyPass(titleValidatorPass,
                                  "QtQuick.Controls", "Dialog", "title");
    \endcode

    \note Running analysis passes on too many items can be expensive. This is
    why it is generally good to filter down the set of properties of a pass
    using the \a moduleName, \a typeName and \a propertyName.

    Returns \c true if the pass was successfully added, \c false otherwise.
    Adding a pass fails when the \l{QQmlSA::Element}{Element} specified by
    \a moduleName and \a typeName does not exist.

    \sa PropertyPass
*/
bool PassManager::registerPropertyPass(std::shared_ptr<PropertyPass> pass,
                                       QAnyStringView moduleName, QAnyStringView typeName,
                                       QAnyStringView propertyName, bool allowInheritance)
{
    Q_D(PassManager);
    return d->registerPropertyPass(pass, moduleName, typeName, propertyName, allowInheritance);
}

bool PassManagerPrivate::registerPropertyPass(std::shared_ptr<PropertyPass> pass,
                                              QAnyStringView moduleName, QAnyStringView typeName,
                                              QAnyStringView propertyName, bool allowInheritance)
{
    if (moduleName.isEmpty() != typeName.isEmpty()) {
        qWarning() << "Both the moduleName and the typeName must be specified "
                      "for the pass to be registered for a specific element.";
    }

    QString name;
    if (!moduleName.isEmpty() && !typeName.isEmpty()) {
        auto typeImporter = m_visitor->importer();
        auto module = typeImporter->importModule(moduleName.toString());
        auto element = QQmlJSScope::createQQmlSAElement(module.type(typeName.toString()).scope);

        if (element.isNull())
            return false;

        name = lookupName(element, Register);
    }
    const QQmlSA::PropertyPassInfo passInfo{ propertyName.isEmpty()
                                                     ? QStringList{}
                                                     : QStringList{ propertyName.toString() },
                                             std::move(pass), allowInheritance };
    m_propertyPasses.insert({ name, passInfo });

    return true;
}

void PassManagerPrivate::addBindingSourceLocations(const Element &element, const Element &scope,
                                                   const QString prefix, bool isAttached)
{
    const Element &currentScope = scope.isNull() ? element : scope;
    const auto ownBindings = currentScope.ownPropertyBindings();
    for (const auto &binding : ownBindings) {
        switch (binding.bindingType()) {
        case QQmlSA::BindingType::GroupProperty:
            addBindingSourceLocations(element, Element{ binding.groupType() },
                                      prefix + binding.propertyName() + u'.');
            break;
        case QQmlSA::BindingType::AttachedProperty:
            addBindingSourceLocations(element, Element{ binding.attachingType() },
                                      prefix + binding.propertyName() + u'.', true);
            break;
        default:
            m_bindingsByLocation.insert({ binding.sourceLocation().offset(),
                                          BindingInfo{ prefix + binding.propertyName(), binding,
                                                       currentScope, isAttached } });

            if (binding.bindingType() != QQmlSA::BindingType::Script)
                analyzeBinding(element, QQmlSA::Element(), binding.sourceLocation());
        }
    }
}

/*!
    Runs the element passes over \a root and all its children.
 */
void PassManager::analyze(const Element &root)
{
    Q_D(PassManager);
    d->analyze(root);
}

void PassManagerPrivate::analyze(const Element &root)
{
    QList<Element> runStack;
    runStack.push_back(root);
    while (!runStack.isEmpty()) {
        auto element = runStack.takeLast();
        addBindingSourceLocations(element);
        for (auto &elementPass : m_elementPasses)
            if (elementPass->shouldRun(element))
                elementPass->run(element);

        for (auto it = element.childScopesBegin(); it != element.childScopesEnd(); ++it) {
            if ((*it)->scopeType() == QQmlSA::ScopeType::QMLScope)
                runStack.push_back(QQmlJSScope::createQQmlSAElement(*it));
        }
    }
}

void PassManagerPrivate::analyzeWrite(const Element &element, QString propertyName,
                                      const Element &value, const Element &writeScope,
                                      QQmlSA::SourceLocation location)
{
    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onWrite(element, propertyName, value, writeScope, location);
}

void PassManagerPrivate::analyzeRead(const Element &element, QString propertyName,
                                     const Element &readScope, QQmlSA::SourceLocation location)
{
    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onRead(element, propertyName, readScope, location);
}

void PassManagerPrivate::analyzeBinding(const Element &element, const QQmlSA::Element &value,
                                        QQmlSA::SourceLocation location)
{
    const auto info = m_bindingsByLocation.find(location.offset());

    // If there's no matching binding that means we're in a nested Ret somewhere inside an
    // expression
    if (info == m_bindingsByLocation.end())
        return;

    const QQmlSA::Element &bindingScope = info->second.bindingScope;
    const QQmlSA::Binding &binding = info->second.binding;
    const QString &propertyName = info->second.fullPropertyName;

    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onBinding(element, propertyName, binding, bindingScope, value);

    if (!info->second.isAttached || bindingScope.baseType().isNull())
        return;

    for (PropertyPass *pass : findPropertyUsePasses(bindingScope.baseType(), propertyName))
        pass->onBinding(element, propertyName, binding, bindingScope, value);
}

/*!
    Returns \c true if the module named \a module has been imported by the
    QML to be analyzed, \c false otherwise.

    This can be used to skip registering a pass which is specific to a specific
    module.

    \code
    if (passManager->hasImportedModule("QtPositioning"))
        passManager->registerElementPass(
           std::make_unique<PositioningPass>(passManager)
        );
    \endcode

    \sa registerPropertyPass(), registerElementPass()
 */
bool PassManager::hasImportedModule(QAnyStringView module) const
{
    return PassManagerPrivate::visitor(*this)->imports().hasType(u"$module$." + module.toString());
}

/*!
    Returns \c true if warnings of \a category are enabled, \c false otherwise.
 */
bool PassManager::isCategoryEnabled(LoggerWarningId category) const
{
    return !PassManagerPrivate::visitor(*this)->logger()->isCategoryIgnored(category);
}

QQmlJSImportVisitor *QQmlSA::PassManagerPrivate::visitor(const QQmlSA::PassManager &manager)
{
    return manager.d_func()->m_visitor;
}

QQmlJSTypeResolver *QQmlSA::PassManagerPrivate::resolver(const QQmlSA::PassManager &manager)
{
    return manager.d_func()->m_typeResolver;
}

QSet<PropertyPass *> PassManagerPrivate::findPropertyUsePasses(const QQmlSA::Element &element,
                                                               const QString &propertyName)
{
    QStringList typeNames { lookupName(element) };

    QQmlJSUtils::searchBaseAndExtensionTypes(
            QQmlJSScope::scope(element),
            [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
                Q_UNUSED(mode);
                typeNames.append(lookupName(QQmlJSScope::createQQmlSAElement(scope)));
                return false;
            });

    QSet<PropertyPass *> passes;

    for (const QString &typeName : typeNames) {
        for (auto &pass :
             { m_propertyPasses.equal_range(u""_s), m_propertyPasses.equal_range(typeName) }) {
            if (pass.first == pass.second)
                continue;

            for (auto it = pass.first; it != pass.second; it++) {
                if (typeName != typeNames.constFirst() && !it->second.allowInheritance)
                    continue;
                if (it->second.properties.isEmpty()
                    || it->second.properties.contains(propertyName)) {
                    passes.insert(it->second.pass.get());
                }
            }
        }
    }
    return passes;
}

void DebugElementPass::run(const Element &element) {
    emitWarning(u"Type: " + element.baseTypeName(), qmlPlugin);
    if (auto bindings = element.propertyBindings(u"objectName"_s); !bindings.isEmpty()) {
        emitWarning(u"is named: " + bindings.first().stringValue(), qmlPlugin);
    }
    if (auto defPropName = element.defaultPropertyName(); !defPropName.isEmpty()) {
        emitWarning(u"binding " + QString::number(element.propertyBindings(defPropName).size())
                            + u" elements to property "_s + defPropName,
                    qmlPlugin);
    }
}

/*!
    \class QQmlSA::LintPlugin
    \inmodule QtQmlCompiler

    \brief Base class for all static analysis plugins.
 */

/*!
    \fn void QQmlSA::LintPlugin::registerPasses(PassManager *manager, const Element &rootElement)

    Adds a pass \a manager that will be executed on \a rootElement.
 */

/*!
    \class QQmlSA::ElementPass
    \inmodule QtQmlCompiler

    \brief Base class for all static analysis passes on elements.

    ElementPass is the simpler of the two analysis passes. It will consider every element in
    a file. The \l shouldRun() method can be used to filter out irrelevant elements, and the
    \l run() method is doing the initial work.

    Common tasks suitable for an ElementPass are
    \list
    \li checking that properties of an Element are not combined in a nonsensical way
    \li validating property values (e.g. that a property takes only certain enum values)
    \li checking behavior dependent on an Element's parent (e.g. not using \l {Item::width}
        when the parent element is a \c Layout).
    \endlist

    As shown in the snippet below, it is recommended to do necessary type resolution in the
    constructor of the ElementPass and cache it in local members, and to implement some
    filtering via \l shouldRun() to keep the static analysis performant.

    \code
    using namespace QQmlSA;
    class MyElementPass : public ElementPass
    {
        Element myType;
        public:
            MyElementPass(QQmlSA::PassManager *manager)
            : myType(resolveType("MyModule", "MyType")) {}

            bool shouldRun(const Element &element) override
            {
                return element.inherits(myType);
            }
            void run(const Element &element) override
            {
                // actual pass logic
            }
    }
    \endcode

    ElementPasses have limited insight into how an element's properties are used. If you need
    that information, consider using a \l PropertyPass instead.

    \note ElementPass will only ever consider instantiable types. Therefore, it is unsuitable
    to analyze attached types and singletons. Those need to be handled via a PropertyPass.
 */

/*!
    \fn void QQmlSA::ElementPass::run(const Element &element)

    Executes if \c shouldRun() returns \c true. Performs the real computation
    of the pass on \a element.
    This method is meant to be overridden. Calling the base method is not
    necessary.
 */

/*!
    Controls whether the \c run() function should be executed on the given \a element.
    Subclasses can override this method to improve performance of the analysis by
    filtering out elements which are not relevant.

    The default implementation unconditionally returns \c true.
 */
bool ElementPass::shouldRun(const Element &element)
{
    (void)element;
    return true;
}

/*!
    \class QQmlSA::PropertyPass
    \inmodule QtQmlCompiler

    \brief Base class for all static analysis passes on properties.
 */


PropertyPass::PropertyPass(PassManager *manager) : GenericPass(manager) { }
/*!
    Executes whenever a property gets bound to a value.

    The property \a propertyName of \a element is bound to the \a value within
    \a bindingScope with \a binding.
 */
void PropertyPass::onBinding(const Element &element, const QString &propertyName,
                             const QQmlSA::Binding &binding, const Element &bindingScope,
                             const Element &value)
{
    Q_UNUSED(element);
    Q_UNUSED(propertyName);
    Q_UNUSED(binding);
    Q_UNUSED(bindingScope);
    Q_UNUSED(value);
}

/*!
    Executes whenever a property is read.

    The property \a propertyName of \a element is read by an instruction within
    \a readScope defined at \a location.
 */
void PropertyPass::onRead(const Element &element, const QString &propertyName,
                          const Element &readScope, QQmlSA::SourceLocation location)
{
    Q_UNUSED(element);
    Q_UNUSED(propertyName);
    Q_UNUSED(readScope);
    Q_UNUSED(location);
}

/*!
    Executes whenever a property is written to.

    The property \a propertyName of \a element is written to by an instruction
    within \a writeScope defined at \a location. The type of the expression
    written to \a propertyName is \a expressionType.
 */
void PropertyPass::onWrite(const Element &element, const QString &propertyName,
                           const Element &expressionType, const Element &writeScope,
                           QQmlSA::SourceLocation location)
{
    Q_UNUSED(element);
    Q_UNUSED(propertyName);
    Q_UNUSED(writeScope);
    Q_UNUSED(expressionType);
    Q_UNUSED(location);
}

DebugPropertyPass::DebugPropertyPass(QQmlSA::PassManager *manager) : QQmlSA::PropertyPass(manager)
{
}

void DebugPropertyPass::onRead(const QQmlSA::Element &element, const QString &propertyName,
                               const QQmlSA::Element &readScope, QQmlSA::SourceLocation location)
{
    emitWarning(u"onRead "_s
                        + (QQmlJSScope::scope(element)->internalName().isEmpty()
                                   ? element.baseTypeName()
                                   : QQmlJSScope::scope(element)->internalName())
                        + u' ' + propertyName + u' ' + QQmlJSScope::scope(readScope)->internalName()
                        + u' ' + QString::number(location.startLine()) + u':'
                        + QString::number(location.startColumn()),
                qmlPlugin, location);
}

void DebugPropertyPass::onBinding(const QQmlSA::Element &element, const QString &propertyName,
                                  const QQmlSA::Binding &binding,
                                  const QQmlSA::Element &bindingScope, const QQmlSA::Element &value)
{
    const auto location = QQmlSA::SourceLocation{ binding.sourceLocation() };
    emitWarning(u"onBinding element: '"_s
                        + (QQmlJSScope::scope(element)->internalName().isEmpty()
                                   ? element.baseTypeName()
                                   : QQmlJSScope::scope(element)->internalName())
                        + u"' property: '"_s + propertyName + u"' value: '"_s
                        + (value.isNull() ? u"NULL"_s
                                          : (QQmlJSScope::scope(value)->internalName().isNull()
                                                     ? value.baseTypeName()
                                                     : QQmlJSScope::scope(value)->internalName()))
                        + u"' binding_scope: '"_s
                        + (QQmlJSScope::scope(bindingScope)->internalName().isEmpty()
                                   ? bindingScope.baseTypeName()
                                   : QQmlJSScope::scope(bindingScope)->internalName())
                        + u"' "_s + QString::number(location.startLine()) + u':'
                        + QString::number(location.startColumn()),
                qmlPlugin, location);
}

void DebugPropertyPass::onWrite(const QQmlSA::Element &element, const QString &propertyName,
                                const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                                QQmlSA::SourceLocation location)
{
    emitWarning(u"onWrite "_s + element.baseTypeName() + u' ' + propertyName + u' '
                        + QQmlJSScope::scope(value)->internalName() + u' '
                        + QQmlJSScope::scope(writeScope)->internalName() + u' '
                        + QString::number(location.startLine()) + u':'
                        + QString::number(location.startColumn()),
                qmlPlugin, location);
}

/*!
    Returns the list of element passes.
 */
std::vector<std::shared_ptr<ElementPass>> PassManager::elementPasses() const
{
    Q_D(const PassManager);
    return d->m_elementPasses;
}

/*!
    Returns the list of property passes.
 */
std::multimap<QString, PropertyPassInfo> PassManager::propertyPasses() const
{
    Q_D(const PassManager);
    return d->m_propertyPasses;
}

/*!
    Returns bindings by their source location.
 */
std::unordered_map<quint32, BindingInfo> PassManager::bindingsByLocation() const
{
    Q_D(const PassManager);
    return d->m_bindingsByLocation;
}

FixSuggestionPrivate::FixSuggestionPrivate(FixSuggestion *interface) : q_ptr{ interface } { }

FixSuggestionPrivate::FixSuggestionPrivate(FixSuggestion *interface, const QString &fixDescription,
                                           const QQmlSA::SourceLocation &location,
                                           const QString &replacement)
    : m_fixSuggestion{ fixDescription, QQmlSA::SourceLocationPrivate::sourceLocation(location),
                       replacement },
      q_ptr{ interface }
{
}

FixSuggestionPrivate::FixSuggestionPrivate(FixSuggestion *interface,
                                           const FixSuggestionPrivate &other)
    : m_fixSuggestion{ other.m_fixSuggestion }, q_ptr{ interface }
{
}

FixSuggestionPrivate::FixSuggestionPrivate(FixSuggestion *interface, FixSuggestionPrivate &&other)
    : m_fixSuggestion{ std::move(other.m_fixSuggestion) }, q_ptr{ interface }
{
}

QString FixSuggestionPrivate::fixDescription() const
{
    return m_fixSuggestion.fixDescription();
}

QQmlSA::SourceLocation FixSuggestionPrivate::location() const
{
    return QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(m_fixSuggestion.location());
}

QString FixSuggestionPrivate::replacement() const
{
    return m_fixSuggestion.replacement();
}

void FixSuggestionPrivate::setFileName(const QString &fileName)
{
    m_fixSuggestion.setFilename(fileName);
}

QString FixSuggestionPrivate::fileName() const
{
    return m_fixSuggestion.filename();
}

void FixSuggestionPrivate::setHint(const QString &hint)
{
    m_fixSuggestion.setHint(hint);
}

QString FixSuggestionPrivate::hint() const
{
    return m_fixSuggestion.hint();
}

void FixSuggestionPrivate::setAutoApplicable(bool autoApplicable)
{
    m_fixSuggestion.setAutoApplicable(autoApplicable);
}

bool FixSuggestionPrivate::isAutoApplicable() const
{
    return m_fixSuggestion.isAutoApplicable();
}

QQmlJSFixSuggestion &FixSuggestionPrivate::fixSuggestion(FixSuggestion &saFixSuggestion)
{
    return saFixSuggestion.d_func()->m_fixSuggestion;
}

const QQmlJSFixSuggestion &FixSuggestionPrivate::fixSuggestion(const FixSuggestion &saFixSuggestion)
{
    return saFixSuggestion.d_func()->m_fixSuggestion;
}

/*!
    \class QQmlSA::FixSuggestion
    \inmodule QtQmlCompiler

    \brief Represents a suggested fix for an issue in the source code.
 */


FixSuggestion::FixSuggestion(const QString &fixDescription, const QQmlSA::SourceLocation &location,
                             const QString &replacement)
    : d_ptr{ new FixSuggestionPrivate{ this, fixDescription, location, replacement } }
{
}

FixSuggestion::FixSuggestion(const FixSuggestion &other)
    : d_ptr{ new FixSuggestionPrivate{ this, *other.d_func() } }
{
}

FixSuggestion::FixSuggestion(FixSuggestion &&other) noexcept
    : d_ptr{ new FixSuggestionPrivate{ this, std::move(*other.d_func()) } }
{
}

FixSuggestion &FixSuggestion::operator=(const FixSuggestion &other)
{
    if (*this == other)
        return *this;

    d_func()->m_fixSuggestion = other.d_func()->m_fixSuggestion;
    return *this;
}

FixSuggestion &FixSuggestion::operator=(FixSuggestion &&other) noexcept
{
    if (*this == other)
        return *this;

    d_func()->m_fixSuggestion = std::move(other.d_func()->m_fixSuggestion);
    return *this;
}

FixSuggestion::~FixSuggestion() = default;

/*!
    Returns the description of the fix.
 */
QString QQmlSA::FixSuggestion::fixDescription() const
{
    return FixSuggestionPrivate::fixSuggestion(*this).fixDescription();
}

/*!
    Returns the location where the fix would be applied.
 */
QQmlSA::SourceLocation FixSuggestion::location() const
{
    return QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
            FixSuggestionPrivate::fixSuggestion(*this).location());
}

/*!
    Returns the fix that will replace the problematic source code.
 */
QString FixSuggestion::replacement() const
{
    return FixSuggestionPrivate::fixSuggestion(*this).replacement();
}

/*!
    Sets \a fileName as the name of the file where this fix suggestion applies.
 */
void FixSuggestion::setFileName(const QString &fileName)
{
    FixSuggestionPrivate::fixSuggestion(*this).setFilename(fileName);
}

/*!
    Returns the name of the file where this fix suggestion applies.
 */
QString FixSuggestion::fileName() const
{
    return FixSuggestionPrivate::fixSuggestion(*this).filename();
}

/*!
    Sets \a hint as the hint for this fix suggestion.
 */
void FixSuggestion::setHint(const QString &hint)
{
    FixSuggestionPrivate::fixSuggestion(*this).setHint(hint);
}

/*!
    Returns the hint for this fix suggestion.
 */
QString FixSuggestion::hint() const
{
    return FixSuggestionPrivate::fixSuggestion(*this).hint();
}

/*!
    Sets uses \a autoApplicable to set whtether this suggested fix can be applied automatically.
 */
void FixSuggestion::setAutoApplicable(bool autoApplicable)
{
    return FixSuggestionPrivate::fixSuggestion(*this).setAutoApplicable(autoApplicable);
}

/*!
    Returns whether this suggested fix can be applied automatically.
 */
bool QQmlSA::FixSuggestion::isAutoApplicable() const
{
    return FixSuggestionPrivate::fixSuggestion(*this).isAutoApplicable();
}

bool FixSuggestion::operatorEqualsImpl(const FixSuggestion &lhs, const FixSuggestion &rhs)
{
    return lhs.d_func()->m_fixSuggestion == rhs.d_func()->m_fixSuggestion;
}

} // namespace QQmlSA

QT_END_NAMESPACE
