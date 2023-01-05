// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlcompletionsupport.h"
#include "qqmllanguageserver.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/QRegularExpression>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_USE_NAMESPACE
using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(complLog, "qt.languageserver.completions")

void QmlCompletionSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerCompletionRequestHandler(
            [this](const QByteArray &, const CompletionParams &cParams,
                   LSPPartialResponse<
                           std::variant<QList<CompletionItem>, CompletionList, std::nullptr_t>,
                           std::variant<CompletionList, QList<CompletionItem>>> &&response) {
                QmlLsp::OpenDocument doc = m_codeModel->openDocumentByUrl(
                        QmlLsp::lspUriToQmlUrl(cParams.textDocument.uri));
                if (!doc.textDocument) {
                    response.sendResponse(QList<CompletionItem>());
                    return;
                }
                CompletionRequest *req = new CompletionRequest;
                std::optional<int> targetVersion;
                {
                    QMutexLocker l(doc.textDocument->mutex());
                    targetVersion = doc.textDocument->version();
                    if (!targetVersion) {
                        qCWarning(complLog) << "no target version for completions in "
                                            << QString::fromUtf8(cParams.textDocument.uri);
                    }
                    req->code = doc.textDocument->toPlainText();
                }
                req->minVersion = (targetVersion ? *targetVersion : 0);
                req->response = std::move(response);
                req->completionParams = cParams;
                {
                    QMutexLocker l(&m_mutex);
                    m_completions.insert(req->completionParams.textDocument.uri, req);
                }
                if (doc.snapshot.docVersion && *doc.snapshot.docVersion >= req->minVersion)
                    updatedSnapshot(QmlLsp::lspUriToQmlUrl(req->completionParams.textDocument.uri));
            });
    protocol->registerCompletionItemResolveRequestHandler(
            [](const QByteArray &, const CompletionItem &cParams,
               LSPResponse<CompletionItem> &&response) { response.sendResponse(cParams); });
}

QmlCompletionSupport::QmlCompletionSupport(QmlLsp::QQmlCodeModel *codeModel)
    : m_codeModel(codeModel)
{
    QObject::connect(m_codeModel, &QmlLsp::QQmlCodeModel::updatedSnapshot, this,
                     &QmlCompletionSupport::updatedSnapshot);
}

QmlCompletionSupport::~QmlCompletionSupport()
{
    QMutexLocker l(&m_mutex);
    qDeleteAll(m_completions);
    m_completions.clear();
}

QString QmlCompletionSupport::name() const
{
    return u"QmlCompletionSupport"_s;
}

void QmlCompletionSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    QLspSpecification::CompletionOptions cOptions;
    if (serverCapabilities.capabilities.completionProvider)
        cOptions = *serverCapabilities.capabilities.completionProvider;
    cOptions.resolveProvider = false;
    cOptions.triggerCharacters = QList<QByteArray>({ QByteArray(".") });
    serverCapabilities.capabilities.completionProvider = cOptions;
}

void QmlCompletionSupport::updatedSnapshot(const QByteArray &url)
{
    QmlLsp::OpenDocumentSnapshot doc = m_codeModel->snapshotByUrl(url);
    QList<CompletionRequest *> toCompl;
    {
        QMutexLocker l(&m_mutex);
        for (auto [it, end] = m_completions.equal_range(url); it != end; ++it) {
            if (doc.docVersion && it.value()->minVersion <= *doc.docVersion)
                toCompl.append(it.value());
        }
        if (!m_completions.isEmpty())
            qCDebug(complLog) << "updated " << QString::fromUtf8(url) << " v"
                              << (doc.docVersion ? (*doc.docVersion) : -1) << ", completing"
                              << m_completions.size() << "/" << m_completions.size();
        for (auto req : toCompl)
            m_completions.remove(url, req);
    }
    for (auto it = toCompl.rbegin(), end = toCompl.rend(); it != end; ++it) {
        CompletionRequest *req = *it;
        QThreadPool::globalInstance()->start([req, doc]() mutable {
            req->sendCompletions(doc);
            delete req;
        });
    }
}

struct ItemLocation
{
    int depth = -1;
    DomItem domItem;
    FileLocations::Tree fileLocation;
};

