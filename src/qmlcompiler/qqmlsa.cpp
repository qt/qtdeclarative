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
    \enum QQmlSA::MethodType
    \inmodule QtQmlCompiler

    \brief Describes the type of a \l{QQmlSA::Method}.
    \value Signal       The method is a signal
    \value Slot         The method is a slot
    \value Method       The method is a \l{Q_INVOKABLE} method
    \value StaticMethod The method is a \l{Q_INVOKABLE} static method
*/

/*!
    \enum QQmlSA::AccessSemantics
    \inmodule QtQmlCompiler

    \brief Describes how a type is accessed and shared.
    \value Reference The type behaves like an \l{QML Object Types}{Object type}
    \value Value     The type behaves like a \l{QML Value Types}{Value type}
    \value None      The type is a \l{QML Namespaces}{namespace}, or is invalid
    \value Sequence  The type behaves like a \l{QML Sequence Types}{Sequence type}

    \sa {The QML Type System}
*/

/*!
    \enum QQmlSA::BindingType
    \inmodule QtQmlCompiler

    \brief Describes the type of a \l{QQmlSA::Binding}.
    \value Invalid          There is no binding
    \value BoolLiteral      The binding is a bool literal
    \value NumberLiteral    The binding is a number literal
    \value StringLiteral    The binding is a string literal
    \value RegExpLiteral    The binding is a regular expression literal
    \value Null             The binding is a null literal
    \value Translation      The binding is a \l{Text ID based translations}{translation}
    \value TranslationById  The binding is a \l{Text ID based translations}{translation} by id
    \value Script           The binding is a regular script
    \value Object           The binging is an \l{QML Object Types}{Object}
    \value Interceptor      The binding is an interceptor that can intercept writes to properties such as \l[Quick]{Behavior}
    \value ValueSource      The binging is a \l{Defining QML Types from C++#Property Value Sources}{property value source}
    \value AttachedProperty The binding is an \l{QML Object Attributes#Attached Properties and Attached Signal Handlers}{attached object}
    \value GroupProperty    The binding is a \l{QML Object Attributes#Grouped Properties}{grouped property}
*/

/*!
    \enum QQmlSA::ScriptBindingKind
    \inmodule QtQmlCompiler

    \brief Describes the script type of a \l{QQmlSA::Binding} of type \l{Script}.
    \value Invalid         The binding has an invalid script
    \value PropertyBinding The binding is bound to a property
    \value SignalHandler   The binding is a \l{Signal and Handler Event System#Receiving signals with signal handlers}{signal handler}
    \value ChangeHandler   The binding is a \l{Signal and Handler Event System#Property change signal handlers}{change handler}
*/

/*!
    \enum QQmlSA::ScopeType
    \brief Describes the type of QML scope.
    \value JSFunctionScope          The scope is a JavaScript function:
                                    \badcode
                                    Item {
                                        function f() : int { <- begin
                                            return 1
                                        } <- end
                                    }
                                    \endcode
    \value JSLexicalScope           The scope is a JavaScript lexical scope:
                                    \badcode
                                    property int i: { <- begin
                                        let a = 1
                                        { <- begin
                                            console.log("hello")
                                        } <- end
                                        return a
                                    } <- end
                                    \endcode
    \value QMLScope                 The scope is a QML Object:
                                    \badcode
                                    Item { <- begin
                                        x: 50
                                    } <- end
                                    \endcode
    \value GroupedPropertyScope     The scope is a \l{QML Object Attributes#Grouped Properties}{grouped property}:
                                    \badcode
                                    Text {
                                        font { <- begin
                                            pixelSize: 12
                                            bold: true
                                        } <- end
                                    }
                                    \endcode
    \value AttachedPropertyScope    The scope is an \l{QML Object Attributes#Attached Properties and Attached Signal Handlers}{attached property}:
                                    \badcode
                                    Item {
                                        Component.onCompleted: console.log("Hello")
                                        ^^^^^^^^^
                                                 \ Scope of attached property Component
                                    }
                                    \endcode
    \value EnumScope                The scope is a QML \l{QML Enumerations}{enum}:
                                    \badcode
                                    enum E { <- begin
                                        A,
                                        B,
                                        C
                                    } <- end
                                    \endcode

    Each entry is shown with an example scope of the matching type in QML code.
*/

