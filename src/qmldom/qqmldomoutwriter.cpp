// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomoutwriter_p.h"
#include "qqmldomattachedinfo_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomitem_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldomexternalitems_p.h"
#include "qqmldomtop_p.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

OutWriterState::OutWriterState(Path itCanonicalPath, const DomItem &it, FileLocations::Tree fLoc)
    : itemCanonicalPath(itCanonicalPath), item(it), currentMap(fLoc)
{
    DomItem cRegions = it.field(Fields::comments);
    if (const RegionComments *cRegionsPtr = cRegions.as<RegionComments>()) {
        pendingComments = cRegionsPtr->regionComments;
        fLoc->info().ensureCommentLocations(pendingComments.keys());
    }
}

void OutWriterState::closeState(OutWriter &w)
{
    if (w.lineWriter.options().updateOptions & LineWriterOptions::Update::Locations)
        w.lineWriter.endSourceLocation(fullRegionId);
    if (!pendingRegions.isEmpty()) {
        qCWarning(writeOutLog) << "PendingRegions non empty when closing item"
                               << pendingRegions.keys();
        auto iend = pendingRegions.end();
        auto it = pendingRegions.begin();
        while (it == iend) {
            w.lineWriter.endSourceLocation(it.value());
            ++it;
        }
    }
    if (!w.skipComments && !pendingComments.isEmpty())
        qCWarning(writeOutLog) << "PendingComments when closing item "
                               << item.canonicalPath().toString() << "for regions"
                               << pendingComments.keys();
}

OutWriterState &OutWriter::state(int i)
{
    return states[states.size() - 1 - i];
}

void OutWriter::itemStart(const DomItem &it)
{
    if (!topLocation->path())
        topLocation->setPath(it.canonicalPath());
    bool updateLocs = lineWriter.options().updateOptions & LineWriterOptions::Update::Locations;
    FileLocations::Tree newFLoc = topLocation;
    Path itP = it.canonicalPath();
    if (updateLocs) {
        if (!states.isEmpty()
            && states.last().itemCanonicalPath
                    == itP.mid(0, states.last().itemCanonicalPath.length())) {
            int oldL = states.last().itemCanonicalPath.length();
            newFLoc = FileLocations::ensure(states.last().currentMap,
                                            itP.mid(oldL, itP.length() - oldL),
                                            AttachedInfo::PathType::Relative);

        } else {
            newFLoc = FileLocations::ensure(topLocation, itP, AttachedInfo::PathType::Canonical);
        }
    }
    states.append(OutWriterState(itP, it, newFLoc));
    if (updateLocs)
        state().fullRegionId = lineWriter.startSourceLocation(
                [newFLoc](SourceLocation l) { FileLocations::updateFullLocation(newFLoc, l); });
    regionStart(MainRegion);
}

void OutWriter::itemEnd(const DomItem &it)
{
    Q_ASSERT(states.size() > 0);
    Q_ASSERT(state().item == it);
    regionEnd(MainRegion);
    state().closeState(*this);
    states.removeLast();
}

void OutWriter::regionStart(FileLocationRegion region)
{
    Q_ASSERT(!state().pendingRegions.contains(region));
    FileLocations::Tree fMap = state().currentMap;
    if (!skipComments && state().pendingComments.contains(region)) {
        bool updateLocs = lineWriter.options().updateOptions & LineWriterOptions::Update::Locations;
        QList<SourceLocation> *cLocs =
                (updateLocs ? &(fMap->info().preCommentLocations[region]) : nullptr);
        state().pendingComments[region].writePre(*this, cLocs);
    }
    state().pendingRegions[region] = lineWriter.startSourceLocation(
            [region, fMap](SourceLocation l) { FileLocations::addRegion(fMap, region, l); });
}

