// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
// TODO: verifyj critical part should be in YARR

#include "qv4regexp_p.h"
#include "qv4engine_p.h"
#include "qv4scopedvalue_p.h"
#include <private/qv4mm_p.h>
#include <runtime/VM.h>

using namespace QV4;

#if ENABLE(YARR_JIT)
static constexpr qsizetype LongStringJitThreshold = 1024;
static constexpr int LongStringJitBoost = 3;
#endif

static JSC::RegExpFlags jscFlags(quint8 flags)
{
    JSC::RegExpFlags jscFlags = JSC::NoFlags;
    if (flags & CompiledData::RegExp::RegExp_Global)
        jscFlags = static_cast<JSC::RegExpFlags>(jscFlags | JSC::FlagGlobal);
    if (flags & CompiledData::RegExp::RegExp_IgnoreCase)
        jscFlags = static_cast<JSC::RegExpFlags>(jscFlags | JSC::FlagIgnoreCase);
    if (flags & CompiledData::RegExp::RegExp_Multiline)
        jscFlags = static_cast<JSC::RegExpFlags>(jscFlags | JSC::FlagMultiline);
    if (flags & CompiledData::RegExp::RegExp_Unicode)
        jscFlags = static_cast<JSC::RegExpFlags>(jscFlags | JSC::FlagUnicode);
    if (flags & CompiledData::RegExp::RegExp_Sticky)
        jscFlags = static_cast<JSC::RegExpFlags>(jscFlags | JSC::FlagSticky);
    return jscFlags;
}

RegExpCache::~RegExpCache()
{
    for (RegExpCache::Iterator it = begin(), e = end(); it != e; ++it) {
        if (RegExp *re = it.value().as<RegExp>())
            re->d()->cache = nullptr;
    }
}

DEFINE_MANAGED_VTABLE(RegExp);

uint RegExp::match(const QString &string, int start, uint *matchOffsets)
{
    // Note that we rely on the side-effect of checkStackLimits updating
    // cppStackLimit (which might be necessary after a thread move):
    // hasCppStackOverflow's recheck will trigger after a move (if it
    // didn't, we'd had overlapping stack regions, which can't be).
    if (!isValid() || internalClass()->engine->checkStackLimits())
        return JSC::Yarr::offsetNoMatch;

#if ENABLE(YARR_JIT)
    auto *priv = d();

    auto regenerateByteCode = [](Heap::RegExp *regexp) {
        const void* stackLimit = regexp->internalClass->engine->cppStackLimit;
        JSC::Yarr::ErrorCode error = JSC::Yarr::ErrorCode::NoError;
        JSC::Yarr::YarrPattern yarrPattern(WTF::String(*regexp->pattern), jscFlags(regexp->flags),
                                           error, const_cast<void *>(stackLimit));

        // As we successfully parsed the pattern before, we should still be able to.
        Q_ASSERT(error == JSC::Yarr::ErrorCode::NoError);

        regexp->byteCode = JSC::Yarr::byteCompile(
                                   yarrPattern,
                                   regexp->internalClass->engine->bumperPointerAllocator).release();
    };

    auto removeJitCode = [](Heap::RegExp *regexp) {
        delete regexp->jitCode;
        regexp->jitCode = nullptr;
        regexp->jitFailed = true;
    };

    auto removeByteCode = [](Heap::RegExp *regexp) {
        delete regexp->byteCode;
        regexp->byteCode = nullptr;
    };

    if (!priv->jitCode) {

        // Long strings count as more calls. We want the JIT to run earlier.
        const bool longString = string.length() > LongStringJitThreshold;
        if (longString)
            priv->interpreterCallCount += LongStringJitBoost;

        if (priv->internalClass->engine->canJIT(priv)) {
            removeByteCode(priv);

            JSC::Yarr::ErrorCode error = JSC::Yarr::ErrorCode::NoError;
            const void* stackLimit = internalClass()->engine->cppStackLimit;
            JSC::Yarr::YarrPattern yarrPattern(
                    WTF::String(*priv->pattern), jscFlags(priv->flags), error, const_cast<void *>(stackLimit));
            if (!yarrPattern.m_containsBackreferences) {
                priv->jitCode = new JSC::Yarr::YarrCodeBlock;
                JSC::VM *vm = static_cast<JSC::VM *>(priv->internalClass->engine);
                JSC::Yarr::jitCompile(yarrPattern, JSC::Yarr::Char16, vm, *priv->jitCode);
            }

            if (!priv->hasValidJITCode()) {
                removeJitCode(priv);
                regenerateByteCode(priv);
            }
        } else if (!longString) {
            // Short strings do the regular post-increment to honor
            // QV4_JIT_CALL_THRESHOLD.
            ++priv->interpreterCallCount;
        }
    }
#endif

    WTF::String s(string);

#if ENABLE(YARR_JIT)
    if (priv->hasValidJITCode()) {
        static const uint offsetJITFail = std::numeric_limits<unsigned>::max() - 1;
        uint ret = JSC::Yarr::offsetNoMatch;
#if ENABLE(YARR_JIT_ALL_PARENS_EXPRESSIONS)
        char buffer[8192];
        ret = uint(priv->jitCode->execute(s.characters16(), start, s.size(),
                                          (int*)matchOffsets, buffer, 8192).start);
#else
        ret = uint(priv->jitCode->execute(s.characters16(), start, s.length(),
                                          (int*)matchOffsets).start);
#endif
        if (ret != offsetJITFail)
            return ret;

        removeJitCode(priv);
        // JIT failed. We need byteCode to run the interpreter.
        Q_ASSERT(!priv->byteCode);
        regenerateByteCode(priv);
    }
#endif // ENABLE(YARR_JIT)

    return JSC::Yarr::interpret(byteCode(), s.characters16(), string.size(), start, matchOffsets);
}