/*!
    \class QQmlSA::Binding::Bindings
    \inmodule QtQmlCompiler

    \brief Holds multiple property name to property binding associations.
 */

/*!
    Constructs a new Bindings object.
 */
Binding::Bindings::Bindings() : d_ptr{ new BindingsPrivate{ this } } { }

BindingsPrivate::BindingsPrivate(QQmlSA::Binding::Bindings *interface) : q_ptr{ interface } { }

/*!
    Creates a copy of \a other.
 */
Binding::Bindings::Bindings(const Bindings &other)
    : d_ptr{ new BindingsPrivate{ this, *other.d_func() } }
{
}

/*!
    Destroys the Bindings object.
 */
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

/*!
    \fn QMultiHash<QString, Binding>::const_iterator Binding::Bindings::begin() const
    Same as constBegin().
 */

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

/*!
    \fn QMultiHash<QString, Binding>::const_iterator Binding::Bindings::end() const
    Same as constEnd().
 */

QMultiHash<QString, Binding>::const_iterator BindingsPrivate::constEnd() const
{
    return m_bindings.constEnd();
}

/*!
    \class QQmlSA::Binding
    \inmodule QtQmlCompiler

    \brief Represents a single QML property binding for a specific type.
 */

/*!
    Constructs a new Binding object.
 */
Binding::Binding() : d_ptr{ new BindingPrivate{ this } } { }

BindingPrivate::BindingPrivate(Binding *interface) : q_ptr{ interface } { }

/*!
    Creates a copy of \a other.
*/
Binding::Binding(const Binding &other) : d_ptr{ new BindingPrivate{ this, *other.d_func() } } { }

/*!
    Move-constructs a \c Binding instance.
*/
Binding::Binding(Binding &&other) noexcept
    : d_ptr{ new BindingPrivate{ this, *other.d_func() } } { }


/*!
    Assigns \a other to this Binding instance.
*/
Binding &Binding::operator=(const Binding &other)
{
    if (*this == other)
        return *this;

    Q_D(Binding);
    d->m_binding = other.d_func()->m_binding;
    d->m_bindingScope = other.d_func()->m_bindingScope;
    d->q_ptr = this;
    d->m_isAttached = other.d_func()->m_isAttached;
    return *this;
}

/*!
    Move-assigns \a other to this Binding instance.
*/
Binding &Binding::operator=(Binding &&other) noexcept
{
    if (*this == other)
        return *this;

    Q_D(Binding);
    d->m_binding = std::move(other.d_func()->m_binding);
    d->m_bindingScope = std::move(other.d_func()->m_bindingScope);
    d->q_ptr = this;
    d->m_isAttached = other.d_func()->m_isAttached;
    return *this;
}

/*!
    Destroys the binding.
*/
Binding::~Binding() = default;

bool Binding::operatorEqualsImpl(const Binding &lhs, const Binding &rhs)
{
    return lhs.d_func()->m_binding == rhs.d_func()->m_binding
            && lhs.d_func()->m_bindingScope == rhs.d_func()->m_bindingScope
            && lhs.d_func()->m_isAttached == rhs.d_func()->m_isAttached;
}

BindingPrivate::BindingPrivate(Binding *interface, const BindingPrivate &other)
    : m_binding{ other.m_binding }, m_bindingScope{ other.m_bindingScope }, q_ptr{ interface },
      m_isAttached{ other.m_isAttached }
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
    Returns the type of the property of this binding if it is a group property,
    otherwise returns an invalid Element.
 */
Element Binding::groupType() const
{
    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(*this).groupType());
}

/*!
    Returns the Element scope in which the binding is defined.
 */
Element Binding::bindingScope() const
{
    return BindingPrivate::get(this)->m_bindingScope;
}