void OutWriter::regionEnd(FileLocationRegion region)
{
    Q_ASSERT(state().pendingRegions.contains(region));
    FileLocations::Tree fMap = state().currentMap;
    lineWriter.endSourceLocation(state().pendingRegions.value(region));
    state().pendingRegions.remove(region);
    if (state().pendingComments.contains(region)) {
        if (!skipComments) {
            bool updateLocs =
                    lineWriter.options().updateOptions & LineWriterOptions::Update::Locations;
            QList<SourceLocation> *cLocs =
                    (updateLocs ? &(fMap->info().postCommentLocations[region]) : nullptr);
            state().pendingComments[region].writePost(*this, cLocs);
        }
        state().pendingComments.remove(region);
    }
}

/*!
\internal
Helper method for writeRegion(FileLocationRegion region) that allows to use
\c{writeRegion(ColonTokenRegion);} instead of having to write out the more error-prone
\c{writeRegion(ColonTokenRegion, ":");} for tokens and keywords.
*/
OutWriter &OutWriter::writeRegion(FileLocationRegion region)
{
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

    // not keywords:
    case ImportUriRegion:
    case IdNameRegion:
    case IdentifierRegion:
    case PragmaValuesRegion:
    case MainRegion:
    case OnTargetRegion:
    case TypeIdentifierRegion:
    case FirstSemicolonTokenRegion:
    case SecondSemicolonRegion:
    case InOfTokenRegion:
    case OperatorTokenRegion:
        Q_ASSERT_X(false, "regionToString", "Using regionToString on a value or an identifier!");
        return *this;
    }

    return writeRegion(region, codeForRegion);
}