QString CompletionRequest::urlAndPos() const
{
    return QString::fromUtf8(completionParams.textDocument.uri) + u":"
            + QString::number(completionParams.position.line) + u":"
            + QString::number(completionParams.position.character);
}

// return the position of 0 based line and char offsets, never goes to the "next" line, but might
// return the position of the \n or \r that starts the next line (never the one that starts line)
static qsizetype posAfterLineChar(QString code, int line, int character)
{
    int targetLine = line;
    qsizetype i = 0;
    while (i != code.size() && targetLine != 0) {
        QChar c = code.at(i++);
        if (c == u'\n') {
            --targetLine;
        }
        if (c == u'\r') {
            if (i != code.size() && code.at(i) == u'\n')
                ++i;
            --targetLine;
        }
    }
    qsizetype leftChars = character;
    while (i != code.size() && leftChars) {
        QChar c = code.at(i);
        if (c == u'\n' || c == u'\r')
            break;
        ++i;
        if (!c.isLowSurrogate())
            --leftChars;
    }
    return i;
}

static QList<ItemLocation> findLastItemsContaining(DomItem file, int line, int character)
{
    QList<ItemLocation> itemsFound;
    std::shared_ptr<QmlFile> filePtr = file.ownerAs<QmlFile>();
    if (!filePtr)
        return itemsFound;
    FileLocations::Tree t = filePtr->fileLocationsTree();
    Q_ASSERT(t);
    QString code = filePtr->code(); // do something more advanced wrt to changes wrt to this->code?
    if (code.isEmpty())
        qCWarning(complLog) << "no code";
    QList<ItemLocation> toDo;
    qsizetype targetPos = posAfterLineChar(code, line, character);
    Q_ASSERT(targetPos >= 0);
    auto containsTarget = [targetPos](QQmlJS::SourceLocation l) {
        if constexpr (sizeof(qsizetype) <= sizeof(quint32)) {
            return l.begin() <= quint32(targetPos) && quint32(targetPos) < l.end();
        } else {
            return l.begin() <= targetPos && targetPos < l.end();
        }
    };
    if (containsTarget(t->info().fullRegion)) {
        ItemLocation loc;
        loc.depth = 0;
        loc.domItem = file;
        loc.fileLocation = t;
        toDo.append(loc);
    }
    while (!toDo.isEmpty()) {
        ItemLocation iLoc = toDo.last();
        toDo.removeLast();
        if (itemsFound.isEmpty() || itemsFound.constFirst().depth <= iLoc.depth) {
            if (!itemsFound.isEmpty() && itemsFound.constFirst().depth < iLoc.depth)
                itemsFound.clear();
            itemsFound.append(iLoc);
        }
        auto subEls = iLoc.fileLocation->subItems();
        for (auto it = subEls.begin(); it != subEls.end(); ++it) {
            auto subLoc = std::static_pointer_cast<AttachedInfoT<FileLocations>>(it.value());
            Q_ASSERT(subLoc);
            if (containsTarget(subLoc->info().fullRegion)) {
                ItemLocation subItem;
                subItem.depth = iLoc.depth + 1;
                subItem.domItem = iLoc.domItem.path(it.key());
                subItem.fileLocation = subLoc;
                toDo.append(subItem);
            }
        }
    }
    return itemsFound;
}

// finds the filter string, the base (for fully qualified accesses) and the whole string
// just before pos in code
struct CompletionContextStrings
{
    CompletionContextStrings(QString code, qsizetype pos);

public:
    // line up until pos
    QStringView preLine() const
    {
        return QStringView(m_code).mid(m_lineStart, m_pos - m_lineStart);
    }
    // the part used to filter the completion (normally actual filtering is left to the client)
    QStringView filterChars() const
    {
        return QStringView(m_code).mid(m_filterStart, m_pos - m_filterStart);
    }
    // the base part (qualified access)
    QStringView base() const
    {
        return QStringView(m_code).mid(m_baseStart, m_filterStart - m_baseStart);
    }
    // if we are at line start
    bool atLineStart() const { return m_atLineStart; }

private:
    QString m_code; // the current code
    qsizetype m_pos = {}; // current position of the cursor
    qsizetype m_filterStart = {}; // start of the characters that are used to filter the suggestions
    qsizetype m_lineStart = {}; // start of the current line
    qsizetype m_baseStart = {}; // start of the dotted expression that ends at the cursor position
    bool m_atLineStart = {}; // if there are only spaces before base
};

