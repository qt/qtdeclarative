// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldompathparser_p.h"
#include <QtQmlDom/private/qqmldomerrormessage_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>

#include <QtCore/QChar>
#include <QtCore/QMetaEnum>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

// Error group and translation context for parse errors.
class PathParseMessages
{
    Q_DECLARE_TR_FUNCTIONS(PathParsing)
public:
    static ErrorGroups group()
    {
        static ErrorGroups res = { { NewErrorGroup("PathParsing") } };
        return res;
    }
};

namespace {

enum class ParserState {
    Start,
    IndexOrKey,
    End
};

template<typename Enum>
bool lookupEnumValue(QStringView name, Enum &out)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Enum>();
    for (int j = 0; j < metaEnum.keyCount(); ++j) {
        if (name.compare(QString::fromUtf8(metaEnum.key(j)), Qt::CaseInsensitive) == 0) {
            out = Enum(metaEnum.value(j));
            return true;
        }
    }
    return false;
}

} // namespace

Path parsePath(QStringView s, const ErrorHandler &errorHandler)
{
    using Component = Path::Component;

    if (s.isEmpty())
        return Path();
    int len = 1;
    const QChar dot = QChar::fromLatin1('.');
    const QChar lsBrace = QChar::fromLatin1('[');
    const QChar rsBrace = QChar::fromLatin1(']');
    const QChar dollar = QChar::fromLatin1('$');
    const QChar at = QChar::fromLatin1('@');
    const QChar quote = QChar::fromLatin1('"');
    const QChar backslash = QChar::fromLatin1('\\');
    const QChar underscore = QChar::fromLatin1('_');
    const QChar tilda = QChar::fromLatin1('~');
    for (int i = 0; i < s.size(); ++i)
        if (s.at(i) == lsBrace || s.at(i) == dot)
            ++len;
    QList<Component> components;
    components.reserve(len);
    int i = 0;
    int i0 = 0;
    ParserState state = ParserState::Start;
    while (i < s.size()) {
        // skip space
        while (i < s.size() && s.at(i).isSpace())
            ++i;
        if (i >= s.size())
            break;
        QChar c = s.at(i++);
        switch (state) {
        case ParserState::Start:
            if (c == dollar) {
                i0 = i;
                while (i < s.size() && s.at(i).isLetterOrNumber()) {
                    ++i;
                }
                QStringView name = s.mid(i0, i - i0);
                PathRoot rootKind;
                if (!lookupEnumValue(name, rootKind)) {
                    PathParseMessages::group().error(PathParseMessages::tr("Unknown root context '$%1'.").arg(name))
                            .handle(errorHandler);
                    return Path();
                }
                components.append(Component(rootKind));
                state = ParserState::End;
            } else if (c == at) {
                i0 = i;
                while (i < s.size() && s.at(i).isLetterOrNumber()) {
                    ++i;
                }
                QStringView name = s.mid(i0, i - i0);
                PathCurrent currentKind;
                if (!lookupEnumValue(name, currentKind)) {
                    PathParseMessages::group().error(PathParseMessages::tr("Unknown current context '@%1'.").arg(name))
                            .handle(errorHandler);
                    return Path();
                }
                components.append(Component(currentKind));
                state = ParserState::End;
            } else if (c.isLetter()) {
                PathParseMessages::group().warning(
                        PathParseMessages::tr("Field expressions should start with a dot, even when at the "
                                 "start of the path %1.")
                                .arg(s)).handle(errorHandler);
                return Path();
            } else {
                --i;
                state = ParserState::End;
            }
            break;
        case ParserState::IndexOrKey:
            if (c.isDigit()) {
                i0 = i - 1;
                while (i < s.size() && s.at(i).isDigit())
                    ++i;
                bool ok;
                components.append(Component(static_cast<index_type>(
                        s.mid(i0, i - i0).toString().toLongLong(&ok))));
                if (!ok) {
                    PathParseMessages::group().warning(PathParseMessages::tr("Error extracting integer from '%1' at char %2.")
                                                     .arg(s.mid(i0, i - i0))
                                                     .arg(QString::number(i0)))
                            .handle(errorHandler);
                }
            } else if (c.isLetter() || c == tilda || c == underscore) {
                i0 = i - 1;
                while (i < s.size() && (s.at(i).isLetterOrNumber() || s.at(i) == underscore || s.at(i) == tilda))
                    ++i;
                components.append(Component(PathEls::Key(s.mid(i0, i - i0).toString())));
            } else if (c == quote) {
                i0 = i;
                QString strVal;
                bool properEnd = false;
                while (i < s.size()) {
                    c = s.at(i);
                    if (c == quote) {
                        properEnd = true;
                        break;
                    } else if (c == backslash) {
                        strVal.append(s.mid(i0, i - i0).toString());
                        c = s.at(++i);
                        i0 = i + 1;
                        if (c == QChar::fromLatin1('n'))
                            strVal.append(QChar::fromLatin1('\n'));
                        else if (c == QChar::fromLatin1('r'))
                            strVal.append(QChar::fromLatin1('\r'));
                        else if (c == QChar::fromLatin1('t'))
                            strVal.append(QChar::fromLatin1('\t'));
                        else
                            strVal.append(c);
                    }
                    ++i;
                }
                if (properEnd) {
                    strVal.append(s.mid(i0, i - i0).toString());
                    ++i;
                } else {
                    PathParseMessages::group().error(PathParseMessages::tr("Unclosed quoted string at char %1.")
                                                   .arg(QString::number(i - 1))).handle(errorHandler);
                    return Path();
                }
                components.append(PathEls::Key(strVal));
            } else {
                PathParseMessages::group().error(PathParseMessages::tr("Unexpected character '%1' after square bracket at %2.")
                                               .arg(c).arg(i - 1)).handle(errorHandler);
                return Path();
            }
            while (i < s.size() && s.at(i).isSpace())
                ++i;
            if (i >= s.size() || s.at(i) != rsBrace) {
                PathParseMessages::group().error(PathParseMessages::tr("square braces misses closing brace at char %1.")
                                               .arg(QString::number(i))).handle(errorHandler);
                return Path();
            } else {
                ++i;
            }
            state = ParserState::End;
            break;
        case ParserState::End:
            if (c == dot) {
                while (i < s.size() && s.at(i).isSpace())
                    ++i;
                if (i == s.size()) {
                    components.append(Component());
                    state = ParserState::End;
                } else if (s.at(i).isLetter() || s.at(i) == underscore || s.at(i) == tilda) {
                    i0 = i;
                    while (i < s.size() && (s.at(i).isLetterOrNumber() || s.at(i) == underscore || s.at(i) == tilda)) {
                        ++i;
                    }
                    const QStringView ident = s.mid(i0, i - i0);
                    if (auto fn = fieldNameFromString(ident))
                        components.append(Component(*fn));
                    else
                        components.append(Component(PathEls::Key(ident.toString())));
                    state = ParserState::End;
                } else if (s.at(i).isDigit()) {
                    i0 = i;
                    while (i < s.size() && s.at(i).isDigit()) {
                        ++i;
                    }
                    bool ok;
                    components.append(Component(static_cast<index_type>(
                            s.mid(i0, i - i0).toString().toLongLong(&ok))));
                    if (!ok) {
                        PathParseMessages::group().warning(PathParseMessages::tr("Error extracting integer from '%1' at char %2.")
                                                         .arg(s.mid(i0, i - i0))
                                                         .arg(QString::number(i0)))
                                .handle(errorHandler);
                        return Path();
                    } else {
                        PathParseMessages::group().info(PathParseMessages::tr("Index should use square brackets and not a dot (at char %1).")
                                                      .arg(QString::number(i0)))
                                .handle(errorHandler);
                    }
                    state = ParserState::End;
                } else if (s.at(i) == dot || s.at(i) == lsBrace) {
                    components.append(Component());
                    state = ParserState::End;
                } else if (s.at(i) == at) {
                    i0 = ++i;
                    while (i < s.size() && s.at(i).isLetterOrNumber()) {
                        ++i;
                    }
                    QStringView name = s.mid(i0, i - i0);
                    PathCurrent currentKind;
                    if (!lookupEnumValue(name, currentKind)) {
                        PathParseMessages::group().error(PathParseMessages::tr("Unknown current context '@%1'.").arg(name))
                                .handle(errorHandler);
                        return Path();
                    }
                    components.append(Component(currentKind));
                    state = ParserState::End;
                } else if (s.at(i) == dollar) {
                    i0 = ++i;
                    while (i < s.size() && s.at(i).isLetterOrNumber()) {
                        ++i;
                    }
                    QStringView name = s.mid(i0, i - i0);
                    PathRoot rootKind;
                    if (!lookupEnumValue(name, rootKind)) {
                        PathParseMessages::group().error(PathParseMessages::tr("Unknown root context '$%1'.").arg(name))
                                .handle(errorHandler);
                        return Path();
                    }
                    components.append(Component(rootKind));
                    state = ParserState::End;
                } else {
                    c = s.at(i);
                    PathParseMessages::group().error(PathParseMessages::tr("Unexpected character '%1' after dot (at char %2).")
                                                   .arg(QStringView(&c, 1))
                                                   .arg(QString::number(i - 1)))
                            .handle(errorHandler);
                    return Path();
                }
            } else if (c == lsBrace) {
                state = ParserState::IndexOrKey;
            } else {
                PathParseMessages::group().error(PathParseMessages::tr("Unexpected character '%1' after end of component (char %2).")
                                               .arg(QStringView(&c, 1))
                                               .arg(QString::number(i - 1)))
                        .handle(errorHandler);
                return Path();
            }
            break;
        }
    }
    switch (state) {
    case ParserState::Start:
        return Path();
    case ParserState::IndexOrKey:
        errorHandler(PathParseMessages::group().error(PathParseMessages::tr("unclosed square brace at end.")));
        return Path();
    case ParserState::End: {
        Path result;
        for (auto &c : components)
            result = std::move(result).withComponent(std::move(c));
        return result;
    }
    }
    Q_ASSERT(false && "Unexpected state in parsePath");
    return Path();
}

} // namespace Dom
} // namespace QQmlJS

QT_END_NAMESPACE
