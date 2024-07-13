The V4 Garbage Collector                                   {#v4-garbage-collector}
========================

ChangeLog
---------
- < 6.8: There was little documentation, and the gc was STW mark&sweep
- 6.8: The gc became incremental (with a stop-the-world sweep phase)
- 6.8: Sweep was made incremental, too


Glossary:
------------
- V4: The ECMAScript engine used in Qt (qtdeclarative)
- gc: abbreviation for garbage collector
- mutator: the actual user application  (in constrast to the collector)
- roots: A set of pointers from which the gc process starts
- fast roots: A subset of the roots which are not protected by a barrier
- write barrier: A set of instructions executed on each write
- stop the world: not concurrent
- concurrent gc: gc and mutator can run "at the same time"; this can either mean
    + incremental: collector and mutator run in the same thread,  but in certain time intervals the mutator is paused, a chunk of the collector is executed, and then the mutator resumes. Repeats until the gc cycle is finished
    + parallel: gc and mutator operations run in different threads
- precise: a gc is precise if for every value it can know whether it's a pointer to a heap object (a non-precise gc can't in general distinguish pointers from pointer-sized numbers)
- floating garbage: items that are not live, but nevertheless end up surviving the gc cycle
- generational: generational refers to dividing objects into different "generations" based on how many collection cycles they survived. This technique is used in garbage collection to improve performance by focusing on collecting the most recently created objects first.
- moving: A gc is moving if it can relocate objects in memory. Care must be taken to update pointers pointing to them.


Overview:
---------

Since Qt 6.8, V4 uses an incremental, precise mark-and-sweep gc algorithm. It is neither generational nor moving.

In the mark phase, each heap-item can be in one of three states:
1. unvisited ("white"): The gc has not seen this item at all
2. seen ("grey"): All grey items have been discovered by the gc, but items directly  reachable from them have (potentially) not been visited.
3. finished ("black"): Not only has the item been seen, but also all items directly reachable from it have been seen.

Items are black if they have their corresponding bit set in the black-object bitmap. They are grey if they are stored at least once in the MarkStack, a stack data structure. Items are white if they are neither grey nor black. Note that black items must never be pushed to the MarkStack (otherwise we could easily end up with endless cycles), but items already _on_ the MarkStack can be black:
If an item has been pushed multiple times before it has been popped, this can happen. It causes some additional work to revisit its fields, but that is safe, as after popping the item will be black, and thus we won't keep on repeatedly pushing the same item on the mark stack.

The roots consist of
- the engine-global objects (namely the internal classes for the JS globals)
- all strings (and symbols) stored in the identifier table and
- all actively linked compilation units.
- Moreover, the values on the JS stack are also treated as roots; more precisely as fast roots.
- Additionally, all persistent values (everything stored in a QJSValue as well as all bound functions of QQmlBindings) are added to the roots.
- Lastly, all QObjectWrapper of QObjects with C++ ownership, or which are rooted in or parented to a QObject with C++ ownership are added to the root set.

All roots are added to the MarkStack. Then, during mark phase, entries are:
1. popped from the markstack
2. All heap-objects reachable from them are added to the MarkStack (unless they are already black)

To avoid that values that were on the heap during the start of the gc cycle, then moved to the stack before they could be visited and consequently freed even though they are still live, the stack is rescanned before the sweep phase.

To avoid that unmarked heap-items are moved from one heap item (or the stack) to an already marked heap-item (and consequently end up hidden from the gc), we employ a Dijkstra style write barrier: Any item that becomes reachable from another heap-item is marked grey (unless already black).

While a gc cycle is ongoing, allocations are black, meaning every allocated object is considered to be live (until the next gc cycle is started).
This is currently required as compilation units linked to the engine while the gc is running are not protected by the write barrier or another mechanism. It also helps to reduce the amount of work to be done when rescanning the JS stack (as it helps to ensure that most items are already black at that point).


The gc state machine
--------------------

To facilitate incremental garbage collection, the gc algorithm is divided into the following stages:

1. markStart, the atomic initialization phase, in which the MarkStack is initialized, and a flag is set on the engine indicating that incremental gc is active
2. markGlobalObject, an atomic phase in which the global object, the engine's identifier table and the currently linked compilation units are marked
3. markJSStack, an atomic phase in which the JS stack is marked
4. initMarkPersistentValues: Atomic phase. If there are persistent values, some setup is done for the next phase.
5. markPersistentValues: An interruptible phase in which all persistent values are marked.
6. initMarkWeakValues: Atomic phase. If there are weak values, some setup is done for the next phase
7. markWeakValues: An interruptible phase which takes care of marking the QObjectWrappers
8. markDrain: An interrupible phase. While the MarkStack is not empty, the marking algorithm runs.
9.  markReady: An atomic phase which currently does nothing, but could be used for e.g. logging statistics
10. initCallDestroyObjects: An atomic phase, in which the stack is rescanned, the MarkStack is drained once more. This ensures that all live objects are really marked.
    Afterwards, the iteration over all the QObjectWrappers is prepared.
