// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qv4stacklimits_p.h>
#include <private/qobject_p.h>
#include <private/qthread_p.h>

#include <QtCore/qfile.h>

#if defined(Q_OS_UNIX)
#  include <pthread.h>
#endif

#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h>
#elif defined(Q_OS_FREEBSD_KERNEL) || defined(Q_OS_OPENBSD)
#  include <pthread_np.h>
#elif defined(Q_OS_LINUX)
#  include <unistd.h>
#  include <sys/resource.h> // for getrlimit()
#  include <sys/syscall.h>  // for SYS_gettid
#  if defined(__GLIBC__) && QT_CONFIG(dlopen)
#    include <dlfcn.h>
#  endif
#elif defined(Q_OS_DARWIN)
#  include <sys/resource.h> // for getrlimit()
#elif defined(Q_OS_QNX)
#  include <devctl.h>
#  include <sys/procfs.h>
#  include <sys/types.h>
#  include <unistd.h>
#elif defined(Q_OS_INTEGRITY)
#  include <INTEGRITY.h>
#elif defined(Q_OS_WASM)
#  include <emscripten/stack.h>
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

enum StackDefaults : qsizetype {
    // Default safety margin at the end of the usable stack.
    // Since we don't check the stack on every instruction, we might overrun our soft limit.
    DefaultSafetyMargin = 128 * 1024,
#if defined(Q_OS_IOS)
    PlatformStackSize = 1024 * 1024,
    PlatformSafetyMargin = DefaultSafetyMargin,
#elif defined(Q_OS_MACOS)
    PlatformStackSize = 8 * 1024 * 1024,
    PlatformSafetyMargin = DefaultSafetyMargin,
#elif defined(Q_OS_ANDROID)
    // Android appears to have 1MB stacks.
    PlatformStackSize = 1024 * 1024,
    PlatformSafetyMargin = DefaultSafetyMargin,
#elif defined(Q_OS_LINUX)
    // On linux, we assume 8MB stacks if rlimit doesn't work.
    PlatformStackSize = 8 * 1024 * 1024,
    PlatformSafetyMargin = DefaultSafetyMargin,
#elif defined(Q_OS_QNX)
    // QNX's stack is only 512k by default
    PlatformStackSize = 512 * 1024,
    PlatformSafetyMargin = DefaultSafetyMargin,
#else
    // We try to claim 512k if we don't know anything else.
    PlatformStackSize = 512 * 1024,
    PlatformSafetyMargin = DefaultSafetyMargin,
#endif
};

// We may not be able to take the negative of the type
// used to represent stack size, but we can always add
// or subtract it to/from a quint8 pointer.

template<typename Size>
static void *incrementStackPointer(void *base, Size amount)
{
#if Q_STACK_GROWTH_DIRECTION > 0
    return static_cast<quint8 *>(base) + amount;
#else
    return static_cast<quint8 *>(base) - amount;
#endif
}

template<typename Size>
static void *decrementStackPointer(void *base, Size amount)
{
#if Q_STACK_GROWTH_DIRECTION > 0
    return static_cast<quint8 *>(base) - amount;
#else
    return static_cast<quint8 *>(base) + amount;
#endif
}

static StackProperties createStackProperties(void *base, qsizetype size = PlatformStackSize)
{
    return StackProperties {
        base,
        incrementStackPointer(base, size - PlatformSafetyMargin),
        incrementStackPointer(base, size),
    };
}

#if defined(Q_OS_DARWIN) || defined(Q_OS_LINUX)

