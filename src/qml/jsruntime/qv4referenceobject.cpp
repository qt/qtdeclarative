// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qv4referenceobject_p.h>

QT_BEGIN_NAMESPACE

DEFINE_OBJECT_VTABLE(QV4::ReferenceObject);

/*!
  \class QV4::ReferenceObject
  \brief An object that keeps track of the provenance of its owned
  value, allowing to reflect mutations on the original instance.

  \internal

  \section1 Copied Types and Mutations

  In QML, certain types are conceptually passed by value.
  Instances of those types are always copied when they are accessed
  or passed around.

  Let those be "Copied Type"s.

  For example, suppose that \c{foo} is an instance of a Copied Type
  that has a member \c{bar} that has some value \c{X}.

  Consider the following example:

  \qml
      import QtQuick

      Item {
          Component.onCompleted: {
             foo.bar = Y
             console.log(foo.bar)
          }
      }
  \endqml

  Where \c{Y} is some value that can inhabit \c{foo.bar} and whose
  stringified representation is distinguishable from \c{X}.

  One might expect that a stringified representation of \c{Y} should be logged.
  Nonetheless, as \c{foo} is a Copied Type, accessing it creates a copy.
  The access to the \c{bar} member and its further mutation is
  performed on the copy that was created, and thus is not retained by
  the object that \c{foo} refers to.

  If \c{copy} is an operation that performs a deep-copy of an object
  and returns it, the above snippet can be considered implicitly
  equivalent to the following:

  \qml
      import QtQuick

      Item {
          Component.onCompleted: {
             copy(foo).bar = Y
             console.log(copy(foo).bar)
          }
      }
  \endqml

  This can generally be surprising as it stands in contrast to the
  effect that the above assignment would have if \c{foo} wasn't a
  Copied Type. Similarly, it stands in contrast to what one could
  expect from the outcome of the same assignment in a Javascript
  environment, where the mutation might be expected to generally be
  visible in later steps no matter the type of \c{foo}.

  A ReferenceObject can be used to avoid this inconsistency by
  wrapping a value and providing a "write-back" mechanism that can
  reflect mutations back on the original value.

  Furthermore, a ReferenceObject can be used to load the data from
  the original value to ensure that the two values remain in sync, as
  the value might have been mutated while the copy is still alive,
  conceptually allowing for an "inverted write-back".

  \section1 Setting Up a ReferenceObject

  ReferenceObject is intended to be extended by inheritance.

  An object that is used to wrap a value that is copied around but
  has a provenance that requires a write-back, can inherit from
  ReferenceObject to plug into the write-back behavior.

  The heap part of the object should subclass
  QV4::Heap::ReferenceObject while the object part should subclass
  QV4::ReferenceObject.

  When initializing the heap part of the subclass,
  QV4::Heap::ReferenceObject::init should be called to set up the
  write-back mechanism.

  The write-back mechanism stores a reference to an object and,
  potentially, a property index to write-back at.

  Furthermore, a series of flags can be used to condition the
  behavior of the write-back.

  For example:

  \code
     void QV4::Heap::Foo::init(Heap::Object *object) {
         ReferenceObject::init(object, 1, ReferenceObject::Flag::CanWriteBack);
         // Some further initialization code
         ...
     }
  \endcode

  The snippet is an example of some sub-class \c{Foo} of
  ReferenceObject setting up for a write-back to the property at index
  1 of some object \c{object}.

  Generally, a sub-class of ReferenceObject will be used to wrap one
  or more Copied Types and provide a certain behavior.

  In certain situations there is no need to setup a write-back.
  For example, we might have certain cases where there is no original
  value to be wrapped while still in need of providing an object of
  the sub-classing type.

  One example of such a behavior is that of returning an instance from
  a C++ method to QML.

  When that is the case, the following initialization can be provided:

  \code
     void QV4::Heap::Foo::init(Heap::Object *object) {
         ReferenceObject::init(nullptr, -1, NoFlag);
         // Some further initialization code
         ...
     }
  \endcode

  \section2 Intitialization and the IsDirty flag

  In certain cases, we try to avoid read-backs when we know that we
  have the latest data available already, see \l{Limiting reads on a
  QObject property}.

  Certain implementations of ReferenceObject, might want to lazily load
  the data on the first read, rather than on the initialization of the
  reference. One such example would be `QQmlValueTypeWrapper`.

  When that is the case, the IsDirty flag should be passed at
  initialiazation time.
  For example:

  \code
     void QV4::Heap::Foo::init(Heap::Object *object) {
         ReferenceObject::init(object, 1, ReferenceObject::Flag::CanWriteBack | ReferenceObject::Flag::IsDirty);
         // Some further initialization code
         ...
     }
  \endcode

  If the flag is not passed there the first read might be elided,
  leaving the object in an incorrect state.

  \section1 Providing the Required Infrastructure for a Default Write-back

  Generally, to use the base implementation of write and read backs,
  as provided by QV4::ReferenceObject::readReference and
  QV4::ReferenceObject::writeBack, a sub-class should provide the
  following interface:

  \code
      void *storagePointer();
      const void *storagePointer();


       QVariant toVariant() const;
       bool setVariant(const QVariant &variant);
  \endcode

  The two overloads of \c{storagePointer} should provide access to
  the internal backing storage of the ReferenceObject.

  Generally, a ReferenceObject will be wrapping some instance of a C++
  type, owning a copy of the original instance used as a storage for
  writes and reads.
  An implementation of \c{storagePointer} should give access to this
  backing storage, which is used to know what value to write-back and
  where to write when reading back from the original value.

  For example, a Sequence type that wraps a QVariantList will own its
  own instance of a QVariantList. The implementation for
  \c{storagePointer} should give access to that owned instance.

  \c{toVariant} should provide a QVariant wrapped representation of
  the internal storage that the ReferenceObject uses.
  This is used during the write-back of a ReferenceObject whose
  original value was a QVariant backed instance.

  Do remember that instances of a ReferenceObject that are backing
  QVariant values should further pass the
  QV4::Heap::ReferenceObject::Flag::IsVariant flag at initialization
  time.

  On the opposite side, \c{setVariant} should switch the value that
  the ReferenceObject stores with the provided variant.
  This is used when a QVariant backed ReferenceObject performs a read
  of its original value, to allow for synchronization.

  It is still possible to use ReferenceObject without necessarily
  providing the above interface.
  QV4::DateObject is an example of a ReferenceObject sub-class that
  provides its own writeBack implementation and doesn't abide by the
  above.

  \section1 Performing a Write-back

  With a sub-class of ReferenceObject that was set-up as above, a
  write-back can be performed by calling
  QV4::ReferenceObject::writeBack.

  For example, some ReferenceObject subclass \c{Foo} might be
  backing, say, some map-like object.

  Internally, insertion of an element might pass by some method, say
  \c{insertElement}. An implementation might, for example, look like
  the following:

  \code
    bool Foo::insertElement(const QString& key, const Value& value) {
        // Insert the element if possible
        ...
        QV4::ReferenceObject::writeBack(d());
        ...
        // Some further handling
    }
  \endcode

  The call to writeBack will ensure that the newly inserted element
  will be reflected on the original value where the object originates
  from.

  Here \c{d()}, with \c{Foo} being a sub-class of ReferenceObject and
  thus of Object, is the heap part of \c{Foo} that should provide the
  interface specificed above and owns the actual storage of the stored
  value.

  \section1 Synchronizing with the Original Value

  QV4::ReferenceObject::readReference provides a way to obtain the
  current state of the value that the ReferenceObject refers to.

  When this read is performed, the obtained value will be stored back
  into the backing storage for the ReferenceObject.

  This allows the ReferenceObject to lazily load the latest data on
  demand and correctly reflect the original value.

  For example, say that \c{Foo} is a sub-class of ReferenceObject that
  is used to back array-like values.

  \c{Foo} should provide the usual \c{virtualGetLength} method to
  support the \c{length} property that we expect an array to have.

  In a possible implementation of \c{virtualGetLength}, \c{Foo} should
  ensure that readReference is called before providing a value for the
  length, to ensure that the latest data is available.

  For example:

  \code
    qint64 Foo::virtualGetLength(const Managed *m)
    {
        const Foo *foo = static_cast<const Foo *>(m);
        foo->readReference(d());
        // Return the length from the owned storage
    }
  \endcode

  \section2 Limiting reads on a QObject property

  In most cases we cannot know whether the original data was modified between
  read accesses. This generally forces a read to be performed each time we
  require the latest data, even if we might have it already.

  This can have surprising results, as certain procedure might require reading
  the data multiple times to be performed, which sometimes can be very
  expensive.

  When the original data comes from the property of a \c{QObject}, and the
  property has a \tt{NOTIFY} signal or is \tt{BINDABLE}, we can subscribe to the
  signal to know when the data is actually modified outside our control, so that
  we need to fetch it again.

  A ReferenceObject can take advantage of this to reduce the number of reads
  that are required when dealing with a \c{QObject}'s property provening data.

  When a property has a \tt{NOTIFY} signal and is a \tt{BINDABLE} at
  the same time, we only need to use one such connection.
  Currently, the \tt{BINDABLE} subscription will take predecedence.

  ReferenceObjects that are part of a \l{Reference object chains}{chain}, will
  traverse the chain up until a QObject holding root is found, and connect based
  on that object.
  As read/write backs in a chain are always propagated up the chain, this allow
  ReferenceObjects that are not directly parented to relevant element to still
  avoid unnecesary reads.

  For example, the property of a value type exposed by a Q_GADGET, cannot have a
  \tt{NOTIFY} signal.
  Nonetheless, if a change were to occur to the parent value type or the
  property itself, that change would be propagated up the chain, possibly
  triggering a \tt{NOTIFY} signal that is part of the chain.
  Thus, by connecting to that upper \tt{NOTIFY} signal, we can still reliably know
  if a change was performed on the property itself and thus reduce the
  number of reads.

  As changes in the chain that do not really invalidate the data of that
  property will still trigger that same \tt{NOTIFY} signal, sometimes we will
  perform a read that is unnecessary due to granularity at which we are working.
  This is the case, returning to the example above, when a different
  property of that same value type will be changed.

  This should still be a win, as we still expect to cut off multiple reads that
  would be performed without the optimization.

  The default implementation for QV4::ReferenceObject::readReference will take
  care of performing this optimization already.

  Derived objects that provide their own readReference implementation can plug
  into QV4::Heap::ReferenceObject::isDirty, QV4::Heap::ReferenceObject::setDirty
  and QV4::Heap::ReferenceObject::isConnected to provide the same optimization.

  A ReferenceObject uses a "dirty" flag to track whether the data should be read
  again.
  If the ReferenceObject refers to a \c{QObject}'s property that has a
  \tt{NOTIFY} signal or is \tt{BINDABLE}, it will set the flag each time the
  \tt{NOTIFY} signal is emitted or the \tt{BINDABLE} is changed.

  isDirty returns whether the flag is set and, thus, a readReference
  implementation should avoid performing the read itself when the method
  returns true.

  After a read is performed, the "dirty" flag should be set again if the read
  was unsuccessful.
  The flag can be modified by usages of `setDirty`.

  Generally, this only applies to instances of ReferenceObject that provene from
  a \c{QObject}'s property that has a notify signal, as that is the case that
  allows us to know when a read is required.

  In all other cases, a ReferenceObject should always be "dirty" and perform a
  read, as it cannot know if the data was modified since its last read.
  This case will initially be managed by the base constructor for
  ReferenceObject, nonetheless derived objects with a custom readReference
  implementation need to take it into account when setting the "dirty" flag
  after a read.

  isConnected can be used to discern between the two cases, as it will only
  return true when the ReferenceObject is connected to a NOTIFY signal that can
  modify the "dirty" flag.
  When isConnected is false, a read implementation should always keep the
  ReferenceObject in a permanent "dirty" state, to ensure that the correct data
  is fetched when required.

  \section1 Limiting Write-backs Based on Source Location

  \note We generally consider location-aware write-backs to be a
  mistake and expect to generally avoid further uses of them.
  Due to backward compatibility promises they cannot be universally
  enforced, possibly creating discrepancies in certain behaviors.
  If at some point possible, the feature might be backtracked on and
  removed, albeit this has shown to be difficult due to certain
  existing cross-dependencies.

  Consider the following code:

  \qml
      import QtQuick

      Text {
          Component.onCompleted: {
              var f = font // font is a value type and f is a value type reference
              f.bold = true;
              otherText.font = f
          }
      }
  \endqml

  Remembering that \c{font} is a Copied Type in QtQuick, \c{f} will be a
  Copied Type reference, internally backed by a ReferenceObject.
  Changing the \c{bold} property of \c{f} would internally perform a
  write-back which will reflect on the original \c{font} property of
  the \c{Text} component, as we would expect from the definition we
  gave above.

  Nonetheless, we could generally expect that the intention behind the
  code wasn't to update the original \c{font} property, but to pass a
  slightly modified version of its value to \c{otherText}.

  To avoid introducing this kind of possibly unintended behavior while
  still supporting a solution to the original problem, ReferenceObject
  allows limiting the availability of write-backs to a specific source
  location.

  For example, by limiting the availability of write-backs to the
  single statement where a Copied Type reference is created, we can
  address the most common cases of the original issue while avoiding
  the unintuitive behavior of the above.

  To enable source location enforcement,
  QV4::Heap::ReferenceObject::Flag::EnforcesLocation should be set
  when the ReferenceObject is initialized.

  A reference location should be set by calling
  QV4::Heap::ReferenceObject::setLocation.
  For example, during initialization, one might be set to limit
  write-backs to the currently processed statement, where the
  ReferenceObject was created, as follows:

  \code
  void Heap::Sequence::Foo::init(..., Heap::ReferenceObject::Flags flags) {
      ...
      ReferenceObject::init(..., flags & ReferenceObject::Flag::EnforcesLocation);
      ...
      if (CppStackFrame *frame = internalClass->engine->currentStackFrame)
          setLocation(frame->v4Function, frame->statementNumber());
      ...
  }
  \endcode

  Do note that calls to QV4::ReferenceObject::writeBack and
  QV4::ReferenceObject::readReference do not directly take into
  account location enforcement.

  This should generally be handled by the sub-class.
  QV4::Heap::ReferenceObject::isAttachedToProperty can be used to
  recognize whether the reference is still suitable for write-backs in
  a location-enforcement-aware way.

  \section1 Reference object chains

  ReferenceObject can be nested.

  For example, consider:

  \code
  a.b.c
  \endcode

  Where \c{a} is some object exposed to QML, \c{b} is a property of \c{a} and \c{c} is a property of \c{b}.

  Based on what each of \c{a}, \c{b} and \c{c} is, multiple ReferenceObject
  instances, parented to one another, might be introduced.

  For example, if \c{a} is a Q_OBJECT, \c{b} is a value type and \c{c} is a type
  that will be converted to a QV4::Sequence, \c{a} will be wrapped by a
  QObjectWrapper, \c{b} will be wrapped by a QQmlValueTypeWrapper which is parented
  to the QObjectWrapper wrapping \c{a} and \c{c} will be a Sequence that is
  parented to the QQmlValueTypeWrapper wrapping \c{b}.

  This parenting chain is used to enable recursive read/write backs, ensuring
  that a read/write back travels up the chain as required so that the latest
  data is available on every relevant element.

  At certain points in the chain, it is possible that a non-reference object is
  introduced.

  For example, this is always the case when a Q_OBJECT is retrieved, as it will
  be wrapped in a QObjectWrapper which is not a reference object.

  This breaks the chain of parenting and introduces the start of a new chain.
  As a QObjectWrapper directly stores a pointer to the original object, it
  doesn't need to perform the same read/write backs that reference objects do.
  Similarly, child reference objects only need to read up to the innermost
  QObjectWrapper in a chain to obtain the latest data.

  Returning to the example above, if \c{b} is a Q_OBJECT instead of a value
  type, then it will be the root of the reference chain that has \c{c} as its
  child, without the need to be related to the QObjectWrapper that has been
  built by accessing \c{a}.

  QQmlTypeWrapper, that can wrap a QObject pointer that represents a
  singleton or an attached property, behaves as chain root in the
  exact same way that QObjectWrapper does.
 */