/*!
    Returns the type of this binding.
 */
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
    Returns the name of the property bound with this binding.
 */
QString Binding::propertyName() const
{
    return BindingPrivate::binding(*this).propertyName();
}

/*!
    Returns \c true if this type is attached to another one, \c false otherwise.
 */
bool Binding::isAttached() const
{
    return BindingPrivate::get(this)->m_isAttached;
}

/*!
    Returns the attached type if the content type of this binding is
    AttachedProperty, otherwise returns an invalid Element.
 */
Element Binding::attachedType() const
{
    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(*this).attachedType());
}

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    Returns the attached type if the content type of this binding is
    AttachedProperty, otherwise returns an invalid Element.

    \deprecated [6.9] Use the better named attachedType() method.
 */
Element Binding::attachingType() const
{
    return attachedType();
}
#endif

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
    Returns the kind of the associated script if the content type of this
    binding is Script, otherwise returns Invalid.
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
    binding is Object, otherwise returns an invalid Element.
 */
QQmlSA::Element Binding::objectType() const
{
    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(*this).objectType());
}

/*!
    Returns whether this binding has script value type undefined like when it
    is assigned \c undefined. If the content type of this binding is not
    \l{QQmlSA::BindingType::Script}, returns \c false.
 */
bool Binding::hasUndefinedScriptValue() const
{
    const auto &jsBinding = BindingPrivate::binding(*this);
    return jsBinding.bindingType() == BindingType::Script
            && jsBinding.scriptValueType() == ScriptValue_Undefined;
}

/*!
    Returns \c true if \a bindingType is a literal type, and \c false
    otherwise.
 */
bool QQmlSA::Binding::isLiteralBinding(QQmlSA::BindingType bindingType)
{
    return QQmlJSMetaPropertyBinding::isLiteralBinding(bindingType);
}

/*!
    \fn friend bool Binding::operator==(const Binding &lhs, const Binding &rhs)
    Returns \c true if \a lhs and \a rhs are equal, and \c false otherwise. Two
    \c Bindings are considered equal if their property name, content type, and
    source location match.
 */
/*!
    \fn friend bool Binding::operator!=(const Binding &lhs, const Binding &rhs)
    Returns \c true if \a lhs and \a rhs are not equal, and \c false otherwise.
    Two \c Bindings are considered equal if their property name, content type,
    and source location match.
 */

/*!
    Constructs a new Methods object.
*/
QQmlSA::Method::Methods::Methods() : d_ptr{ new MethodsPrivate{ this } } { }

/*!
    Creates a copy of \a other.
 */
QQmlSA::Method::Methods::Methods(const Methods &other)
    : d_ptr{ new MethodsPrivate{ this, *other.d_func() } }
{
}

/*!
    Destroys the Methods instance.
 */
QQmlSA::Method::Methods::~Methods() = default;

/*!
    Returns an iterator to the beginning of the methods.
 */
QMultiHash<QString, Method>::const_iterator Method::Methods::constBegin() const
{
    Q_D(const Methods);
    return d->constBegin();
}

/*!
    \fn QMultiHash<QString, QQmlSA::Method>::const_iterator QQmlSA::Method::Methods::begin() const
    Returns an iterator to the beginning of the methods.
 */

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

/*!
    \fn QMultiHash<QString, QQmlSA::Method>::const_iterator QQmlSA::Method::Methods::end() const
    Returns an iterator to the end of the methods.
 */

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

