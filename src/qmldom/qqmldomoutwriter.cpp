// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomformatdirectivescanner_p.h"
#include "qqmldomoutwriter_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomindentinglinewriter_p.h"
#include "qqmldomitem_p.h"
#include "qqmldomcomments_p.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using DisabledRegionIt = OutWriter::OffsetToDisabledRegionMap::const_iterator;

static inline OutWriter::RegionToCommentMap extractComments(const DomItem &it)
{
    OutWriter::RegionToCommentMap comments;
    DomItem commentsField = it.field(Fields::comments);
    if (const RegionComments *cRegionsPtr = commentsField.as<RegionComments>()) {
        comments = cRegionsPtr->regionComments();
    }
    return comments;
}

/*
\internal
\brief Utility function to determine if two source locations overlap
*/
static inline bool overlaps(const SourceLocation &a, const SourceLocation &b)
{
    return a.isValid() && b.isValid() && (a.begin() < b.end() && b.begin() < a.end());
}

/*
\internal
\brief Utility function to determine the indent after skipping to format a piece of code
*/
static inline int indentAfterPartialFormatting(int initialIndent, QStringView code,
                                               LineWriterOptions options)
{
    FormatTextStatus initialState = FormatTextStatus::initialStatus(initialIndent);
    FormatPartialStatus partialStatus({}, options.formatOptions, initialState);
    IndentingLineWriter indentingLineWriter([](QStringView line) { Q_UNUSED(line) }, QString(),
                                            options, partialStatus.currentStatus);
    OutWriter indentTracker(indentingLineWriter);
    const auto commentLines = code.split(u'\n');
    for (const auto &line : commentLines) {
        if (!line.isEmpty()) {
            partialStatus =
                    formatCodeLine(line, options.formatOptions, partialStatus.currentStatus);
            indentTracker.write(line);
        }
    }

    return indentTracker.indent;
}

/*
\internal
\brief Utility function to determine if a given location overlapping disabled region, returns an
iterator to the region if found, or end() if not found
*/
static inline DisabledRegionIt
findOverlappingRegion(const SourceLocation &loc,
                      const OutWriter::OffsetToDisabledRegionMap &formatDisabledRegions)
{
    if (!loc.isValid())
        return formatDisabledRegions.cend();

    return std::find_if(formatDisabledRegions.cbegin(), formatDisabledRegions.cend(),
                        [&loc](const auto &it) { return it.isValid() && overlaps(loc, it); });
}

QStringView OutWriter::attachedDisableCode(quint32 offset) const
{
    if (formatDisabledRegions.contains(offset)) {
        const auto &loc = formatDisabledRegions.value(offset);
        return code.mid(loc.offset, loc.length);
    }
    return {};
}

// This function examines the provided SourceLocation to determine if it overlaps with any regions
// where formatting is disabled. If such a region is found and the formatter is currently enabled,
// it writes the disabled region and disables the formatter. If no overlapping region is found,
// the formatter is enabled.
void OutWriter::maybeWriteDisabledRegion(const SourceLocation &loc)
{
    if (!loc.isValid())
        return;
    if (formatDisabledRegions.isEmpty())
        return;
    if (const auto foundRegionIt = findOverlappingRegion(loc, formatDisabledRegions);
        foundRegionIt != formatDisabledRegions.end()) {
        if (isFormatterEnabled) {
            writeDisabledRegion(loc);
            isFormatterEnabled = false;
        }
    } else {
        isFormatterEnabled = true;
    }
}

// Decides whether the given region should be formatted or not, based on the
// disabled regions found in the file and updates and returns the formatting enabled state.
bool OutWriter::shouldFormat(const FileLocations::Tree &fLoc, FileLocationRegion region)
{
    if (!fLoc || formatDisabledRegions.isEmpty())
        return isFormatterEnabled;

    if (const auto regions = fLoc->info().regions; regions.contains(region)) {
        isFormatterEnabled = findOverlappingRegion(regions.value(region), formatDisabledRegions)
                == formatDisabledRegions.end();
    }
    return isFormatterEnabled;
}

void OutWriter::scanFormatDirectives(QStringView code, const QList<SourceLocation> &comments)
{
    // Disabled regions cannot be effective if the line writer options
    // are set to normalize or sort imports.
    const auto shouldScanDirectives = lineWriter.options().attributesSequence
                    != LineWriterOptions::AttributesSequence::Normalize
            && !lineWriter.options().sortImports;
    if (!shouldScanDirectives)
        return;

    formatDisabledRegions = QmlFormat::identifyDisabledRegions(code, comments);
}