CompletionContextStrings::CompletionContextStrings(QString code, qsizetype pos)
    : m_code(code), m_pos(pos)
{
    // computes the context just before pos in code.
    // After this code all the values of all the attributes should be correct (see above)
    // handle also letter or numbers represented a surrogate pairs?
    m_filterStart = m_pos;
    while (m_filterStart != 0) {
        QChar c = code.at(m_filterStart - 1);
        if (!c.isLetterOrNumber() && c != u'_')
            break;
        else
            --m_filterStart;
    }
    // handle spaces?
    m_baseStart = m_filterStart;
    while (m_baseStart != 0) {
        QChar c = code.at(m_baseStart - 1);
        if (c != u'.' || m_baseStart == 1)
            break;
        c = code.at(m_baseStart - 2);
        if (!c.isLetterOrNumber() && c != u'_')
            break;
        qsizetype baseEnd = --m_baseStart;
        while (m_baseStart != 0) {
            QChar c = code.at(m_baseStart - 1);
            if (!c.isLetterOrNumber() && c != u'_')
                break;
            else
                --m_baseStart;
        }
        if (m_baseStart == baseEnd)
            break;
    }
    m_atLineStart = true;
    m_lineStart = m_baseStart;
    while (m_lineStart != 0) {
        QChar c = code.at(m_lineStart - 1);
        if (c == u'\n' || c == '\r')
            break;
        if (!c.isSpace())
            m_atLineStart = false;
        --m_lineStart;
    }
}

enum class TypeCompletionsType { None, Types, TypesAndAttributes };

enum class FunctionCompletion { None, Declaration };

enum class ImportCompletionType { None, Module, Version };

void CompletionRequest::sendCompletions(QmlLsp::OpenDocumentSnapshot &doc)
{
    QList<CompletionItem> res = completions(doc);
    response.sendResponse(res);
}

static QList<CompletionItem> importCompletions(DomItem &file, const CompletionContextStrings &ctx)
{
    // returns completions for import statements, ctx is supposed to be in an import statement
    QList<CompletionItem> res;
    ImportCompletionType importCompletionType = ImportCompletionType::None;
    QRegularExpression spaceRe(uR"(\W+)"_s);
    QList<QStringView> linePieces = ctx.preLine().split(spaceRe, Qt::SkipEmptyParts);
    qsizetype effectiveLength = linePieces.size()
            + ((!ctx.preLine().isEmpty() && ctx.preLine().last().isSpace()) ? 1 : 0);
    if (effectiveLength < 2) {
        CompletionItem comp;
        comp.label = "import";
        comp.kind = int(CompletionItemKind::Keyword);
        res.append(comp);
    }
    if (linePieces.isEmpty() || linePieces.first() != u"import")
        return res;
    if (effectiveLength == 2) {
        // the cursor is after the import, possibly in a partial module name
        importCompletionType = ImportCompletionType::Module;
    } else if (effectiveLength == 3) {
        if (linePieces.last() != u"as") {
            // the cursor is after the module, possibly in a partial version token (or partial as)
            CompletionItem comp;
            comp.label = "as";
            comp.kind = int(CompletionItemKind::Keyword);
            res.append(comp);
            importCompletionType = ImportCompletionType::Version;
        }
    }
    DomItem env = file.environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        switch (importCompletionType) {
        case ImportCompletionType::None:
            break;
        case ImportCompletionType::Module: {
            QDuplicateTracker<QString> modulesSeen;
            for (const QString &uri : envPtr->moduleIndexUris(env)) {
                QStringView base = ctx.base(); // if we allow spaces we should get rid of them
                if (uri.startsWith(base)) {
                    QStringList rest = uri.mid(base.size()).split(u'.');
                    if (rest.isEmpty())
                        continue;

                    const QString label = rest.first();
                    if (!modulesSeen.hasSeen(label)) {
                        CompletionItem comp;
                        comp.label = label.toUtf8();
                        comp.kind = int(CompletionItemKind::Module);
                        res.append(comp);
                    }
                }
            }
            break;
        }
        case ImportCompletionType::Version:
            if (ctx.base().isEmpty()) {
                for (int majorV :
                     envPtr->moduleIndexMajorVersions(env, linePieces.at(1).toString())) {
                    CompletionItem comp;
                    comp.label = QString::number(majorV).toUtf8();
                    comp.kind = int(CompletionItemKind::Constant);
                    res.append(comp);
                }
            } else {
                bool hasMajorVersion = ctx.base().endsWith(u'.');
                int majorV = -1;
                if (hasMajorVersion)
                    majorV = ctx.base().mid(0, ctx.base().size() - 1).toInt(&hasMajorVersion);
                if (!hasMajorVersion)
                    break;
                if (std::shared_ptr<ModuleIndex> mIndex =
                            envPtr->moduleIndexWithUri(env, linePieces.at(1).toString(), majorV)) {
                    for (int minorV : mIndex->minorVersions()) {
                        CompletionItem comp;
                        comp.label = QString::number(minorV).toUtf8();
                        comp.kind = int(CompletionItemKind::Constant);
                        res.append(comp);
                    }
                }
            }
            break;
        }
    }
    return res;
}