void QQmlDirtyReferenceObject_callback(QQmlNotifierEndpoint *e, void **) {
    static_cast<QV4::Heap::ReferenceObjectEndpoint*>(e)->reference->setDirty(true);
}

namespace QV4 {

void Heap::ReferenceObject::connectToNotifySignal(QObject *obj, int property, QQmlEngine *engine)
{
    Q_ASSERT(obj);
    Q_ASSERT(engine);

    Q_ASSERT(!referenceEndpoint);
    Q_ASSERT(!bindableNotifier);

    referenceEndpoint = new ReferenceObjectEndpoint(this);

    // Connect and signal emission work on "signal indexes". Those are different from "method
    // indexes".
    // The public MetaObject interface can, generally, give us the "method index" of the notify
    // signal.
    // Quite unintuitively, this is true for "notifySignalIndex". As the "method index" and the
    // "signal index" can be different, connecting the "method index" of the notify signal can
    // incur in issues when the signal is being emitted and checking for connected endpoints.
    // For example, we might be connected to the "method index" of the notify signal for the
    // property and end up checking for the subscribers of a different signal when the notify
    // signal is emitted, due to the different meaning of the same index.
    // Thus we pass by the private interface to ensure that we are connecting based on the "signal
    // index" instead.
    const int index = QMetaObjectPrivate::signalIndex(obj->metaObject()->property(property)
                                                                    .notifySignal());
    referenceEndpoint->connect(obj, index, engine);

    // When the object that is being referenced is destroyed, we
    // need to ensure that one additional read is performed to
    // invalidate the data we hold.
    // As the object might be destroyed in a way that doesn't
    // trigger the notify signal for the relevant property, we react
    // directly to the destruction itself.
    // We use a plain connection instead of a QQmlNotifierEndpoint
    // based connection as, currently, declarative-side signals are
    // always discarded during destruction (see
    // QQmlData::signalEmitted).
    // In theory it should be possible to relax that condition for
    // the destroy signal specifically, which should allow a more
    // optimized way of connecting.
    // Nonetheless this seems to be the only place where we have
    // this kind of need, and thus go for the simpler solution,
    // which can be changed later if the need arises.
    new (onDelete) QMetaObject::Connection(
            QObject::connect(obj, &QObject::destroyed, obj, [this](){ setDirty(true); }));
}

void Heap::ReferenceObject::connectToBindable(QObject *obj, int property, QQmlEngine *engine)
{
    Q_ASSERT(obj);
    Q_ASSERT(engine);

    Q_ASSERT(!referenceEndpoint);
    Q_ASSERT(!bindableNotifier);

    bindableNotifier = new QPropertyNotifier(obj->metaObject()->property(property).bindable(obj)
                                                     .addNotifier([this](){ setDirty(true); }));
    new (onDelete) QMetaObject::Connection(
            QObject::connect(obj, &QObject::destroyed, obj, [this]() { setDirty(true); }));
}

bool ReferenceObject::shouldConnect(Heap::ReferenceObject *ref)
{
    if (ref->isAlwaysDirty())
        return false;

    // We want to connect when we are reading the reference object from a statement that is not
    // its creation statement. In other words, when first storing it in a variable and then
    // reusing it, potentially multiple times. This ensures that the cost of the connection itself
    // is only paid when the user separated the declaration from the usage in their code, in the
    // hope that this catches more complex and frequent uses, like accesses in a loop, where
    // unnecessary copies would impact performance more significantly, and not one time accesses.
    if (CppStackFrame *frame = ref->internalClass->engine->currentStackFrame) {
        const auto *refObject = static_cast<Heap::ReferenceObject *>(ref);
        if (frame->v4Function && frame->v4Function == refObject->function()
            && frame->statementNumber() == refObject->statementIndex()) {
            return false;
        }
    }
    return true;
}

void ReferenceObject::connect(Heap::ReferenceObject *ref)
{
    int property = ref->property();
    Heap::Object *object = ref->object();

    while (object
           && object->internalClass->vtable->type != Managed::Type_V4QObjectWrapper
           && object->internalClass->vtable->type != Managed::Type_QMLTypeWrapper)
    {
        const auto type = object->internalClass->vtable->type;
        if (type != Managed::Type_V4ReferenceObject
            && type != Managed::Type_V4Sequence
            && type != Managed::Type_DateObject
            && type != Managed::Type_QMLValueTypeWrapper) {
            break;
        }

        property = static_cast<QV4::Heap::ReferenceObject *>(object)->property();
        object = static_cast<QV4::Heap::ReferenceObject *>(object)->object();
    }

    const auto doConnect = [&](auto wrapper) {
        // The object may be valid on the first read but not on the second, check for that.
        if (QObject *obj = wrapper->object()) {
            auto *refObject = static_cast<Heap::ReferenceObject *>(ref);

            auto *qmlEngine = object->internalClass->engine->qmlEngine();
            if (qmlEngine && obj->metaObject()->property(property).isBindable())
                refObject->connectToBindable(obj, property, qmlEngine);
            else if (qmlEngine && obj->metaObject()->property(property).hasNotifySignal())
                refObject->connectToNotifySignal(obj, property, qmlEngine);
        }
    };

    if (object && object->internalClass->vtable->type == Managed::Type_V4QObjectWrapper) {
        doConnect(static_cast<QV4::Heap::QObjectWrapper *>(object));
    } else if (object && object->internalClass->vtable->type == Managed::Type_QMLTypeWrapper) {
        auto wrapper = static_cast<QV4::Heap::QQmlTypeWrapper *>(object);
        Scope scope(object->internalClass->engine);
        Scoped<QV4::QQmlTypeWrapper> scopedWrapper(scope, wrapper);
        doConnect(scopedWrapper);
    }

    if (!ref->isConnected()) {
        // Mark AlwaysDirty to prevent further connection failures on every read
        ref->setAlwaysDirty(true);
    }
}

} // namespace QV4

QT_END_NAMESPACE