// On linux and darwin, on the main thread, the pthread functions
// may not return the true stack size since the main thread stack
// may grow. Use rlimit instead. rlimit does not work for secondary
// threads, though. If getrlimit fails, we assume the platform
// stack size.
static qsizetype getMainStackSizeFromRlimit()
{
    rlimit limit;
    return (getrlimit(RLIMIT_STACK, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY)
            ? qsizetype(limit.rlim_cur)
            : qsizetype(PlatformStackSize);
}
#endif

#if defined(Q_OS_INTEGRITY)

StackProperties stackProperties()
{
    Address stackLow, stackHigh;
    CheckSuccess(GetTaskStackLimits(CurrentTask(), &stackLow, &stackHigh));
#  if Q_STACK_GROWTH_DIRECTION < 0
    return createStackProperties(reinterpret_cast<void *>(stackHigh), stackHigh - stackLow);
#  else
    return createStackProperties(reinterpret_cast<void *>(stackLow), stackHigh - stackLow);
#  endif
}

#elif defined(Q_OS_DARWIN)

StackProperties stackProperties()
{
    pthread_t thread = pthread_self();
    return createStackProperties(
                pthread_get_stackaddr_np(thread),
                pthread_main_np()
                    ? getMainStackSizeFromRlimit()
                    : qsizetype(pthread_get_stacksize_np(thread)));
}

#elif defined(Q_OS_WIN)

static_assert(Q_STACK_GROWTH_DIRECTION < 0);
StackProperties stackProperties()
{
    // Get the stack base.
#  ifdef _WIN64
    PNT_TIB64 pTib = reinterpret_cast<PNT_TIB64>(NtCurrentTeb());
#  else
    PNT_TIB pTib = reinterpret_cast<PNT_TIB>(NtCurrentTeb());
#  endif
    quint8 *stackBase = reinterpret_cast<quint8 *>(pTib->StackBase);

    // Get the stack limit. tib->StackLimit is the size of the
    // currently mapped stack. The address space is larger.
    MEMORY_BASIC_INFORMATION mbi = {};
    if (!VirtualQuery(&mbi, &mbi, sizeof(mbi)))
        qFatal("Could not retrieve memory information for stack.");

    quint8 *stackLimit = reinterpret_cast<quint8 *>(mbi.AllocationBase);
    return createStackProperties(stackBase, qsizetype(stackBase - stackLimit));
}

#elif defined(Q_OS_OPENBSD)

StackProperties stackProperties()
{
    // From the OpenBSD docs:
    //
    //   The pthread_stackseg_np() function returns information about the given thread's stack.
    //   A stack_t is the same as a struct sigaltstack (see sigaltstack(2)) except the ss_sp
    //   variable points to the top of the stack instead of the base.
    //
    // Since the example in the sigaltstack(2) documentation shows ss_sp being assigned the result
    // of a malloc() call, we can assume that "top of the stack" means "the highest address", not
    // the logical top of the stack.

    stack_t ss;
    rc = pthread_stackseg_np(pthread_self, &ss);
#if Q_STACK_GROWTH_DIRECTION < 0
    return createStackProperties(ss.ss_sp);
#else
    return createStackProperties(decrementStackPointer(ss.ss_sp, ss.ss_size));
#endif
}

#elif defined(Q_OS_QNX)

StackProperties stackProperties()
{
    const auto tid = pthread_self();
    procfs_status status;
    status.tid = tid;

    const int fd = open("/proc/self/ctl", O_RDONLY);
    if (fd == -1)
        qFatal("Could not open /proc/self/ctl");
    const auto guard = qScopeGuard([fd]() { close(fd); });

    if (devctl(fd, DCMD_PROC_TIDSTATUS, &status, sizeof(status), 0) != EOK)
        qFatal("Could not query thread status for current thread");

    if (status.tid != tid)
        qFatal("Thread status query returned garbage");

#if Q_STACK_GROWTH_DIRECTION < 0
    return createStackProperties(
                decrementStackPointer(reinterpret_cast<void *>(status.stkbase), status.stksize),
                status.stksize);
#else
    return createStackProperties(reinterpret_cast<void *>(status.stkbase), status.stksize);
#endif
}

#elif defined(Q_OS_WASM)

StackProperties stackProperties()
{
    const uintptr_t base = emscripten_stack_get_base();
    const uintptr_t end = emscripten_stack_get_end();
    const size_t size = base - end;
    return createStackProperties(reinterpret_cast<void *>(base), size);
}

#else

StackProperties stackPropertiesGeneric(qsizetype stackSize = 0)
{
    // If stackSize is given, do not trust the stack size returned by pthread_attr_getstack

    pthread_t thread = pthread_self();
    pthread_attr_t sattr;
#  if defined(PTHREAD_NP_H) || defined(_PTHREAD_NP_H_) || defined(Q_OS_NETBSD)
    pthread_attr_init(&sattr);
    pthread_attr_get_np(thread, &sattr);
#  else
    pthread_getattr_np(thread, &sattr);
#  endif

    // pthread_attr_getstack returns the address of the memory region, which is the physical
    // base of the stack, not the logical one.
    void *stackBase;
    size_t regionSize;
    int rc = pthread_attr_getstack(&sattr, &stackBase, &regionSize);
    pthread_attr_destroy(&sattr);

    if (rc)
        qFatal("Cannot find stack base");

#  if Q_STACK_GROWTH_DIRECTION < 0
    stackBase = decrementStackPointer(stackBase, regionSize);
#  endif

    return createStackProperties(stackBase, stackSize ? stackSize : regionSize);
}

#if defined(Q_OS_LINUX)

static void *stackBaseFromLibc()
{
#if defined(__GLIBC__) && QT_CONFIG(dlopen)
    void **libcStackEnd = static_cast<void **>(dlsym(RTLD_DEFAULT, "__libc_stack_end"));
    if (!libcStackEnd)
        return nullptr;
    if (void *stackBase = *libcStackEnd)
        return stackBase;
#endif
    return nullptr;
}

struct StackSegment {
    quintptr base;
    quintptr limit;
};

static StackSegment stackSegmentFromProc()
{
    QFile maps(QStringLiteral("/proc/self/maps"));
    if (!maps.open(QIODevice::ReadOnly))
        return {0, 0};

    const quintptr stackAddr = reinterpret_cast<quintptr>(&maps);

    char buffer[1024];
    while (true) {
        const qint64 length = maps.readLine(buffer, 1024);
        if (length <= 0)
            break;

        const QByteArrayView line(buffer, length);
        bool ok = false;

        const qsizetype boundary = line.indexOf('-');
        if (boundary < 0)
            continue;

        const quintptr base = line.sliced(0, boundary).toULongLong(&ok, 16);
        if (!ok || base > stackAddr)
            continue;

        const qsizetype end = line.indexOf(' ', boundary);
        if (end < 0)
            continue;

        const quintptr limit = line.sliced(boundary + 1, end - boundary - 1).toULongLong(&ok, 16);
        if (!ok || limit <= stackAddr)
            continue;

        return {base, limit};
    }

    return {0, 0};
}

StackProperties stackProperties()
{
    if (getpid() != static_cast<pid_t>(syscall(SYS_gettid)))
        return stackPropertiesGeneric();

    // On linux (including android), the pthread functions are expensive
    // and unreliable on the main thread.

    // First get the stack size from rlimit
    const qsizetype stackSize = getMainStackSizeFromRlimit();

    // If we have glibc and libdl, we can query a special symbol in glibc to find the base.
    // That is extremely cheap, compared to all other options.
    if (stackSize) {
        if (void *base = stackBaseFromLibc())
            return createStackProperties(base, stackSize);
    }

    // Try to read the stack segment from /proc/self/maps if possible.
    const StackSegment segment = stackSegmentFromProc();
    if (segment.base) {
#    if Q_STACK_GROWTH_DIRECTION > 0
        void *stackBase = reinterpret_cast<void *>(segment.base);
#    else
        void *stackBase = reinterpret_cast<void *>(segment.limit);
#    endif
        return createStackProperties(
                    stackBase, stackSize ? stackSize : segment.limit - segment.base);
    }

    // If we can't read /proc/self/maps, use the pthread functions after all, but
    // override the stackSize. The main thread can grow its stack, and the pthread
    // functions typically return the currently allocated stack size.
    return stackPropertiesGeneric(stackSize);
}

#else // Q_OS_LINUX

StackProperties stackProperties() { return stackPropertiesGeneric(); }

#endif // Q_OS_LINUX
#endif

} // namespace QV4

QT_END_NAMESPACE