static QList<CompletionItem> idsCompletions(DomItem component)
{
    qCDebug(complLog) << "adding ids completions";
    QList<CompletionItem> res;
    for (const QString &k : component.field(Fields::ids).keys()) {
        CompletionItem comp;
        comp.label = k.toUtf8();
        comp.kind = int(CompletionItemKind::Value);
        res.append(comp);
    }
    return res;
}

static QList<CompletionItem> bindingsCompletions(DomItem &containingObject)
{
    // returns valid bindings completions (i.e. reachable properties and signal handlers)
    QList<CompletionItem> res;
    qCDebug(complLog) << "binding completions";
    containingObject.visitPrototypeChain(
            [&res](DomItem &it) {
                qCDebug(complLog) << "prototypeChain" << it.internalKindStr() << it.canonicalPath();
                if (const QmlObject *itPtr = it.as<QmlObject>()) {
                    // signal handlers
                    auto methods = itPtr->methods();
                    auto it = methods.cbegin();
                    while (it != methods.cend()) {
                        if (it.value().methodType == MethodInfo::MethodType::Signal) {
                            CompletionItem comp;
                            QString signal = it.key();
                            comp.label =
                                    (u"on"_s + signal.at(0).toUpper() + signal.mid(1)).toUtf8();
                            res.append(comp);
                        }
                        ++it;
                    }
                    // properties that can be bound
                    auto pDefs = itPtr->propertyDefs();
                    for (auto it2 = pDefs.keyBegin(); it2 != pDefs.keyEnd(); ++it2) {
                        qCDebug(complLog) << "adding property" << *it2;
                        CompletionItem comp;
                        comp.label = it2->toUtf8();
                        comp.insertText = (*it2 + u": "_s).toUtf8();
                        comp.kind = int(CompletionItemKind::Property);
                        res.append(comp);
                    }
                }
                return true;
            },
            VisitPrototypesOption::Normal);
    return res;
}

