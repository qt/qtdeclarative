// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

// This code is adapted from QtCreator/src/libs/utils/differ.

/*
The main algorithm "diffMyers()" is based on "An O(ND) Difference Algorithm
and Its Variations" by Eugene W. Myers: http://www.xmailserver.org/diff2.pdf

Preprocessing and postprocessing functions inspired by "Diff Strategies"
publication by Neil Fraser: http://neil.fraser.name/writing/diff/
*/

#include "qqmldiffer_p.h"

#include <QStringList>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
namespace QQmlLSUtils {

static qsizetype commonPrefix(const QString &text1, const QString &text2)
{
    // mismatch returns first non-equal pair
    const auto [it1, it2] = std::mismatch(text1.cbegin(), text1.cend(),
     text2.cbegin(), text2.cend());

    return qsizetype(std::distance(text1.cbegin(), it1));  // number of matching characters
}

static qsizetype commonSuffix(const QString &text1, const QString &text2)
{
    // mismatch returns first non-equal pair
    const auto [it1, it2] = std::mismatch(text1.crbegin(), text1.crend(),
     text2.crbegin(), text2.crend());

    return qsizetype(std::distance(text1.crbegin(), it1));  // number of matching characters
}

static QList<Diff> decode(const QList<Diff> &diffList, const QStringList &lines)
{
    QList<Diff> newDiffList;
    newDiffList.reserve(diffList.size());
    for (const Diff &diff : diffList) {
        QString text;
        for (QChar c : diff.text) {
            const qsizetype idx = static_cast<ushort>(c.unicode());
            text += lines.value(idx);
        }
        newDiffList.append({diff.command, text});
    }
    return newDiffList;
}

static QList<Diff> squashEqualities(const QList<Diff> &diffList)
{
    if (diffList.size() < 3) // we need at least 3 items
        return diffList;

    QList<Diff> newDiffList;
    Diff prevDiff = diffList.at(0);
    Diff thisDiff = diffList.at(1);
    Diff nextDiff = diffList.at(2);
    qsizetype i = 2;
    while (i < diffList.size()) {
        if (prevDiff.command == Diff::Equal
                && nextDiff.command == Diff::Equal) {
            if (thisDiff.text.endsWith(prevDiff.text)) {
                thisDiff.text = prevDiff.text
                        + thisDiff.text.left(thisDiff.text.size()
                        - prevDiff.text.size());
                nextDiff.text = prevDiff.text + nextDiff.text;
            } else if (thisDiff.text.startsWith(nextDiff.text)) {
                prevDiff.text += nextDiff.text;
                thisDiff.text = thisDiff.text.mid(nextDiff.text.size())
                        + nextDiff.text;
                i++;
                if (i < diffList.size())
                    nextDiff = diffList.at(i);
                newDiffList.append(prevDiff);
            } else {
                newDiffList.append(prevDiff);
            }
        } else {
            newDiffList.append(prevDiff);
        }
        prevDiff = thisDiff;
        thisDiff = nextDiff;
        i++;
        if (i < diffList.size())
            nextDiff = diffList.at(i);
    }
    newDiffList.append(prevDiff);
    if (i == diffList.size())
        newDiffList.append(thisDiff);
    return newDiffList;
}

///////////////

Diff::Diff(Command com, const QString &txt) :
    command(com),
    text(txt)
{
}

bool Diff::operator==(const Diff &other) const
{
     return command == other.command && text == other.text;
}

bool Diff::operator!=(const Diff &other) const
{
     return !(operator == (other));
}

///////////////

Differ::Differ()
{
}

QList<Diff> Differ::diff(const QString &text1, const QString &text2)
{
    m_currentDiffMode = m_diffMode;
    return merge(preprocess1AndDiff(text1, text2));
}

void Differ::setDiffMode(Differ::DiffMode mode)
{
    m_diffMode = mode;
}

Differ::DiffMode Differ::diffMode() const
{
    return m_diffMode;
}

QList<Diff> Differ::preprocess1AndDiff(const QString &text1, const QString &text2)
{
    if (text1.isNull() && text2.isNull())
        return {};

    if (text1 == text2) {
        QList<Diff> diffList;
        if (!text1.isEmpty())
            diffList.append(Diff(Diff::Equal, text1));
        return diffList;
    }

    QString newText1 = text1;
    QString newText2 = text2;
    QString prefix;
    QString suffix;
    const qsizetype prefixCount = commonPrefix(text1, text2);
    if (prefixCount) {
        prefix = text1.left(prefixCount);
        newText1 = text1.mid(prefixCount);
        newText2 = text2.mid(prefixCount);
    }
    const qsizetype suffixCount = commonSuffix(newText1, newText2);
    if (suffixCount) {
        suffix = newText1.right(suffixCount);
        newText1 = newText1.left(newText1.size() - suffixCount);
        newText2 = newText2.left(newText2.size() - suffixCount);
    }
    QList<Diff> diffList = preprocess2AndDiff(newText1, newText2);
    if (prefixCount)
        diffList.prepend(Diff(Diff::Equal, prefix));
    if (suffixCount)
        diffList.append(Diff(Diff::Equal, suffix));
    return diffList;
}

QList<Diff> Differ::preprocess2AndDiff(const QString &text1, const QString &text2)
{
    QList<Diff> diffList;

    if (text1.isEmpty()) {
        diffList.append(Diff(Diff::Insert, text2));
        return diffList;
    }

    if (text2.isEmpty()) {
        diffList.append(Diff(Diff::Delete, text1));
        return diffList;
    }

    if (text1.size() != text2.size()) {
        const QString longtext = text1.size() > text2.size() ? text1 : text2;
        const QString shorttext = text1.size() > text2.size() ? text2 : text1;
        const qsizetype i = longtext.indexOf(shorttext);
        if (i != -1) {
            const Diff::Command command = (text1.size() > text2.size())
                    ? Diff::Delete : Diff::Insert;
            diffList.append(Diff(command, longtext.left(i)));
            diffList.append(Diff(Diff::Equal, shorttext));
            diffList.append(Diff(command, longtext.mid(i + shorttext.size())));
            return diffList;
        }

        if (shorttext.size() == 1) {
            diffList.append(Diff(Diff::Delete, text1));
            diffList.append(Diff(Diff::Insert, text2));
            return diffList;
        }
    }

    if (m_currentDiffMode != Differ::CharMode && text1.size() > 80 && text2.size() > 80)
        return diffNonCharMode(text1, text2);

    return diffMyers(text1, text2);
}

QList<Diff> Differ::diffMyers(const QString &text1, const QString &text2)
{
    const qsizetype n = text1.size();
    const qsizetype m = text2.size();
    const bool odd = (n + m) % 2;
    const qsizetype D = odd ? (n + m) / 2 + 1 : (n + m) / 2;
    const qsizetype delta = n - m;
    const qsizetype vShift = D;
    std::vector<qsizetype> forwardV(2 * D + 1);
    std::vector<qsizetype> reverseV(2 * D + 1);
    for (qsizetype i = 0; i <= 2 * D; i++) {
        forwardV[i] = -1;
        reverseV[i] = -1;
    }
    forwardV[vShift + 1] = 0;
    reverseV[vShift + 1] = 0;
    qsizetype kMinForward = -D;
    qsizetype kMaxForward = D;
    qsizetype kMinReverse = -D;
    qsizetype kMaxReverse = D;
    for (qsizetype d = 0; d <= D; d++) {
        // going forward
        for (qsizetype k = qMax(-d, kMinForward + qAbs(d + kMinForward) % 2);
             k <= qMin(d, kMaxForward - qAbs(d + kMaxForward) % 2);
             k = k + 2) {
            qsizetype x;
            if (k == -d || (k < d && forwardV[k + vShift - 1] < forwardV[k + vShift + 1]))
                x = forwardV[k + vShift + 1]; // copy vertically from diagonal k + 1, y increases, y may exceed the graph
            else
                x = forwardV[k + vShift - 1] + 1; // copy horizontally from diagonal k - 1, x increases, x may exceed the graph
            qsizetype y = x - k;

            if (x > n) {
                kMaxForward = k - 1; // we are beyond the graph (right border), don't check diagonals >= current k anymore
            } else if (y > m) {
                kMinForward = k + 1; // we are beyond the graph (bottom border), don't check diagonals <= current k anymore
            } else {
                // find snake
                while (x < n && y < m) {
                    if (text1.at(x) != text2.at(y))
                        break;
                    x++;
                    y++;
                }
                forwardV[k + vShift] = x;
                if (odd) { // check if overlap
                    if (k >= delta - (d - 1) && k <= delta + (d - 1)) {
                        if (n - reverseV[delta - k + vShift] <= x) {
                            return diffMyersSplit(text1, x, text2, y);
                        }
                    }
                }
            }
        }
        // in reverse direction
        for (qsizetype k = qMax(-d, kMinReverse + qAbs(d + kMinReverse) % 2);
             k <= qMin(d, kMaxReverse - qAbs(d + kMaxReverse) % 2);
             k = k + 2) {
            qsizetype x;
            if (k == -d || (k < d && reverseV[k + vShift - 1] < reverseV[k + vShift + 1]))
                x = reverseV[k + vShift + 1];
            else
                x = reverseV[k + vShift - 1] + 1;
            qsizetype y = x - k;

            if (x > n) {
                kMaxReverse = k - 1; // we are beyond the graph (right border), don't check diagonals >= current k anymore
            } else if (y > m) {
                kMinReverse = k + 1; // we are beyond the graph (bottom border), don't check diagonals <= current k anymore
            } else {
                // find snake
                while (x < n && y < m) {
                    if (text1.at(n - x - 1) != text2.at(m - y - 1))
                        break;
                    x++;
                    y++;
                }
                reverseV[k + vShift] = x;
                if (!odd) { // check if overlap
                    if (k >= delta - d && k <= delta + d) {
                        if (n - forwardV[delta - k + vShift] <= x) {
                            return diffMyersSplit(text1, n - x, text2, m - x + k);
                        }
                    }
                }
            }
        }
    }
    // Completely different
    QList<Diff> diffList;
    diffList.append(Diff(Diff::Delete, text1));
    diffList.append(Diff(Diff::Insert, text2));
    return diffList;
}

QList<Diff> Differ::diffMyersSplit(
        const QString &text1, qsizetype x,
        const QString &text2, qsizetype y)
{
    const QString text11 = text1.left(x);
    const QString text12 = text1.mid(x);
    const QString text21 = text2.left(y);
    const QString text22 = text2.mid(y);

    const QList<Diff> &diffList1 = preprocess1AndDiff(text11, text21);
    const QList<Diff> &diffList2 = preprocess1AndDiff(text12, text22);
    return diffList1 + diffList2;
}

QList<Diff> Differ::diffNonCharMode(const QString &text1, const QString &text2)
{
    QString encodedText1;
    QString encodedText2;
    QStringList subtexts = encode(text1, text2, &encodedText1, &encodedText2);

    DiffMode diffMode = m_currentDiffMode;
    m_currentDiffMode = CharMode;

    // Each different subtext is a separate symbol
    // process these symbols as text with bigger alphabet
    QList<Diff> diffList = preprocess1AndDiff(encodedText1, encodedText2);

    diffList = decode(diffList, subtexts);

    QString lastDelete;
    QString lastInsert;
    QList<Diff> newDiffList;
    for (qsizetype i = 0; i <= diffList.size(); i++) {
        const Diff diffItem = i < diffList.size()
                  ? diffList.at(i)
                  : Diff(Diff::Equal); // dummy, ensure we process to the end
                                       // even when diffList doesn't end with equality
        if (diffItem.command == Diff::Delete) {
            lastDelete += diffItem.text;
        } else if (diffItem.command == Diff::Insert) {
            lastInsert += diffItem.text;
        } else { // Diff::Equal
            if (!(lastDelete.isEmpty() && lastInsert.isEmpty())) {
                // Rediff here on char basis
                newDiffList += preprocess1AndDiff(lastDelete, lastInsert);

                lastDelete.clear();
                lastInsert.clear();
            }
            newDiffList.append(diffItem);
        }
    }

    m_currentDiffMode = diffMode;
    return newDiffList;
}

QStringList Differ::encode(const QString &text1,
                                  const QString &text2,
                                  QString *encodedText1,
                                  QString *encodedText2)
{
    QStringList lines{{}}; // don't use code: 0
    QHash<QString, qsizetype> lineToCode;

    *encodedText1 = encode(text1, &lines, &lineToCode);
    *encodedText2 = encode(text2, &lines, &lineToCode);

    return lines;
}

qsizetype Differ::findSubtextEnd(const QString &text,
                                  qsizetype subtextStart)
{
    if (m_currentDiffMode == Differ::LineMode) {
        qsizetype subtextEnd = text.indexOf(u'\n', subtextStart);
        if (subtextEnd == -1)
            subtextEnd = text.size() - 1;
        return ++subtextEnd;
    } else if (m_currentDiffMode == Differ::WordMode) {
        if (!text.at(subtextStart).isLetter())
            return subtextStart + 1;
        qsizetype i = subtextStart + 1;

        const qsizetype count = text.size();
        while (i < count && text.at(i).isLetter())
            i++;
        return i;
    }
    return subtextStart + 1; // CharMode
}

QString Differ::encode(const QString &text,
                              QStringList *lines,
                              QHash<QString, qsizetype> *lineToCode)
{
    qsizetype subtextStart = 0;
    qsizetype subtextEnd = -1;
    QString codes;
    while (subtextEnd < text.size()) {
        subtextEnd = findSubtextEnd(text, subtextStart);
        const QString line = text.mid(subtextStart, subtextEnd - subtextStart);
        subtextStart = subtextEnd;

        if (lineToCode->contains(line)) {
            codes += QChar(static_cast<ushort>(lineToCode->value(line)));
        } else {
            lines->append(line);
            lineToCode->insert(line, lines->size() - 1);
            codes += QChar(static_cast<ushort>(lines->size() - 1));
        }
    }
    return codes;
}

QList<Diff> Differ::merge(const QList<Diff> &diffList)
{
    QString lastDelete;
    QString lastInsert;
    QList<Diff> newDiffList;
    for (qsizetype i = 0; i <= diffList.size(); i++) {
        Diff diff = i < diffList.size()
                  ? diffList.at(i)
                  : Diff(Diff::Equal); // dummy, ensure we process to the end
                                       // even when diffList doesn't end with equality
        if (diff.command == Diff::Delete) {
            lastDelete += diff.text;
        } else if (diff.command == Diff::Insert) {
            lastInsert += diff.text;
        } else { // Diff::Equal
            if (!(lastDelete.isEmpty() && lastInsert.isEmpty())) {

                // common prefix
                const qsizetype prefixCount = commonPrefix(lastDelete, lastInsert);
                if (prefixCount) {
                    const QString prefix = lastDelete.left(prefixCount);
                    lastDelete = lastDelete.mid(prefixCount);
                    lastInsert = lastInsert.mid(prefixCount);

                    if (!newDiffList.isEmpty()
                            && newDiffList.last().command == Diff::Equal) {
                        newDiffList.last().text += prefix;
                    } else {
                        newDiffList.append(Diff(Diff::Equal, prefix));
                    }
                }

                // common suffix
                const qsizetype suffixCount = commonSuffix(lastDelete, lastInsert);
                if (suffixCount) {
                    const QString suffix = lastDelete.right(suffixCount);
                    lastDelete = lastDelete.left(lastDelete.size() - suffixCount);
                    lastInsert = lastInsert.left(lastInsert.size() - suffixCount);

                    diff.text.prepend(suffix);
                }

                // append delete / insert / equal
                if (!lastDelete.isEmpty())
                    newDiffList.append(Diff(Diff::Delete, lastDelete));
                if (!lastInsert.isEmpty())
                    newDiffList.append(Diff(Diff::Insert, lastInsert));
                if (!diff.text.isEmpty())
                    newDiffList.append(diff);
                lastDelete.clear();
                lastInsert.clear();
            } else { // join with last equal diff
                if (!newDiffList.isEmpty()
                        && newDiffList.last().command == Diff::Equal) {
                    newDiffList.last().text += diff.text;
                } else {
                    if (!diff.text.isEmpty())
                        newDiffList.append(diff);
                }
            }
        }
    }

    QList<Diff> squashedDiffList = squashEqualities(newDiffList);
    if (squashedDiffList.size() != newDiffList.size())
        return merge(squashedDiffList);

    return squashedDiffList;
}

} // namespace QQmlLSUtils

QT_END_NAMESPACE