QQmlSA::SourceLocation MethodPrivate::sourceLocation() const
{
    return QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(m_method.sourceLocation());
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

/*!
    Constructs a new Method object.
 */
Method::Method() : d_ptr{ new MethodPrivate{ this } } { }

/*!
    Creates a copy of \a other.
 */
Method::Method(const Method &other) : d_ptr{ new MethodPrivate{ this, *other.d_func() } } { }

/*!
    Move-constructs a Method instance.
 */
Method::Method(Method &&other) noexcept
    : d_ptr{ new MethodPrivate{ this, std::move(*other.d_func()) } }
{
}

/*!
    Assigns \a other to this Method instance.
 */
Method &Method::operator=(const Method &other)
{
    if (*this == other)
        return *this;

    d_func()->m_method = other.d_func()->m_method;
    d_func()->q_ptr = this;
    return *this;
}

/*!
    Move-assigns \a other to this Method instance.
 */
Method &Method::operator=(Method &&other) noexcept
{
    if (*this == other)
        return *this;

    d_func()->m_method = std::move(other.d_func()->m_method);
    d_func()->q_ptr = this;
    return *this;
}

/*!
    Destroys the Method.
 */
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
    Returns the type of this method.
 */
MethodType Method::methodType() const
{
    Q_D(const Method);
    return d->methodType();
}

/*!
    \fn friend bool Method::operator==(const Method &lhs, const Method &rhs)
    Returns \c true if \a lhs and \a rhs are equal, and \c false otherwise.
 */
/*!
    \fn friend bool Method::operator!=(const Method &lhs, const Method &rhs)
    Returns \c true if \a lhs and \a rhs are not equal, and \c false otherwise.
 */

/*!
    Returns the location in the QML code where this method is defined.
 */
QQmlSA::SourceLocation Method::sourceLocation() const
{
    Q_D(const Method);
    return d->sourceLocation();
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

/*!
   Returns whether this property is readonly. Properties defined in QML are readonly when their
   definition has the 'readonly' keyword. Properties defined in C++ are readonly when they do not
   have a WRITE accessor function.
 */
bool PropertyPrivate::isReadonly() const
{
    return !m_property.isWritable();
}

/*!
    Returns the type that this property was defined with.
 */
QQmlSA::Element PropertyPrivate::type() const
{
    return QQmlJSScope::createQQmlSAElement(m_property.type());
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

/*!
    Constructs a new Property object.
 */
Property::Property() : d_ptr{ new PropertyPrivate{ this } } { }

/*!
    Creates a copy of \a other.
 */
Property::Property(const Property &other)
    : d_ptr{ new PropertyPrivate{ this, *other.d_func() } } { }

/*!
    Move-constructs a Property instance.
 */
Property::Property(Property &&other) noexcept
    : d_ptr{ new PropertyPrivate{ this, std::move(*other.d_func()) } }
{
}

/*!
    Assigns \a other to this Property instance.
 */
Property &Property::operator=(const Property &other)
{
    if (*this == other)
        return *this;

    d_func()->m_property = other.d_func()->m_property;
    d_func()->q_ptr = this;
    return *this;
}

/*!
    Move-assigns \a other to this Property instance.
 */
Property &Property::operator=(Property &&other) noexcept
{
    if (*this == other)
        return *this;

    d_func()->m_property = std::move(other.d_func()->m_property);
    d_func()->q_ptr = this;
    return *this;
}

/*!
    Destroys this property.
 */
Property::~Property() = default;

/*!
    Returns the name of the type of this property.
 */
QString Property::typeName() const
{
    Q_D(const Property);
    return d->typeName();
}

/*!
    Returns \c true if this property is valid, \c false otherwise.
 */
bool Property::isValid() const
{
    Q_D(const Property);
    return d->isValid();
}

/*!
    Returns \c true if this property is read-only, \c false otherwise.
 */
bool Property::isReadonly() const
{
    Q_D(const Property);
    return d->isReadonly();
}

/*!
    Returns the type of this property.
*/
QQmlSA::Element Property::type() const
{
    Q_D(const Property);
    return d->type();
}

/*!
    \fn friend bool Property::operator==(const Property &lhs, const Property &rhs)
    Returns \c true if \a lhs and \a rhs are equal, and \c false otherwise.
 */
/*!
    \fn friend bool Property::operator!=(const Property &lhs, const Property &rhs)
    Returns \c true if \a lhs and \a rhs are not equal, and \c false otherwise.
 */


bool Property::operatorEqualsImpl(const Property &lhs, const Property &rhs)
{
    return lhs.d_func()->m_property == rhs.d_func()->m_property;
}

/*!
    \class QQmlSA::Element
    \inmodule QtQmlCompiler

    \brief Represents a QML type.
 */

/*!
    Constructs a new Element object.
 */
Element::Element()
{
    new (m_data) QQmlJSScope::ConstPtr();
}

/*!
    Creates a copy of \a other.
 */
Element::Element(const Element &other)
{
    new (m_data) QQmlJSScope::ConstPtr(QQmlJSScope::scope(other));
}

/*!
    \fn Element::Element(Element &&other) noexcept
    Move-constructs an Element instance.
 */

/*!
    Assigns \a other to this element instance.
 */
Element &Element::operator=(const Element &other)
{
    if (this == &other)
        return *this;

    *reinterpret_cast<QQmlJSScope::ConstPtr *>(m_data) = QQmlJSScope::scope(other);
    return *this;
}

/*!
    \fn QQmlSA::Element &QQmlSA::Element::operator=(QQmlSA::Element &&other)
    Move-assigns \a other to this Element instance.
 */

/*!
    Destroys the element.
 */
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

/*!
    Returns whether this Element is the root component of its QML file.
 */
bool Element::isFileRootComponent() const
{
    return QQmlJSScope::scope(*this)->isFileRootComponent();
}

/*!
    Returns \c true if this element is null, \c false otherwise.
 */
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
    Returns \c true for objects defined from Qml, and \c false for objects declared from C++.
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
    Returns whether this Element defines a property with the name \a propertyName
    which is not defined on its base or extension objects.
 */
bool Element::hasOwnProperty(const QString &propertyName) const
{
    return QQmlJSScope::scope(*this)->hasOwnProperty(propertyName);
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
    Returns this Elements's methods, which are not defined on its base or
    extension objects.
 */
Method::Methods Element::ownMethods() const
{
    return MethodsPrivate::createMethods(QQmlJSScope::scope(*this)->ownMethods());
}

/*!
    Returns the location in the QML code where this Element is defined.
 */
QQmlSA::SourceLocation Element::sourceLocation() const
{
    return QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
            QQmlJSScope::scope(*this)->sourceLocation());
}

/*!
    Returns the file path of the QML code that defines this Element.
 */
QString Element::filePath() const
{
    return QQmlJSScope::scope(*this)->filePath();
}

/*!
    Returns whether this Element has a property binding with the name \a name.
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
    Returns \c true if this element is not null, \c false otherwise.
 */
Element::operator bool() const
{
    return bool(QQmlJSScope::scope(*this));
}

/*!
    Returns \c true if this element is null, \c false otherwise.
 */
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

/*!
    \fn friend inline bool Element::operator==(const Element &lhs, const Element &rhs)
    Returns \c true if \a lhs and \a rhs are equal, and \c false otherwise.
 */
/*!
    \fn friend inline bool Element::operator!=(const Element &lhs, const Element &rhs)
    Returns \c true if \a lhs and \a rhs are not equal, and \c false otherwise.
 */

bool Element::operatorEqualsImpl(const Element &lhs, const Element &rhs)
{
    return QQmlJSScope::scope(lhs) == QQmlJSScope::scope(rhs);
}

/*!
    \fn friend inline qsizetype Element::qHash(const Element &key, qsizetype seed) noexcept
    Returns the hash for \a key using \a seed to seed the calculation.
*/

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

/*!
    Creates a generic pass.
 */
GenericPass::GenericPass(PassManager *manager)
    : d_ptr{ new GenericPassPrivate{ this, manager } } { }

/*!
    Destroys the GenericPass instance.
 */
GenericPass::~GenericPass() = default;

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
    If an attached type and a non-attached type share the same name
    (for example, \c ListView), the \l Element corresponding to the
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
    Built-in types encompass \c{C++} types which the  QML engine can handle
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
        auto builtins = typeImporter->importHardCodedBuiltins();
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

    return QQmlJSScope::createQQmlSAElement(BindingPrivate::binding(binding).literalType(
            PassManagerPrivate::resolver(*d->m_manager)));
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

// explicitly defaulted out-of-line for PIMPL
PassManager::PassManager() = default;
PassManager::~PassManager() = default;

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
    const QQmlSA::PropertyPassInvocation passInfo{ propertyName.toString(), std::move(pass),
                                                   allowInheritance };
    m_propertyPasses.insert({ name, passInfo });

    return true;
}

void PassManagerPrivate::addBindingSourceLocations(const Element &element, const Element &scope,
                                                   const QString prefix, bool isAttached)
{
    const Element &currentScope = scope.isNull() ? element : scope;
    auto ownBindings = currentScope.ownPropertyBindings();
    for (auto &binding : ownBindings) {
        switch (binding.bindingType()) {
        case QQmlSA::BindingType::GroupProperty:
            addBindingSourceLocations(element, Element{ binding.groupType() },
                                      prefix + binding.propertyName() + u'.');
            continue; // don't insert into m_bindingsByLocation
        case QQmlSA::BindingType::AttachedProperty:
            addBindingSourceLocations(element, Element{ binding.attachedType() },
                                      prefix + binding.propertyName() + u'.', true);
            continue; // don't insert into m_bindingsByLocation
        case QQmlSA::BindingType::Translation: {
            analyzeCall(QQmlJSScope::createQQmlSAElement(m_typeResolver->jsGlobalObject()),
                        u"qsTr"_s, element, binding.sourceLocation());
            break;
        }
        case QQmlSA::BindingType::TranslationById: {
            analyzeCall(QQmlJSScope::createQQmlSAElement(m_typeResolver->jsGlobalObject()),
                        u"qsTrId"_s, element, binding.sourceLocation());
            break;
        }
        default:
            break;
        }

        BindingPrivate::get(&binding)->setBindingScope(currentScope);
        BindingPrivate::get(&binding)->setPropertyName(prefix + binding.propertyName());
        BindingPrivate::get(&binding)->setIsAttached(isAttached);
        m_bindingsByLocation.insert({ binding.sourceLocation().offset(), binding });
        if (binding.bindingType() != QQmlSA::BindingType::Script)
            analyzeBinding(element, QQmlSA::Element(), binding.sourceLocation());
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

static QQmlJS::ConstPtrWrapperIterator childScopesBegin(const Element &element)
{
    return QQmlJSScope::scope(element)->childScopesBegin();
}

static QQmlJS::ConstPtrWrapperIterator childScopesEnd(const Element &element)
{
    return QQmlJSScope::scope(element)->childScopesEnd();
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

        for (auto it = childScopesBegin(element), end = childScopesEnd(element); it != end; ++it) {
            if ((*it)->scopeType() == QQmlSA::ScopeType::QMLScope)
                runStack.push_back(QQmlJSScope::createQQmlSAElement(*it));
        }
    }
}

void PassManagerPrivate::analyzeWrite(const Element &element, const QString &propertyName,
                                      const Element &value, const Element &writeScope,
                                      const QQmlSA::SourceLocation &location)
{
    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onWrite(element, propertyName, value, writeScope, location);
}

void PassManagerPrivate::analyzeRead(const Element &element, const QString &propertyName,
                                     const Element &readScope, const QQmlSA::SourceLocation &location)
{
    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onRead(element, propertyName, readScope, location);
}

void PassManagerPrivate::analyzeCall(const Element &element, const QString &propertyName,
                                     const Element &readScope, const QQmlSA::SourceLocation &location)
{
    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onCall(element, propertyName, readScope, location);
}

void PassManagerPrivate::analyzeBinding(const Element &element, const QQmlSA::Element &value,
                                        const QQmlSA::SourceLocation &location)
{
    const auto it = m_bindingsByLocation.find(location.offset());

    // If there's no matching binding that means we're in a nested Ret somewhere inside an
    // expression
    if (it == m_bindingsByLocation.cend())
        return;

    const auto &[offset, binding] = *it;
    const QQmlSA::Element &bindingScope = binding.bindingScope();
    const QString &propertyName = binding.propertyName();

    const auto elementPasses = findPropertyUsePasses(element, propertyName);
    for (PropertyPass *pass : elementPasses)
        pass->onBinding(element, propertyName, binding, bindingScope, value);

    if (!binding.isAttached() || bindingScope.baseType().isNull())
        return;

    const auto bindingScopePasses = findPropertyUsePasses(bindingScope.baseType(), propertyName);
    for (PropertyPass *pass : bindingScopePasses) {
        if (!elementPasses.contains(pass))
            pass->onBinding(element, propertyName, binding, bindingScope, value);
    }
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
                if (it->second.property.isEmpty() || it->second.property == propertyName)
                    passes.insert(it->second.pass.get());
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
    \fn LintPlugin::LintPlugin()
    Constructs a LintPlugin object.
 */

/*!
    \fn virtual LintPlugin::~LintPlugin()
    Destroys the LintPlugin instance.
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
    \fn ElementPass::ElementPass(PassManager *manager)
    Creates an ElementPass object and uses \a manager to refer to the pass manager.
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


/*!
    Creates a PropertyPass object and uses \a manager to refer to the pass manager.
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

    This is also executed if the property \a propertyName is called as a function as that requires
    the property to be read first.
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
    Executes whenever a property or method is called.

    The property or method \a propertyName of \a element is called as a function by an instruction
    within \a readScope defined at \a location.

    \note Currently only direct calls of methods or properties are supported, indirect calls, for
    example by storing a method into a JavaScript variable and then calling the variable, are not
    recognized.
 */
void PropertyPass::onCall(const Element &element, const QString &propertyName,
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
    Returns bindings by their source location.
 */
std::unordered_map<quint32, Binding> PassManager::bindingsByLocation() const
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


/*!
    Creates a FixSuggestion object.
 */
FixSuggestion::FixSuggestion(const QString &fixDescription, const QQmlSA::SourceLocation &location,
                             const QString &replacement)
    : d_ptr{ new FixSuggestionPrivate{ this, fixDescription, location, replacement } }
{
}

/*!
    Creates a copy of \a other.
 */
FixSuggestion::FixSuggestion(const FixSuggestion &other)
    : d_ptr{ new FixSuggestionPrivate{ this, *other.d_func() } }
{
}

/*!
    Move-constructs a FixSuggestion instance.
 */
FixSuggestion::FixSuggestion(FixSuggestion &&other) noexcept
    : d_ptr{ new FixSuggestionPrivate{ this, std::move(*other.d_func()) } }
{
}

/*!
    Assigns \a other to this FixSuggestion instance.
 */
FixSuggestion &FixSuggestion::operator=(const FixSuggestion &other)
{
    if (*this == other)
        return *this;

    d_func()->m_fixSuggestion = other.d_func()->m_fixSuggestion;
    return *this;
}

/*!
    Move-assigns \a other to this FixSuggestion instance.
 */
FixSuggestion &FixSuggestion::operator=(FixSuggestion &&other) noexcept
{
    if (*this == other)
        return *this;

    d_func()->m_fixSuggestion = std::move(other.d_func()->m_fixSuggestion);
    return *this;
}

/*!
    Destorys the FixSuggestion instance.
 */
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
    Sets \a autoApplicable to determine whether this suggested fix can be
    applied automatically.
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

/*!
    \fn friend bool FixSuggestion::operator==(const FixSuggestion &lhs, const FixSuggestion &rhs)
    Returns \c true if \a lhs and \a rhs are equal, and \c false otherwise.
 */
/*!
    \fn friend bool FixSuggestion::operator!=(const FixSuggestion &lhs, const FixSuggestion &rhs)
    Returns \c true if \a lhs and \a rhs are not equal, and \c false otherwise.
 */

bool FixSuggestion::operatorEqualsImpl(const FixSuggestion &lhs, const FixSuggestion &rhs)
{
    return lhs.d_func()->m_fixSuggestion == rhs.d_func()->m_fixSuggestion;
}

bool isRegularBindingType(BindingType type)
{
    return type >= BindingType::BoolLiteral && type <= BindingType::Object;
}

} // namespace QQmlSA

QT_END_NAMESPACE
