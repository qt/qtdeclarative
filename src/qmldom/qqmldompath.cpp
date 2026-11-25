// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldomitem_p.h"
#include "qqmldomerrormessage_p.h"
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QChar>

#include <cstdint>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
class ErrorMessage;

namespace PathEls {

/*!
\internal
\class QQmljs::Dom::QmlPath::Path

\brief Represents an immutable JsonPath like path in the Qml code model (from a DomItem to another
 DomItem)


\section1 Implementation Details

The Path class is implemented as a linked list tree where components of the path can be shared
between multiple paths.

We will represent a list of components with
\badcode
[a] // one element "a" in the list
[a,b,c] // three elements in the list
\endcode
and the linked list tree structure with arrows:
\badcode
[a,b,c] <- [y,z] // one linked list made up of two nodes consisting of a, b, c, y and z
   ^
   |______ [q] <- [w] // one linked list made up of three nodes consisting of a, b, c, q and w,
                      // sharing the a,b and c part with the above list
\endcode

Appending components to the path will create a new linked list node, for example when appending \c d
to \c{[a,b,c]}:
\badcode
[a,b,c] <- [d]
\endcode
This allows to share the path prefix when constructing paths recursively, for example.

\section2 PathData as elements of the linked list trees

Each node of the linked list tree is stored as \c PathData. It contains a vector of
\c PathComponents and a pointer to the previous element in the path (called parent).

\section3 PathComponents

The path can also be "compressed", as the linked list structure contains vectors of components:
\c {a.b.c.d} could be represented by
\badcode
[a] <- [b] <- [c] <- [d]
\endcode
(which is the default) or also
\badcode
[a, b] <- [c, d]
\endcode
when \c{[c,d]} got appended to \c{[a, b]} via \c withPath, for example.

\section3 Component string ownership

No \c PathComponent refers to external string storage: \c Field stores a
\c FieldName enum value, and \c Key owns its \c QString. Paths are therefore
self-contained.

\section2 Appending

Appending one component to the Path allows to share the "parent" path in the memory and should be
the preferred way of constructing paths for DomItems.

\section2 Iteration

To iterate through the path, you need two iterators. One for the current data (that contains the
vector of components and the pointer to the "parent") and one for the current idx in the vector of
components.

To access \c d in that path:
\badcode
[a, b] <- [c, d]
\endcode
you need to do
\badcode
path.m_data->components[1].
\endcode
to access \c b instead, you need to do:
\badcode
path.m_data->parent->components[1]
\endcode

Note that all kind of forward iterations (getting the components a, b, c, and d in this order from
a.b.c.d) is a bit more involved, and backward iteration is much faster/easier.

\section2 Slicing

The path also supports slicing via the \c m_length and \c m_endOffset members. \c m_endOffset
ignores the n last components, for example \c{a.b.c.d} can be stored as

\badcode
[root] <- [a] <- [b] <- [c] <- [d] <- [e] <- [f] <- [g]
\endcode
with m_endOffset = 3 and m_length = 4.

\section2 Lifetime in the DOM

Paths are usually created during the Dom construction, and destroyed when the DomItem or Dom
element holding it is destroyed.

The underlying PathData, on the other hand, is managed by a std::shared_ptr. It is destroyed when
all Paths referencing it are destroyed. PathData can be referenced either directly through Path's \c
m_data member, or indirectly through another PathData's \c m_parent member.

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
\li \l{$} ? undecided, one the previous ones?
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
\li \l{@} ? undecided, one the previous ones
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

/*!
\internal
\brief Returns the n.th component in linear(!) time.

Especially component(0)-calls have to iterate through the entire path.
Example: returns \c d for 3 on
\badcode
a.b <- c.d <- e <- f
\endcode
*/
const PathEls::PathComponent &Path::component(int i) const
{
    static Component emptyComponent;
    if (i < 0)
        i += m_length;
    if (i >= m_length || i < 0) {
        Q_ASSERT(false && "index out of bounds");
        return emptyComponent;
    }
    i = i - m_length - m_endOffset;
    auto data = m_data.get();
    while (data) {
        i += data->components.size();
        if (i >= 0)
            return std::as_const(data)->components[i];
        data = data->parent.get();
    }
    Q_ASSERT(false && "Invalid data reached while resolving a seemengly valid index in Path (inconsisten Path object)");
    return emptyComponent;
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

PathIterator Path::end() const
{
    return PathIterator();
}

std::optional<PathRoot> Path::headRoot() const
{
    auto &comp = component(0);
    if (PathEls::Root const * r = comp.asRoot())
        return r->contextKind;
    return std::nullopt;
}

std::optional<PathCurrent> Path::headCurrent() const
{
    auto comp = component(0);
    if (PathEls::Current const * c = comp.asCurrent())
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

bool Path::checkHeadName(QStringView name) const
{
    return component(0).checkName(name);
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
    int i = length();
    while (i>0) {
        const PathEls::PathComponent &c=component(--i);
        if (c.kind() == Kind::Field || c.kind() == Kind::Current) {
            return Source{mid(0, i), mid(i)};
        }
        if (c.kind() == Kind::Root) {
            return Source{mid(0, i + 1), mid(i + 1)};
        }
    }
    return Source{Path(), *this};
}


Path Path::fromRoot(PathRoot s)
{
    return Path(0,1,std::make_shared<PathEls::PathData>(
                    QList<Component>(1,Component(s))));
}

Path Path::fromIndex(index_type i)
{
    return Path(0,1,std::make_shared<PathEls::PathData>(
                    QList<Component>(1,Component(PathEls::Index(i)))));
}

Path Path::fromField(FieldName name)
{
    return Path(0, 1,
                std::make_shared<PathEls::PathData>(
                        QList<Component>(1, Component(name))));
}

Path Path::fromKey(QStringView s)
{
    return Path(
            0, 1,
            std::make_shared<PathEls::PathData>(
                    QList<Component>(1, Component(PathEls::Key(s.toString())))));
}

Path Path::fromKey(const QString &s)
{
    return Path(0, 1,
                std::make_shared<PathEls::PathData>(
                        QList<Component>(1, Component(PathEls::Key(s)))));
}

Path Path::fromCurrent(PathCurrent s)
{
    return Path(0,1,std::make_shared<PathEls::PathData>(
                    QList<Component>(1,Component(s))));
}

Path Path::withEmpty() const
{
    if (m_endOffset != 0)
        return noEndOffset().withEmpty();
    return Path(0,m_length+1,std::make_shared<PathEls::PathData>(
                    QList<Component>(1,Component()), m_data));
}

Path Path::withField(FieldName name) const
{
    if (m_endOffset != 0)
        return noEndOffset().withField(name);
    return Path(0, m_length + 1,
                std::make_shared<PathEls::PathData>(
                        QList<Component>(1, Component(name)), m_data));
}

Path Path::withKey(const QString &name) const
{
    if (m_endOffset != 0)
        return noEndOffset().withKey(name);
    return Path(0,m_length+1,std::make_shared<PathEls::PathData>(
                    QList<Component>(1,Component(PathEls::Key(name))), m_data));
}

Path Path::withKey(QStringView name) const
{
    return withKey(name.toString());
}

Path Path::withIndex(index_type i) const
{
    if (m_endOffset != 0)
        return noEndOffset().withIndex(i);
    return Path(0,m_length+1,std::make_shared<PathEls::PathData>(
                    QList<Component>(1,Component(i)), m_data));
}

Path Path::withCurrent(PathCurrent s) const
{
    if (m_endOffset != 0)
        return noEndOffset().withCurrent(s);
    return Path(0,m_length+1,std::make_shared<PathEls::PathData>(
                    QList<Component>(1,Component(s)), m_data));
}

static QList<Path::Component>
reverseAndConcat(const std::vector<QList<Path::Component> *> &list, qsizetype size)
{
    using Component = Path::Component;

    QList<Component> components;
    components.reserve(size);
    for (auto it = list.crbegin(), end = list.crend(); it != end; ++it) {
        for (auto it2 = (**it).cbegin(), end2 = (**it).cend(); it2 != end2; ++it2) {
            components.append(*it2);
            if (components.size() >= size)
                return components;
        }
    }
    return components;
}

static std::vector<QList<Path::Component> *>
collectBackwards(PathEls::PathData *pathStart, qsizetype size)
{
    using Component = Path::Component;
    std::vector<QList<Component> *> componentList;
    qsizetype foundComponents = 0;
    for (auto data = pathStart; data; data = data->parent.get()) {
        componentList.push_back(&data->components);
        foundComponents += data->components.size();

        if (foundComponents >= size)
            return componentList;
    }
    return componentList;
}

/*!
\internal
\brief Returns a copy of this with \c toAdd appended to it

Compresses \c toAdd into one vector of components before adding it to \c this.
*/
Path Path::withPath(const Path &toAdd, bool avoidToAddAsBase) const
{
    if (toAdd.length() == 0)
        return *this;
    const int resLength = length() + toAdd.length();
    if (m_endOffset != 0) {
        Path thisExtended = this->expandBack();
        if (thisExtended.length() > resLength)
            thisExtended = thisExtended.mid(0, resLength);
        Path added = thisExtended.mid(length(), thisExtended.length() - length());
        if (added == toAdd.mid(0, toAdd.length())) {
            if (resLength == thisExtended.length())
                return thisExtended;
            else
                return thisExtended.withPath(toAdd.mid(added.length(), resLength - thisExtended.length()));
        }
    }
    if (!avoidToAddAsBase) {
        Path toAddExtended = toAdd.expandFront();
        if (toAddExtended.length() >= resLength) {
            toAddExtended = toAddExtended.mid(toAddExtended.length() - resLength, resLength);
            if (toAddExtended.mid(0,length()) == *this)
                return toAddExtended;
        }
    }

    const Path toAddNoEndOffset = toAdd.noEndOffset();
    auto componentListList = collectBackwards(toAddNoEndOffset.m_data.get(), toAdd.length());

    QList<Component> componentList = reverseAndConcat(componentListList, toAdd.length());

    Path result(
            0, resLength,
            std::make_shared<PathEls::PathData>(componentList, noEndOffset().m_data));
    return result;
}

/*!
\internal
\brief Expand a path prefix hidden by slicing

This sets m_length to the maximum length this path can have.

For example, for a path
\badcode
b -> c -> d when encoded as a -> b -> c -> d -> e -> f, m_endOffset = 2, m_length = 3
\endcode
expandFront() will return
\badcode
a -> b -> c -> d when encoded as a -> b -> c -> d -> e -> f, m_endOffset = 2, m_length = 4
\endcode
*/
Path Path::expandFront() const
{
    int newLen = 0;
    auto data = m_data.get();
    while (data) {
        newLen += data->components.size();
        data = data->parent.get();
    }
    newLen -= m_endOffset;
    return Path(m_endOffset, newLen, m_data);
}

/*!
\internal
\brief Expand a path suffix hidden by slicing.

This sets m_endOffset to 0.

For example, for a path
\badcode
c -> d when encoded as a -> b -> c -> d -> e -> f -> g, m_endOffset = 3, m_length = 2
\endcode
expandBack() will return
\badcode
c -> d -> e -> f -> g when encoded as a -> b -> c -> d -> e -> f -> g, m_endOffset = 0, m_length = 5
\endcode
*/
Path Path::expandBack() const
{
    if (m_endOffset > 0)
        return Path(0, m_length + m_endOffset, m_data);
    return *this;
}

Path &Path::operator++()
{
    if (m_length > 0)
        m_length -= 1;
    return *this;
}

Path Path::operator++(int)
{
    Path res = *this;
    if (m_length > 0)
        m_length -= 1;
    return res;
}

int Path::cmp(const Path &p1, const Path &p2)
{
    // lexicographic ordering
    const int lMin = qMin(p1.m_length, p2.m_length);
    if (p1.m_data.get() == p2.m_data.get() && p1.m_endOffset == p2.m_endOffset && p1.m_length == p2.m_length)
        return 0;
    for (int i = 0; i < lMin; ++i) {
        int c = Component::cmp(p1.component(i), p2.component(i));
        if (c != 0)
            return c;
    }
    if (lMin < p2.m_length)
        return -1;
    if (p1.m_length > lMin)
        return 1;
    return 0;
}

Path::Path(quint16 endOffset, quint16 length, const std::shared_ptr<PathEls::PathData> &data)
    :m_endOffset(endOffset), m_length(length), m_data(data)
{
}

Path Path::noEndOffset() const
{
    if (m_length == 0)
        return Path();
    if (m_endOffset == 0)
        return *this;
    // peel back
    qint16 endOffset = m_endOffset;
    std::shared_ptr<PathEls::PathData> lastData = m_data;
    while (lastData && endOffset >= lastData->components.size()) {
        endOffset -= lastData->components.size();
        lastData = lastData->parent;
    }
    if (endOffset > 0) {
        Q_ASSERT(lastData && "Internal problem, reference to non existing PathData");
        return Path(0, m_length, std::make_shared<PathEls::PathData>(
                        lastData->components.mid(0, lastData->components.size() - endOffset), lastData->parent));
    }
    return Path(0, m_length, lastData);
}

Path Path::withComponent(const PathEls::PathComponent &c)
{
    if (m_endOffset != 0) {
        Path newP = noEndOffset();
        return newP.withComponent(c);
    }
    if (m_data && m_data.use_count() != 1) {
        // create a new path (otherwise paths linking to this will change)
        Path newP(c);
        newP.m_data->parent = m_data;
        newP.m_length = static_cast<quint16>(m_length + 1);
        return newP;
    }
    auto my_data =
            (m_data ? m_data
                    : std::make_shared<PathEls::PathData>(QList<PathEls::PathComponent>()));
    switch (c.kind()) {
    case PathEls::Kind::Empty:
    case PathEls::Kind::Index:
    case PathEls::Kind::Field:
        // string assumed to stay valid (Fields::...)
    case PathEls::Kind::Current:
    case PathEls::Kind::Key:
    case PathEls::Kind::Root:
        my_data->components.append(c);
        break;
    }
    if (m_data)
        m_endOffset = 1;
    return Path { 0, static_cast<quint16>(m_length + 1), my_data };
}

void Path::dump(const Sink &sink) const
{
    bool first = true;
    for (int i = 0; i < m_length; ++i) {
        auto &c = component(i);
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
        return Path(m_endOffset, m_length - n, m_data);
    return Path();
}

Path Path::dropTail(int n) const
{
    if (m_length > n && n >= 0)
        return Path(m_endOffset + n, m_length - n, m_data);
    return Path();
}

Path Path::mid(int offset, int length) const
{
    length = qMin(m_length - offset, length);
    if (offset < 0 || offset >= m_length || length <= 0 || length > m_length)
        return Path();
    int newEndOffset = m_endOffset + m_length - offset - length;
    return Path(newEndOffset, length, m_data);
}

Path Path::mid(int offset) const
{
    return mid(offset, m_length - offset);
}

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE

#include "moc_qqmldompath_p.cpp"
