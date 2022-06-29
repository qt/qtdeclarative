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

OutWriterState::OutWriterState(Path itCanonicalPath, DomItem &it, FileLocations::Tree fLoc)
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

void OutWriter::itemStart(DomItem &it)
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
    regionStart(QString());
}

void OutWriter::itemEnd(DomItem &it)
{
    Q_ASSERT(states.size() > 0);
    Q_ASSERT(state().item == it);
    regionEnd(QString());
    state().closeState(*this);
    states.removeLast();
}

void OutWriter::regionStart(QString rName)
{
    Q_ASSERT(!state().pendingRegions.contains(rName));
    FileLocations::Tree fMap = state().currentMap;
    if (!skipComments && state().pendingComments.contains(rName)) {
        bool updateLocs = lineWriter.options().updateOptions & LineWriterOptions::Update::Locations;
        QList<SourceLocation> *cLocs =
                (updateLocs ? &(fMap->info().preCommentLocations[rName]) : nullptr);
        state().pendingComments[rName].writePre(*this, cLocs);
    }
    state().pendingRegions[rName] = lineWriter.startSourceLocation(
            [rName, fMap](SourceLocation l) { FileLocations::addRegion(fMap, rName, l); });
}

void OutWriter::regionEnd(QString rName)
{
    Q_ASSERT(state().pendingRegions.contains(rName));
    FileLocations::Tree fMap = state().currentMap;
    lineWriter.endSourceLocation(state().pendingRegions.value(rName));
    state().pendingRegions.remove(rName);
    if (state().pendingComments.contains(rName)) {
        if (!skipComments) {
            bool updateLocs =
                    lineWriter.options().updateOptions & LineWriterOptions::Update::Locations;
            QList<SourceLocation> *cLocs =
                    (updateLocs ? &(fMap->info().postCommentLocations[rName]) : nullptr);
            state().pendingComments[rName].writePost(*this, cLocs);
        }
        state().pendingComments.remove(rName);
    }
}

OutWriter &OutWriter::writeRegion(QString rName, QStringView toWrite)
{
    regionStart(rName);
    lineWriter.write(toWrite);
    regionEnd(rName);
    return *this;
}

DomItem OutWriter::updatedFile(DomItem &qmlFile)
{
    Q_ASSERT(qmlFile.internalKind() == DomType::QmlFile);
    if (std::shared_ptr<QmlFile> qmlFilePtr = qmlFile.ownerAs<QmlFile>()) {
        std::shared_ptr<QmlFile> copyPtr = qmlFilePtr->makeCopy(qmlFile);
        DomItem env = qmlFile.environment();
        std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>();
        Q_ASSERT(envPtr);
        auto newEnvPtr = std::make_shared<DomEnvironment>(
                envPtr, envPtr->loadPaths(), envPtr->options());
        newEnvPtr->addQmlFile(copyPtr);
        MutableDomItem copy = MutableDomItem(DomItem(newEnvPtr).copy(copyPtr));
        FileLocations::Tree newLoc = topLocation;
        Path qmlFilePath = qmlFile.canonicalPath();
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
                                                        [s](DomItem, ErrorMessage msg) {
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