11. callDestroyObject: An interruptible phase, were we call destroyObject of all non-marked QObjectWrapper.
12. freeWeakMaps: An atomic phase in which we remove references to dead objects from live weak maps.
13. freeWeakSets: Same as the last phase, but for weak sets
14: handleQObjectWrappers: An atomic phase in which pending references to QObjectWrappers are cleared
15. multiple sweep phases: Atomic phases, in which do the actual sweeping to free up memory. Note that this will also call destroy on objects marked with `V4_NEEDS_DESTROY`. There is one phase for the various allocators (identifier table, block allocator, huge item allocator, IC allocator)
16. updateMetaData: Updates the black bitmaps, the usage statistics, and marks the gc cycle as done.
17. invalid, the "not-running" stage of the state machine.

To avoid constantly having to query the timer, even interruptible phases run for a fixed amount of steps before checking whether there's a timemout.

Most steps are straight-forward, only the persistent and weak value phases require some explanation as to why it's safe to interrupt the process: The important thing to note is that we never remove elements from the structure while we're undergoing gc, and that we only ever append at the end. So we will see any new values that might be added.

Persistent Values
-----------------
As shown in the diagram above, the handling of persistent values is interruptible (both for "real" persistent values, and also for weak vaules which also are stored in a `PersistentValueStorage` data structure.
This is done by storing the `PersistentValueStorage::Iterator` in the gc state machine. That in turn raises two questions: Is the iterator safe against invalidation? And do we actually keep track of newly added persistent values?

The latter part is easy to answer: Any newly added weak value is marked when we are in a gc cycle, so the marking part is handled. Sweeping only cares about unmarked values, so that's safe too.
To answer the question about iterator validity, we have to look at the `PersistentValueStorage` data structure. Conceptionally, it's a forward-list of `Page`s (arrays of `QV4::Value`). Pages are ref-counted, and only unliked from the list if the ref-count reaches zero. Moreover, iterators also increase the ref-count.
Therefore, as long as we iterate over the list, we don't risk having the pointer point to a deleted `Page` – even if all values in it have been freed. Freeing values is unproblematic for the gc – it will simply encounter `undefined` values, something it is already prepared to handle.
Pages are also kept in the list while they are not deleted, so iteration works as expected. The only adjustment we need to do is to disable an optimization: When searching for a Page with an empty slot, we have
to (potentially) travese the whole `PersistentValueStorage`. To avoid that, the first Page with empty slots is normally moved to the front of the list. However, that would mean that we could potentially skip over it during the marking phase. We sidestep that issue by simply disabling the optimization. This area could
easily be improved in the future by keeping track of the first page with free slots in a different way.

Custom marking
---------------

Some parts of the engine have to deviate from the general scheme described in the overview, as they don't integrate with the normal WriteBarrier. They are wrapped in the callback of the `QV4::WriteBarrier::markCustom` function, so that they can easily be found via "Find references".

1. `QJSValue::encode`. QJSValues act as roots, and don't act as normal heap-items. When the current value of a QJSValue is overwritten with another heap-item, we also mark the new object. That aligns nicely with the gc barrier.
2. The same applies to `PersistentValue::set`.
3. The identifier table is another root; if a new string is inserted there during gc, it is (conservatively) marked black.
4. PropertyKeys should for all intents and purposes use a write barrier (and have a deleted operator=). But them being an ad-hoc union type of numbers, Strings and Symbols, which has the additional requirements of having to be trivial, it turned out to be easier to simply mark them in `SharedInternalClassDataPrivate<PropertyKey>::set` (for PropertyKeys that had already been allocated), and on the fact that we allocate black for newly created PropertyKeys.
5. `QV4::Heap::InternalClass` also requires special handling, as it uses plain members to Heap objects, notably to the prototype and to the parent internal class. As the class is somewhat special in any case (due to the usage of `SharedInternalClassData` and especially due to the usage of `SharedInternalClassData<PropertyKey>`, see notes on PropertyKey above), we use some bespoke sections for now. This could probably be cleaned up.

Motivation for using a Dijkstra style barrier:
----------------------------------------------
- Deletion barriers are hard to support with the current PropertyKey design
- Steele style barriers cause more work (have to revisit more objects), and as long as we have black allocations it doesn't make much sense to optimize for a minimal amount  of floating garbage.

Sweep Phase and finalizers:
---------------------------
A story for another day

Allocator design:
-----------------
Your explanation is in another castle.