static QList<CompletionItem> reachableSymbols(DomItem &context, const CompletionContextStrings &ctx,
                                              TypeCompletionsType typeCompletionType,
                                              FunctionCompletion completeMethodCalls)
{
    // returns completions for the reachable types or attributes from context
    QList<CompletionItem> res;
    QMap<CompletionItemKind, QSet<QString>> symbols;
    QSet<quintptr> visited;
    QList<Path> visitedRefs;
    auto addLocalSymbols = [&res, typeCompletionType, completeMethodCalls, &symbols](DomItem &el) {
        switch (typeCompletionType) {
        case TypeCompletionsType::None:
            return false;
        case TypeCompletionsType::Types:
            switch (el.internalKind()) {
            case DomType::ImportScope: {
                const QSet<QString> localSymbols = el.localSymbolNames(
                        LocalSymbolsType::QmlTypes | LocalSymbolsType::Namespaces);
                qCDebug(complLog) << "adding local symbols of:" << el.internalKindStr()
                                  << el.canonicalPath() << localSymbols;
                symbols[CompletionItemKind::Class] += localSymbols;
                break;
            }
            default: {
                qCDebug(complLog) << "skipping local symbols for non type" << el.internalKindStr()
                                  << el.canonicalPath();
                break;
            }
            }
            break;
        case TypeCompletionsType::TypesAndAttributes:
            auto localSymbols = el.localSymbolNames(LocalSymbolsType::All);
            if (const QmlObject *elPtr = el.as<QmlObject>()) {
                auto methods = elPtr->methods();
                auto it = methods.cbegin();
                while (it != methods.cend()) {
                    localSymbols.remove(it.key());
                    if (completeMethodCalls == FunctionCompletion::Declaration) {
                        QStringList parameters;
                        for (const MethodParameter &pInfo : std::as_const(it->parameters)) {
                            QStringList param;
                            if (!pInfo.typeName.isEmpty())
                                param << pInfo.typeName;
                            if (!pInfo.name.isEmpty())
                                param << pInfo.name;
                            if (pInfo.defaultValue) {
                                param << u"= " + pInfo.defaultValue->code();
                            }
                            parameters.append(param.join(' '));
                        }

                        QString commentsStr;

                        if (!it->comments.regionComments.isEmpty()) {
                            for (const Comment &c : it->comments.regionComments[0].preComments) {
                                commentsStr += c.rawComment().toString().trimmed() + '\n';
                            }
                        }

                        CompletionItem comp;
                        comp.documentation =
                                u"%1%2(%3)"_s.arg(commentsStr, it.key(), parameters.join(u", "))
                                        .toUtf8();
                        comp.label = (it.key() + u"()").toUtf8();
                        comp.kind = int(CompletionItemKind::Function);

                        if (it->typeName.isEmpty())
                            comp.detail = "returns void";
                        else
                            comp.detail = (u"returns "_s + it->typeName).toUtf8();

                        // Only append full bracket if there are no parameters
                        if (it->parameters.isEmpty())
                            comp.insertText = comp.label;
                        else
                            // add snippet support?
                            comp.insertText = (it.key() + u"(").toUtf8();

                        res.append(comp);
                    }
                    ++it;
                }
            }
            qCDebug(complLog) << "adding local symbols of:" << el.internalKindStr()
                              << el.canonicalPath() << localSymbols;
            symbols[CompletionItemKind::Field] += localSymbols;
            break;
        }
        return true;
    };
    if (ctx.base().isEmpty()) {
        if (typeCompletionType != TypeCompletionsType::None) {
            qCDebug(complLog) << "adding symbols reachable from:" << context.internalKindStr()
                              << context.canonicalPath();
            DomItem it = context.proceedToScope();
            it.visitScopeChain(addLocalSymbols, LookupOption::Normal, &defaultErrorHandler,
                               &visited, &visitedRefs);
        }
    } else {
        QList<QStringView> baseItems = ctx.base().split(u'.', Qt::SkipEmptyParts);
        Q_ASSERT(!baseItems.isEmpty());
        auto addReachableSymbols = [&visited, &visitedRefs, &addLocalSymbols](Path,
                                                                              DomItem &it) -> bool {
            qCDebug(complLog) << "adding directly accessible symbols of" << it.internalKindStr()
                              << it.canonicalPath();
            it.visitDirectAccessibleScopes(addLocalSymbols, VisitPrototypesOption::Normal,
                                           &defaultErrorHandler, &visited, &visitedRefs);
            return true;
        };
        Path toSearch = Paths::lookupSymbolPath(ctx.base().toString().chopped(1));
        context.resolve(toSearch, addReachableSymbols, &defaultErrorHandler);
        // add attached types? technically we should...
    }
    for (auto symbolKinds = symbols.constBegin(); symbolKinds != symbols.constEnd();
         ++symbolKinds) {
        for (auto symbol = symbolKinds.value().constBegin();
             symbol != symbolKinds.value().constEnd(); ++symbol) {
            CompletionItem comp;
            comp.label = symbol->toUtf8();
            comp.kind = int(symbolKinds.key());
            res.append(comp);
        }
    }
    return res;
}

