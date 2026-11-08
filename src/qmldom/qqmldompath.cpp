// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldomitem_p.h"
#include "qqmldomerrormessage_p.h"
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QChar>

#include <algorithm>
#include <cstdint>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
class ErrorMessage;

namespace PathEls {

/*!
\internal
\class QQmlJS::Dom::Path

\brief Represents an immutable JsonPath like path in the Qml code model (from a DomItem to another
 DomItem)


\section1 Implementation Details

A Path is a pointer to the node holding its last component, plus the path length.
Each node (\c PathEls::PathNode) holds one \c PathComponent and a pointer to
the node of the component before it:

\badcode

                      __[.e] a.e (node e, length 2)
                     /
  [$root] <- [.a] <- [.b] <- [.c]        $root.a.b.c: (node c, length 4)
              ^
               \___ [.d]        $root.a.d: (node d, length 3)
\endcode

Nodes are immutable and reference counted.

The path consists of the \c m_length nodes ending in \c m_lastNode. Ancestors
beyond that are not part of the path, which makes \c dropFront() O(1).
\c dropTail(n) and \c mid() walk up from the tail, which is O(n) in the
number of dropped components. That's often cheap, as they mostly drop the
last one or two components.

As the links point backwards, accessing the front of a path is O(length).
Operations that traverse the components in order therefore first collect
the nodes into an array.

Each \c PathComponent owns the data it stores, there are no references to outside
data strutures.

\section1 Usage

It can be created with the path static functions or by modifying an existing
path. To parse a Path from its textual form, use \c parsePath() from
\c qqmldompathparser_p.h.
\code
Path imports = Path::fromRoot(PathRoot::Env)
                   .withField(Fields::qmlFilesByPath)
                   .withKey(QStringLiteral("/path/to/file"))
                   .withField(Fields::imports);
Path currentComponentImports = Path::fromCurrent(PathCurrent::Component)
                                   .withField(Fields::imports);
\endcode

This is a way to refer to elements in the Dom models that is not dependent from the source location,
and thus can also be used in visual tools.
A Path is quite stable toward reallocations or changes in the Dom model, and accessing it is safe
even when "dangling", thus it is a good long term reference to an element in the Dom model.

Path objects are a value type that have a shared pointer to extra data if needed, thus one should
use them as value objects.
The implementation has still optimization potential, but the behavior for the user should be already
the final one.

Path is both a range, and a single element (a bit like strings and characters in python).

The root contexts are:
\list
\li \l{$modules} All the known modules (even not imported), this is a global, rename independent
       reference
\li \l{$cpp} The Cpp names (namespaces, and Cpp types) visible in the current component
\li \l{$libs} The plugins/libraries and their contents
\li \l{$top} A top level entry in the DOM model, either $env or $universe (stepping in the universe
       one looses the reference to its environment)
\li \l{$env} The environment containing the currently available modules, i.e. the top level entry in
       the DOM model
\li \l{$universe} The dom unverse used by ths environment, and potentially shared with others that
       contains all the known parse entries, and also the latest, potentially invalid entries
\endlist

The current contexts are:
\list
\li \l{@obj} The current object (if in a map or list goes up until it is in the current object) .
\li \l{@component} The root object of the current component.
\li \l{@module} The current module instantiation.
\li \l{@ids} The ids in the current component.
\li \l{@types} All the types in the current component (reachable through imports, respecting
       renames)
\li \l{@lookupStrict} The strict lookup inside the current object: localJS, ids, properties, proto
       properties, component, its properties, global context, oterwise error
\li \l{@lookupDynamic} The default lookup inside the current object: localJS, ids, properties, proto
       properties, component, its properties, global context, ..
\li \l{@lookup} Either lookupStrict or lookupDynamic depending on the current component and context.
\endlist
 */

void Base::dump(const Sink &sink, const QString &name, bool hasSquareBrackets) const {
    if (hasSquareBrackets)
        sink(u"[");
    sink(name);
    if (hasSquareBrackets)
        sink(u"]");
}

int PathComponent::cmp(const PathComponent &p1, const PathComponent &p2)
{
    int k1 = static_cast<int>(p1.kind());
    int k2 = static_cast<int>(p2.kind());
    if (k1 < k2)
        return -1;
    if (k1 > k2)
        return 1;
    switch (p1.kind()) {
    case Kind::Empty:
        return 0;
    case Kind::Field: {
        const auto a = std::get<Field>(p1.m_data).fieldName;
        const auto b = std::get<Field>(p2.m_data).fieldName;
        if (a == b)
            return 0;
        return size_t(a) < size_t(b) ? -1 : 1;
    }
    case Kind::Index:
        if (std::get<Index>(p1.m_data).indexValue < std::get<Index>(p2.m_data).indexValue)
            return -1;
        if (std::get<Index>(p1.m_data).indexValue > std::get<Index>(p2.m_data).indexValue)
            return 1;
        return 0;
    case Kind::Key:
        return std::get<Key>(p1.m_data).keyValue.compare(std::get<Key>(p2.m_data).keyValue);
    case Kind::Root:
    {
        PathRoot k1 = std::get<Root>(p1.m_data).contextKind;
        PathRoot k2 = std::get<Root>(p2.m_data).contextKind;
        if (k1 == PathRoot::Env || k1 == PathRoot::Universe)
            k1 = PathRoot::Top;
        if (k2 == PathRoot::Env || k2 == PathRoot::Universe)
            k2 = PathRoot::Top;
        return int(k1) - int(k2);
    }
    case Kind::Current:
        return int(std::get<Current>(p1.m_data).contextKind)
                - int(std::get<Current>(p2.m_data).contextKind);
    }
    Q_ASSERT(false && "unexpected PathComponent in PathComponent::cmp");
    return 0;
}

} // namespace PathEls