bool OutWriter::formatterEnabled() const
{
    return isFormatterEnabled;
}

void OutWriter::writeDisabledRegion(const SourceLocation &loc)
{
    const auto disabledCode = attachedDisableCode(loc.offset);
    int newIndent = indentAfterPartialFormatting(indent, disabledCode, lineWriter.options());
    lineWriter.ensureNewline();
    lineWriter.setLineIndent(0);
    indentNextlines = false;
    lineWriter.write(disabledCode);
    lineWriter.setLineIndent(newIndent);
    indentNextlines = true;
}

void OutWriter::maybeWriteComment(const Comment &comment)
{
    maybeWriteDisabledRegion(comment.sourceLocation());

    if (!skipComments && formatterEnabled()) {
        comment.write(*this);
    }

    // if disabled, maybe reenabled with this comment
    if (!formatterEnabled()) {
        auto directive = QmlFormat::directiveFromComment(comment.rawComment());
        if (directive == QmlFormat::Directive::On)
            isFormatterEnabled = true;
    }
}

void OutWriter::itemStart(const DomItem &it)
{
    if (skipComments)
        return;

    pendingComments.push(extractComments(it));
    writePreComment(MainRegion);
}

void OutWriter::itemEnd()
{
    if (skipComments)
        return;

    Q_ASSERT(!pendingComments.isEmpty());
    writePostComment(MainRegion);
    pendingComments.pop();
}

void OutWriter::writePreComment(FileLocationRegion region)
{
    if (skipComments)
        return;

    const auto &comments = pendingComments.top();
    if (comments.contains(region)) {
        const auto attachedComments = comments[region];
        for (const auto &comment : attachedComments.preComments())
            maybeWriteComment(comment);
    }
}

void OutWriter::writePostComment(FileLocationRegion region)
{
    if (skipComments)
        return;

    auto &comments = pendingComments.top();
    if (comments.contains(region)) {
        const auto attachedComments = comments[region];
        for (const auto &comment : attachedComments.postComments())
            maybeWriteComment(comment);
        comments.remove(region);
    }
}

static bool regionIncreasesIndentation(FileLocationRegion region)
{
    switch (region) {
    case LeftBraceRegion:
        return true;
    case LeftBracketRegion:
        return true;
    default:
        return false;
    }
    Q_UNREACHABLE_RETURN(false);
}

static bool regionDecreasesIndentation(FileLocationRegion region)
{
    switch (region) {
    case RightBraceRegion:
        return true;
    case RightBracketRegion:
        return true;
    default:
        return false;
    }
    Q_UNREACHABLE_RETURN(false);
}