QString RegExp::getSubstitution(const QString &matched, const QString &str, int position, const Value *captures, int nCaptures, const QString &replacement)
{
    QString result;

    int matchedLength = matched.size();
    Q_ASSERT(position >= 0 && position <= str.size());
    int tailPos = position + matchedLength;
    int seenDollar = -1;
    for (int i = 0; i < replacement.size(); ++i) {
        QChar ch = replacement.at(i);
        if (seenDollar >= 0) {
            if (ch.unicode() == '$') {
                result += QLatin1Char('$');
            } else if (ch.unicode() == '&') {
                result += matched;
            } else if (ch.unicode() == '`') {
                result += str.left(position);
            } else if (ch.unicode() == '\'') {
                result += str.mid(tailPos);
            } else if (ch.unicode() >= '0' && ch.unicode() <= '9') {
                int n = ch.unicode() - '0';
                if (i + 1 < replacement.size()) {
                    ch = replacement.at(i + 1);
                    if (ch.unicode() >= '0' && ch.unicode() <= '9') {
                        n = n*10 + (ch.unicode() - '0');
                        ++i;
                    }
                }
                if (n > 0 && n <= nCaptures) {
                    String *s = captures[n].stringValue();
                    if (s)
                        result += s->toQString();
                } else {
                    for (int j = seenDollar; j <= i; ++j)
                        result += replacement.at(j);
                }
            } else {
                result += QLatin1Char('$');
                result += ch;
            }
            seenDollar = -1;
        } else {
            if (ch == QLatin1Char('$')) {
                seenDollar = i;
                continue;
            }
            result += ch;
        }
    }
    if (seenDollar >= 0)
        result += QLatin1Char('$');
    return result;
}

Heap::RegExp *RegExp::create(
        ExecutionEngine *engine, const QString &pattern, CompiledData::RegExp::Flags flags)
{
    RegExpCacheKey key(pattern, flags);

    RegExpCache *cache = engine->regExpCache;
    if (!cache)
        cache = engine->regExpCache = new RegExpCache;

    QV4::WeakValue &cachedValue = (*cache)[key];
    if (QV4::RegExp *result = cachedValue.as<RegExp>())
        return result->d();

    Scope scope(engine);
    Scoped<RegExp> result(scope, engine->memoryManager->alloc<RegExp>(engine, pattern, flags));

    result->d()->cache = cache;
    cachedValue.set(engine, result);

    return result->d();
}

void Heap::RegExp::init(ExecutionEngine *engine, const QString &pattern, uint flags)
{
    Base::init();
    this->pattern = new QString(pattern);
    this->flags = flags;

    JSC::Yarr::ErrorCode error = JSC::Yarr::ErrorCode::NoError;
    // compare the comment in RegExp::match
    if (engine->checkStackLimits())
        return;
    const void* stackLimit = internalClass->engine->cppStackLimit;
    JSC::Yarr::YarrPattern yarrPattern(WTF::String(pattern), jscFlags(flags), error, const_cast<void *>(stackLimit));
    if (error != JSC::Yarr::ErrorCode::NoError)
        return;
    subPatternCount = yarrPattern.m_numSubpatterns;
    Q_UNUSED(engine);
    byteCode = JSC::Yarr::byteCompile(yarrPattern, internalClass->engine->bumperPointerAllocator).release();
    if (byteCode)
        valid = true;
}

void Heap::RegExp::destroy()
{
    if (cache) {
        RegExpCacheKey key(this);
        cache->remove(key);
    }
#if ENABLE(YARR_JIT)
    delete jitCode;
#endif
    delete byteCode;
    delete pattern;
    Base::destroy();
}
