// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldomcompare_p.h"
#include "QtCore/qglobal.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

bool domCompare(DomItem &i1, DomItem &i2, function_ref<bool(Path, DomItem &, DomItem &)> change,
                function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter,
                Path basePath)
{
    DomKind k1 = i1.domKind();
    DomKind k2 = i2.domKind();
    if (k1 != k2) {
        if (!change(basePath, i1, i2))
            return false;
    } else {
        switch (k1) {
        case DomKind::Empty:
            return true;
        case DomKind::Object: {
            QStringList f1 = i1.fields();
            QStringList f2 = i2.fields();
            f1.sort();
            f2.sort();
            qsizetype it1 = 0;
            qsizetype it2 = 0;
            while (it1 != f1.size() || it2 != f2.size()) {
                QString k1, k2;
                DomItem el1, el2;
                bool hasK1 = it1 != f1.size();
                bool filt1 = hasK1;
                if (hasK1) {
                    k1 = f1.at(it1);
                    el1 = i1.field(k1);
                    filt1 = i1.isCanonicalChild(el1) && filter(i1, PathEls::Field(k1), el1);
                }
                bool hasK2 = it2 != f2.size();
                bool filt2 = hasK2;
                if (hasK2) {
                    k2 = f2.at(it2);
                    el2 = i2.field(k2);
                    filt2 = i2.isCanonicalChild(el2) && filter(i2, PathEls::Field(k2), el2);
                }
                // continue when filtering out
                if (hasK1 && !filt1) {
                    ++it1;
                    if (hasK2 && !filt2)
                        ++it2;
                    continue;
                } else if (hasK2 && !filt2) {
                    ++it2;
                    continue;
                }
                if (filt1 && filt2 && k1 == k2) {
                    if (!domCompare(el1, el2, change, filter, basePath.field(k1)))
                        return false;
                    ++it1;
                    ++it2;
                } else if (!hasK1 || (hasK2 && k1 > k2)) {
                    if (!change(basePath.field(k1), DomItem::empty, el2))
                        return false;
                    ++it2;
                } else {
                    if (!change(basePath.field(k1), el1, DomItem::empty))
                        return false;
                    ++it1;
                }
            }
        } break;
        case DomKind::Map: {
            QStringList f1 = i1.sortedKeys();
            QStringList f2 = i2.sortedKeys();
            qsizetype it1 = 0;
            qsizetype it2 = 0;
            while (it1 != f1.size() || it2 != f2.size()) {
                QString k1, k2;
                DomItem el1, el2;
                bool hasK1 = it1 != f1.size();
                bool filt1 = hasK1;
                if (hasK1) {
                    k1 = f1.at(it1);
                    el1 = i1.key(k1);
                    filt1 = i1.isCanonicalChild(el1) && filter(i1, PathEls::Key(k1), el1);
                }
                bool hasK2 = it2 != f2.size();
                bool filt2 = hasK2;
                if (hasK2) {
                    k2 = f2.at(it2);
                    el2 = i2.key(k2);
                    filt2 = i2.isCanonicalChild(el2) && filter(i2, PathEls::Key(k2), el2);
                }
                // continue when filtering out
                if (hasK1 && !filt1) {
                    ++it1;
                    if (hasK2 && !filt2)
                        ++it2;
                    continue;
                } else if (hasK2 && !filt2) {
                    ++it2;
                    continue;
                }
                if (filt1 && filt2 && k1 == k2) {
                    if (!domCompare(el1, el2, change, filter, basePath.key(k1)))
                        return false;
                    ++it1;
                    ++it2;
                } else if (!hasK1 || (hasK2 && k1 > k2)) {
                    if (!change(basePath.key(k1), DomItem::empty, el2))
                        return false;
                    ++it2;
                } else {
                    if (!change(basePath.key(k1), el1, DomItem::empty))
                        return false;
                    ++it1;
                }
            }
        } break;
        case DomKind::List: {
            // we could support smarter matching keeping filtering in account like map
            // currently it is just a simple index by index comparison
            index_type len1 = i1.indexes();
            index_type len2 = i2.indexes();
            if (len1 != len2)
                return change(basePath, i1, i2);
            for (index_type i = 0; i < len1; ++i) {
                DomItem v1 = i1.index(i);
                DomItem v2 = i2.index(i);
                if (filter(i1, PathEls::Index(i), v1) && filter(i2, PathEls::Index(i), v2)) {
                    DomItem el1 = i1.index(i);
                    DomItem el2 = i2.index(i);
                    if (i1.isCanonicalChild(el1) && i2.isCanonicalChild(el2)
                        && !domCompare(el1, el2, change, filter, basePath.index(i)))
                        return false;
                }
            }
        } break;
        case DomKind::Value: {
            QCborValue v1 = i1.value();
            QCborValue v2 = i2.value();
            if (v1 != v2)
                return change(basePath, i1, i2);
        } break;
        case DomKind::ScriptElement: {
            // TODO: implement me
            return false;

        } break;
        }
    }
    return true;
}

QStringList
domCompareStrList(DomItem &i1, DomItem &i2,
                  function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &) const> filter,
                  DomCompareStrList stopAtFirstDiff)
{
    QStringList res;
    bool hasDiff = false;
    domCompare(
            i1, i2,
            [&res, &hasDiff, stopAtFirstDiff](Path p, DomItem &j1, DomItem &j2) {
                hasDiff = true;
                if (!j1) {
                    res.append(QStringLiteral("- %1\n").arg(p.toString()));
                } else if (!j2) {
                    res.append(QStringLiteral("+ %1\n").arg(p.toString()));
                } else {
                    DomKind k1 = j1.domKind();
                    DomKind k2 = j2.domKind();
                    if (k1 != k2) {
                        res.append(
                                QStringLiteral("- %1 %2\n").arg(p.toString(), domKindToString(k1)));
                        res.append(
                                QStringLiteral("+ %1 %2\n").arg(p.toString(), domKindToString(k2)));
                    } else {
                        switch (k1) {
                        case DomKind::Empty:
                        case DomKind::Object:
                        case DomKind::Map:
                            Q_ASSERT(false);
                            break;
                        case DomKind::List: {
                            index_type len1 = j1.indexes();
                            index_type len2 = j2.indexes();
                            res.append(QStringLiteral("- %1 #%2\n").arg(p.toString()).arg(len1));
                            res.append(QStringLiteral("+ %1 #%2\n").arg(p.toString()).arg(len2));
                        } break;
                        case DomKind::Value: {
                            QCborValue v1 = j1.value();
                            QCborValue v2 = j2.value();
                            auto t1 = v1.type();
                            auto t2 = v2.type();
                            if (t1 != t2) {
                                res.append(QStringLiteral("- %1 type(%2)\n")
                                                   .arg(p.toString())
                                                   .arg(int(t1)));
                                res.append(QStringLiteral("+ %1 type(%2)\n")
                                                   .arg(p.toString())
                                                   .arg(int(t2)));
                            } else {
                                res.append(QStringLiteral("- %1 value(%2)\n")
                                                   .arg(p.toString())
                                                   .arg(j1.toString()));
                                res.append(QStringLiteral("+ %1 value(%2)\n")
                                                   .arg(p.toString())
                                                   .arg(j2.toString()));
                            }
                        } break;
                        case DomKind::ScriptElement: {
                            // implement me
                            break;
                        }
                        }
                    }
                }
                return (stopAtFirstDiff == DomCompareStrList::AllDiffs);
            },
            filter);
    if (hasDiff && res.isEmpty()) // should never happen
        res.append(QStringLiteral(u"Had changes!"));
    return res;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