QList<CompletionItem> CompletionRequest::completions(QmlLsp::OpenDocumentSnapshot &doc) const
{
    QList<CompletionItem> res;
    if (!doc.validDoc) {
        qCWarning(complLog) << "No valid document for completions  for "
                            << QString::fromUtf8(completionParams.textDocument.uri);
        // try to add some import and global completions?
        return res;
    }
    if (!doc.docVersion || *doc.docVersion < minVersion) {
        qCWarning(complLog) << "sendCompletions on older doc version";
    } else if (!doc.validDocVersion || *doc.validDocVersion < minVersion) {
        qCWarning(complLog) << "using outdated valid doc, position might be incorrect";
    }
    DomItem file = doc.validDoc.fileObject(QQmlJS::Dom::GoTo::MostLikely);
    // clear reference cache to resolve latest versions (use a local env instead?)
    if (std::shared_ptr<DomEnvironment> envPtr = file.environment().ownerAs<DomEnvironment>())
        envPtr->clearReferenceCache();
    qsizetype pos = posAfterLineChar(code, completionParams.position.line,
                                     completionParams.position.character);
    CompletionContextStrings ctx(code, pos);
    QList<ItemLocation> itemsFound =
            findLastItemsContaining(file, completionParams.position.line,
                                    completionParams.position.character - ctx.filterChars().size());
    if (itemsFound.size() > 1) {
        QStringList paths;
        for (auto &it : itemsFound)
            paths.append(it.domItem.canonicalPath().toString());
        qCWarning(complLog) << "Multiple elements of " << urlAndPos()
                            << " at the same depth:" << paths << "(using first)";
    }
    DomItem currentItem;
    if (!itemsFound.isEmpty())
        currentItem = itemsFound.first().domItem;
    qCDebug(complLog) << "Completion at " << urlAndPos() << " " << completionParams.position.line
                      << ":" << completionParams.position.character << "offset:" << pos
                      << "base:" << ctx.base() << "filter:" << ctx.filterChars()
                      << "lastVersion:" << (doc.docVersion ? (*doc.docVersion) : -1)
                      << "validVersion:" << (doc.validDocVersion ? (*doc.validDocVersion) : -1)
                      << "in" << currentItem.internalKindStr() << currentItem.canonicalPath();
    DomItem containingObject = currentItem.qmlObject();
    TypeCompletionsType typeCompletionType = TypeCompletionsType::None;
    FunctionCompletion methodCompletion = FunctionCompletion::Declaration;

    if (!containingObject) {
        methodCompletion = FunctionCompletion::None;
        // global completions
        if (ctx.atLineStart()) {
            if (ctx.base().isEmpty()) {
                {
                    CompletionItem comp;
                    comp.label = "pragma";
                    comp.kind = int(CompletionItemKind::Keyword);
                    res.append(comp);
                }
            }
            typeCompletionType = TypeCompletionsType::Types;
        }
        // Import completion
        res += importCompletions(file, ctx);
    } else {
        methodCompletion = FunctionCompletion::Declaration;
        bool addIds = false;

        if (ctx.atLineStart() && currentItem.internalKind() != DomType::ScriptExpression
            && currentItem.internalKind() != DomType::List) {
            // add bindings
            methodCompletion = FunctionCompletion::None;
            if (ctx.base().isEmpty()) {
                for (const QStringView &s : std::array<QStringView, 5>(
                             { u"property", u"readonly", u"default", u"signal", u"function" })) {
                    CompletionItem comp;
                    comp.label = s.toUtf8();
                    comp.kind = int(CompletionItemKind::Keyword);
                    res.append(comp);
                }
                res += bindingsCompletions(containingObject);
                typeCompletionType = TypeCompletionsType::Types;
            } else {
                // handle value types later with type expansion
                typeCompletionType = TypeCompletionsType::TypesAndAttributes;
            }
        } else {
            addIds = true;
            typeCompletionType = TypeCompletionsType::TypesAndAttributes;
        }
        if (addIds) {
            res += idsCompletions(containingObject.component());
        }
    }

    DomItem context = containingObject;
    if (!context)
        context = file;
    // adds types and attributes
    res += reachableSymbols(context, ctx, typeCompletionType, methodCompletion);
    return res;
}