/*!
\internal
Helper method for writeRegion(FileLocationRegion region) that allows to use
\c{writeRegion(ColonTokenRegion);} instead of having to write out the more error-prone
\c{writeRegion(ColonTokenRegion, ":");} for tokens and keywords.
*/
// TODO(QTBUG-138020)
OutWriter &OutWriter::writeRegion(const FileLocations::Tree &fLoc, FileLocationRegion region)
{
    using namespace Qt::Literals::StringLiterals;
    QString codeForRegion;
    switch (region) {
    case ComponentKeywordRegion:
        codeForRegion = u"component"_s;
        break;
    case IdColonTokenRegion:
    case ColonTokenRegion:
        codeForRegion = u":"_s;
        break;
    case ImportTokenRegion:
        codeForRegion = u"import"_s;
        break;
    case AsTokenRegion:
        codeForRegion = u"as"_s;
        break;
    case OnTokenRegion:
        codeForRegion = u"on"_s;
        break;
    case IdTokenRegion:
        codeForRegion = u"id"_s;
        break;
    case LeftBraceRegion:
        codeForRegion = u"{"_s;
        break;
    case RightBraceRegion:
        codeForRegion = u"}"_s;
        break;
    case LeftBracketRegion:
        codeForRegion = u"["_s;
        break;
    case RightBracketRegion:
        codeForRegion = u"]"_s;
        break;
    case LeftParenthesisRegion:
        codeForRegion = u"("_s;
        break;
    case RightParenthesisRegion:
        codeForRegion = u")"_s;
        break;
    case EnumKeywordRegion:
        codeForRegion = u"enum"_s;
        break;
    case DefaultKeywordRegion:
        codeForRegion = u"default"_s;
        break;
    case RequiredKeywordRegion:
        codeForRegion = u"required"_s;
        break;
    case ReadonlyKeywordRegion:
        codeForRegion = u"readonly"_s;
        break;
    case PropertyKeywordRegion:
        codeForRegion = u"property"_s;
        break;
    case FunctionKeywordRegion:
        codeForRegion = u"function"_s;
        break;
    case SignalKeywordRegion:
        codeForRegion = u"signal"_s;
        break;
    case ReturnKeywordRegion:
        codeForRegion = u"return"_s;
        break;
    case EllipsisTokenRegion:
        codeForRegion = u"..."_s;
        break;
    case EqualTokenRegion:
        codeForRegion = u"="_s;
        break;
    case PragmaKeywordRegion:
        codeForRegion = u"pragma"_s;
        break;
    case CommaTokenRegion:
        codeForRegion = u","_s;
        break;
    case ForKeywordRegion:
        codeForRegion = u"for"_s;
        break;
    case ElseKeywordRegion:
        codeForRegion = u"else"_s;
        break;
    case DoKeywordRegion:
        codeForRegion = u"do"_s;
        break;
    case WhileKeywordRegion:
        codeForRegion = u"while"_s;
        break;
    case TryKeywordRegion:
        codeForRegion = u"try"_s;
        break;
    case CatchKeywordRegion:
        codeForRegion = u"catch"_s;
        break;
    case FinallyKeywordRegion:
        codeForRegion = u"finally"_s;
        break;
    case CaseKeywordRegion:
        codeForRegion = u"case"_s;
        break;
    case ThrowKeywordRegion:
        codeForRegion = u"throw"_s;
        break;
    case ContinueKeywordRegion:
        codeForRegion = u"continue"_s;
        break;
    case BreakKeywordRegion:
        codeForRegion = u"break"_s;
        break;
    case QuestionMarkTokenRegion:
        codeForRegion = u"?"_s;
        break;
    case SemicolonTokenRegion:
        codeForRegion = u";"_s;
        break;
    case IfKeywordRegion:
        codeForRegion = u"if"_s;
        break;
    case SwitchKeywordRegion:
        codeForRegion = u"switch"_s;
        break;
    case YieldKeywordRegion:
        codeForRegion = u"yield"_s;
        break;
    case NewKeywordRegion:
        codeForRegion = u"new"_s;
        break;
    case ThisKeywordRegion:
        codeForRegion = u"this"_s;
        break;
    case SuperKeywordRegion:
        codeForRegion = u"super"_s;
        break;
    case StarTokenRegion:
        codeForRegion = u"*"_s;
        break;
    case DollarLeftBraceTokenRegion:
        codeForRegion = u"${"_s;
        break;
    case LeftBacktickTokenRegion:
    case RightBacktickTokenRegion:
        codeForRegion = u"`"_s;
        break;
    case VirtualKeywordRegion:
        codeForRegion = u"virtual"_s;
        break;
    case OverrideKeywordRegion:
        codeForRegion = u"override"_s;
        break;
    case FinalKeywordRegion:
        codeForRegion = u"final"_s;
        break;
    // not keywords:
    case ImportUriRegion:
    case IdNameRegion:
    case IdentifierRegion:
    case PragmaValuesRegion:
    case MainRegion:
    case OnTargetRegion:
    case TypeIdentifierRegion:
    case TypeModifierRegion:
    case FirstSemicolonTokenRegion:
    case SecondSemicolonRegion:
    case InOfTokenRegion:
    case OperatorTokenRegion:
    case VersionRegion:
    case EnumValueRegion:
        Q_ASSERT_X(false, "regionToString", "Using regionToString on a value or an identifier!");
        return *this;
    }

    return writeRegion(fLoc, region, codeForRegion);
}

OutWriter &OutWriter::writeRegion(const FileLocations::Tree &fLoc, FileLocationRegion region,
                                  QStringView toWrite)
{
    writePreComment(region);
    if (regionDecreasesIndentation(region))
        decreaseIndent(1);
    if (shouldFormat(fLoc, region))
        lineWriter.write(toWrite);
    if (regionIncreasesIndentation(region))
        increaseIndent(1);
    writePostComment(region);
    return *this;
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