namespace PathEls {

PathNode::~PathNode()
{
    // Release the chain of ancestors iteratively, to avoid recursive destruction
    // blowing up stack space
    const PathNode *node = parent.take();
    while (node && !node->ref.deref()) {
        const PathNode *next = const_cast<PathNode *>(node)->parent.take();
        delete node;
        node = next;
    }
}

} // namespace PathEls

/*!
\internal
\brief Returns the nodes of this path, front to back.
*/
Path::NodeArray Path::nodes() const
{
    NodeArray result(m_length);
    const Node *node = m_lastNode.data();
    for (qsizetype i = m_length; i > 0; node = node->parent.data())
        result[--i] = node;
    return result;
}

/*!
\internal
\brief Returns the \a i th component, in O(length - i).
*/
const PathEls::PathComponent &Path::component(int i) const
{
    static const Component emptyComponent;
    if (i < 0)
        i += m_length;
    if (i >= m_length || i < 0) {
        Q_ASSERT(false && "index out of bounds");
        return emptyComponent;
    }
    const Node *node = m_lastNode.data();
    for (int toSkip = m_length - 1 - i; toSkip > 0; --toSkip)
        node = node->parent.data();
    return node->component;
}

Path Path::operator[](int i) const
{
    return mid(i,1);
}

QQmlJS::Dom::Path::operator bool() const
{
    return length() != 0;
}

PathIterator Path::begin() const
{
    return PathIterator{*this};
}

PathSentinel Path::end() const
{
    return {};
}

std::optional<PathRoot> Path::headRoot() const
{
    auto &comp = component(0);
    if (PathEls::Root const *r = comp.asRoot())
        return r->contextKind;
    return std::nullopt;
}

std::optional<PathCurrent> Path::headCurrent() const
{
    auto &comp = component(0);
    if (PathEls::Current const *c = comp.asCurrent())
        return c->contextKind;
    return std::nullopt;
}

std::optional<FieldName> Path::headField() const
{
    auto &comp = component(0);
    if (const PathEls::Field *f = comp.asField())
        return f->fieldName;
    return std::nullopt;
}

Path::Kind Path::headKind() const
{
    if (m_length == 0)
        return Path::Kind::Empty;
    return component(0).kind();
}

QString Path::headName() const
{
    return component(0).name();
}

std::optional<index_type> Path::headIndex() const
{
    auto &comp = component(0);
    if (PathEls::Index const *i = comp.asIndex())
        return i->indexValue;
    return std::nullopt;
}

Path Path::head() const
{
    return mid(0,1);
}

Path Path::last() const
{
    return mid(m_length-1, 1);
}

/*!
\internal
\brief Splits the path at the last field or current component, or right after a
root anchor.

Field and Current introduce a new logical "starting point" inside the path,
so split puts everything before them into \c pathToSource and the component
itself (plus any tail) into \c pathFromSource. A Root, in contrast, is an
anchor: \c pathToSource includes the Root and \c pathFromSource is what
follows it. If neither is present, the entire path is \c pathFromSource.
*/
Source Path::split() const
{
    const NodeArray nodes = this->nodes();
    int i = length();
    while (i>0) {
        const PathEls::PathComponent &c = nodes[--i]->component;
        if (c.kind() == Kind::Field || c.kind() == Kind::Current) {
            return Source{mid(0, i), mid(i)};
        }
        if (c.kind() == Kind::Root) {
            return Source{mid(0, i + 1), mid(i + 1)};
        }
    }
    return Source{Path(), *this};
}