OutWriter &OutWriter::writeRegion(FileLocationRegion region, QStringView toWrite)
{
    regionStart(region);
    lineWriter.write(toWrite);
    regionEnd(region);
    return *this;
}
/*!
\internal
Creates a DomEnv which will contain a reformatted file also finalises reformatting of the file
*/
DomItem OutWriter::updatedFile(const DomItem &fileItem)
{
    // TODO(QTBUG-117849) unify for JS and QML files
    if (std::shared_ptr<JsFile> jsFilePtr = fileItem.ownerAs<JsFile>()) {
        std::shared_ptr<JsFile> copyPtr = jsFilePtr->makeCopy(fileItem);
        DomItem env = fileItem.environment();
        std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>();
        Q_ASSERT(envPtr);
        auto newEnvPtr =
                std::make_shared<DomEnvironment>(envPtr, envPtr->loadPaths(), envPtr->options());
        newEnvPtr->addJsFile(copyPtr);
        MutableDomItem copy = MutableDomItem(DomItem(newEnvPtr).copy(copyPtr));
        FileLocations::Tree newLoc = topLocation;
        Path filePath = fileItem.canonicalPath();
        if (newLoc->path() != filePath) {
            if (newLoc->path()) {
                if (newLoc->path().length() > filePath.length()
                    && newLoc->path().mid(0, filePath.length()) == filePath) {
                    newLoc = FileLocations::createTree(filePath);
                    FileLocations::Tree loc =
                            FileLocations::ensure(newLoc, newLoc->path().mid(filePath.length()),
                                                  AttachedInfo::PathType::Relative);
                    loc->setSubItems(topLocation->subItems());
                } else {
                    qCWarning(writeOutLog)
                            << "failed to base fileLocations in OutWriter (" << newLoc->path()
                            << ") to current file (" << filePath << ")";
                }
            } else {
                newLoc = FileLocations::createTree(filePath);
                Q_ASSERT(newLoc->subItems().isEmpty() && newLoc->info().regions.isEmpty());
            }
        }
        copyPtr->setFileLocationsTree(newLoc);
        UpdatedScriptExpression::visitTree(
                reformattedScriptExpressions,
                [&copy, filePath](Path p, UpdatedScriptExpression::Tree t) {
                    if (std::shared_ptr<ScriptExpression> exprPtr = t->info().expr) {
                        Q_ASSERT(p.mid(0, filePath.length()) == filePath);
                        //Set reformatted expression to the JsFile
                        //hacky workaround to avoid mutating DOM API
                        copy.mutableAs<JsFile>()->setExpression(exprPtr);
                    }
                    return true;
                });
        return copy.item();
    }
    Q_ASSERT(fileItem.internalKind() == DomType::QmlFile);
    if (std::shared_ptr<QmlFile> qmlFilePtr = fileItem.ownerAs<QmlFile>()) {
        std::shared_ptr<QmlFile> copyPtr = qmlFilePtr->makeCopy(fileItem);
        DomItem env = fileItem.environment();
        std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>();
        Q_ASSERT(envPtr);
        auto newEnvPtr = std::make_shared<DomEnvironment>(
                envPtr, envPtr->loadPaths(), envPtr->options());
        newEnvPtr->addQmlFile(copyPtr);
        MutableDomItem copy = MutableDomItem(DomItem(newEnvPtr).copy(copyPtr));
        FileLocations::Tree newLoc = topLocation;
        Path qmlFilePath = fileItem.canonicalPath();
        if (newLoc->path() != qmlFilePath) {
            if (newLoc->path()) {
                if (newLoc->path().length() > qmlFilePath.length()
                    && newLoc->path().mid(0, qmlFilePath.length()) == qmlFilePath) {
                    newLoc = FileLocations::createTree(qmlFilePath);
                    FileLocations::Tree loc =
                            FileLocations::ensure(newLoc, newLoc->path().mid(qmlFilePath.length()),
                                                  AttachedInfo::PathType::Relative);
                    loc->setSubItems(topLocation->subItems());
                } else {
                    qCWarning(writeOutLog)
                            << "failed to base fileLocations in OutWriter (" << newLoc->path()
                            << ") to current file (" << qmlFilePath << ")";
                }
            } else {
                newLoc = FileLocations::createTree(qmlFilePath);
                Q_ASSERT(newLoc->subItems().isEmpty() && newLoc->info().regions.isEmpty());
            }
        }
        copyPtr->setFileLocationsTree(newLoc);
        UpdatedScriptExpression::visitTree(
                reformattedScriptExpressions,
                [&copy, qmlFilePath](Path p, UpdatedScriptExpression::Tree t) {
                    if (std::shared_ptr<ScriptExpression> exprPtr = t->info().expr) {
                        Q_ASSERT(p.mid(0, qmlFilePath.length()) == qmlFilePath);
                        MutableDomItem targetExpr = copy.path(p.mid(qmlFilePath.length()));
                        if (!targetExpr)
                            qCWarning(writeOutLog) << "failed to get" << p.mid(qmlFilePath.length())
                                                   << "from" << copy.canonicalPath();
                        else if (exprPtr->ast()
                                 || (!targetExpr.as<ScriptExpression>()
                                     || !targetExpr.as<ScriptExpression>()->ast()))
                            targetExpr.setScript(exprPtr);
                        else {
                            qCWarning(writeOutLog).noquote()
                                    << "Skipped update of reformatted ScriptExpression with "
                                       "code:\n---------------\n"
                                    << exprPtr->code() << "\n---------------\n preCode:" <<
                                    [exprPtr](Sink s) { sinkEscaped(s, exprPtr->preCode()); }
                                    << "\n postCode: " <<
                                    [exprPtr](Sink s) { sinkEscaped(s, exprPtr->postCode()); }
                                    << "\n as it failed standalone reparse with errors:" <<
                                    [&targetExpr, exprPtr](Sink s) {
                                        targetExpr.item()
                                                .copy(exprPtr, targetExpr.canonicalPath())
                                                .iterateErrors(
                                                        [s](const DomItem &, const ErrorMessage &msg) {
                                                            s(u"\n  ");
                                                            msg.dump(s);
                                                            return true;
                                                        },
                                                        true);
                                    }
                                    << "\n";
                        }
                    }
                    return true;
                });
        return copy.item();
    }
    return DomItem();
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