Path Path::fromComponent(Component c)
{
    return Path().withComponent(std::move(c));
}

Path Path::fromRoot(PathRoot s)         { return fromComponent(s); }
Path Path::fromIndex(index_type i)      { return fromComponent(PathEls::Index(i)); }
Path Path::fromField(FieldName name)    { return fromComponent(name); }
Path Path::fromKey(const QString &s)    { return fromComponent(PathEls::Key(s)); }
Path Path::fromKey(QStringView s)       { return fromKey(s.toString()); }
Path Path::fromCurrent(PathCurrent s)   { return fromComponent(s); }

Path Path::withComponent(PathEls::PathComponent c) const
{
    Q_ASSERT(m_length < std::numeric_limits<quint16>::max());
    return Path(Node::Ptr(new Node(std::move(c), m_lastNode)), m_length + 1);
}

Path Path::withEmpty() const                  { return withComponent(PathEls::PathComponent()); }
Path Path::withField(FieldName name) const    { return withComponent(name); }
Path Path::withKey(const QString &name) const { return withComponent(PathEls::Key(name)); }
Path Path::withKey(QStringView name) const    { return withKey(name.toString()); }
Path Path::withIndex(index_type i) const      { return withComponent(PathEls::Index(i)); }
Path Path::withCurrent(PathCurrent s) const   { return withComponent(s); }

/*!
\internal
\brief Returns a copy of this with \c toAdd appended to it.

This allocates one node per component of \a toAdd. If this path is empty,
\a toAdd is returned as is.
*/
Path Path::withPath(const Path &toAdd) const
{
    if (m_length == 0)
        return toAdd;
    Path result = *this;
    for (const Node *node : toAdd.nodes())
        result = result.withComponent(node->component);
    return result;
}

/*!
\internal
\brief Compares \a p1 and \a p2 lexicographically.
*/
int Path::cmp(const Path &p1, const Path &p2)
{
    if (p1.m_lastNode == p2.m_lastNode && p1.m_length == p2.m_length)
        return 0;
    const Node *n1 = p1.m_lastNode.data();
    const Node *n2 = p2.m_lastNode.data();
    // Lexicographic order only depends on the first min(length) components.
    for (int l = p1.m_length; l > p2.m_length; --l)
        n1 = n1->parent.data();
    for (int l = p2.m_length; l > p1.m_length; --l)
        n2 = n2->parent.data();
    // Walk towards the front; the last mismatch seen is the first one in path order.
    // Shared nodes are equal by pointer, which ends the walk early.
    int result = 0;
    for (int l = qMin(p1.m_length, p2.m_length); l > 0 && n1 != n2; --l) {
        if (const int c = Component::cmp(n1->component, n2->component))
            result = c;
        n1 = n1->parent.data();
        n2 = n2->parent.data();
    }
    if (result != 0)
        return result;
    return p1.m_length - p2.m_length;
}

void Path::dump(const Sink &sink) const
{
    const NodeArray nodes = this->nodes();
    bool first = true;
    for (const Node *node : nodes) {
        const Component &c = node->component;
        if (!c.hasSquareBrackets()) {
            if (!first || (c.kind() != Kind::Root && c.kind() != Kind::Current))
                sink(u".");
        }
        c.dump(sink);
        first = false;
    }
}

QString Path::toString() const
{
    QString res;
    QTextStream stream(&res);
    dump([&stream](QStringView str){ stream << str; });
    stream.flush();
    return res;
}

Path Path::dropFront(int n) const
{
    if (m_length > n && n >= 0)
        return Path(m_lastNode, m_length - n);
    return Path();
}

Path Path::dropTail(int n) const
{
    if (m_length > n && n >= 0) {
        const Node *node = m_lastNode.data();
        for (int i = 0; i < n; ++i)
            node = node->parent.data();
        return Path(Node::Ptr(node), m_length - n);
    }
    return Path();
}

Path Path::mid(int offset, int length) const
{
    length = qMin(m_length - offset, length);
    if (offset < 0 || offset >= m_length || length <= 0 || length > m_length)
        return Path();
    return dropTail(m_length - offset - length).dropFront(offset);
}

Path Path::mid(int offset) const
{
    return mid(offset, m_length - offset);
}

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE

