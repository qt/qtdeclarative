# The qmllint plugin system (Qml Static Analysis / QmlSA)

In order to allow for users to extend qmllint and for us to separate module specific linting warnings into separate modules so they don't pollute qmllint - we created a plugin system. We will refer to it as QmlSA for short in the rest of this document.

It currently does not have a stable API and doesn't have any public headers.

## Anatomy of a plugin

A basic QQmlSA plugin header looks like this:

```cpp
#include <QtPlugin>
#include <QtCore/qobject.h>
#include <QtQmlCompiler/private/qqmlsa_p.h>

class FooPlugin : public QObject, public QQmlSA::LintPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QmlLintPluginInterface_iid FILE "metadata.json")
    Q_INTERFACES(QQmlSA::LintPlugin)

public:
    void registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement) override;
};
```

Note the reference to the metadata file. This is required in order for a QQmlSA plugin to work properly and must contain the following entries:

```js
{
    /* The name of the plugin. Shouldn't contain any spaces, ideally with the first letter capitalized */
    "name": "PluginName",
    /* Plugin author. You may or may not provide a contact email in here as well */
    "author": "John Doe",
    /* A short description of the plugin - should not contain multiple lines */
    "description": "A plugin for doing X",
    /* A version string. There are no strict requirements in how you use this but it should be present */
    "version": "1.0"
}
```

The plugin implementation itself is rather simple:

```cpp
void FooPlugin::registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement)
{
    // You can register either element or property passes
    manager->registerElementPass(std::make_unique<SomeElementPass>(manager));
    manager->registerPropertyPass(std::make_shared<PropertyTest>(manager), "SomeModule", "SomeElement",
                                  "somePropertyName");
    // ...or check whether a module has been imported in a certain file
    manager->hasImportedModule("QtQuick.Controls")

    // You may also inspect the root element
    rootElement->filePath()
}
```

As you can see there's calls to ``registerElementPass`` adn ``registerPropertyPass`` in the above example. We will explain both of these in more detail in the next section.

## The passes

The way QQmlSA plugins work is that they can register different kinds of passes which will run and iterate over the QML file being linted in different ways.

Right now QmlSA has two types of passes:

## Element passes (QQmlSA::ElementPass)

These run for every QML element present in the QML scope being linted.

### Methods

The pass has a ``shouldRun`` and ``run`` method.

``shouldRun`` checks whether the element is interesting to the pass at hand or not (i.e. by checking whether the element is derived from a certain type). It then returns either true or false.

If it returns true, the ``run`` is executed. This will do the actual checking and emitting of warnings.

**Do not do anything computationally expensive in shouldRun**. Either it has to be moved into the constructor (i.e. resolving a type that has to be checked against) or it should be done later in ``run``. This is because shouldRun will be run for every single element in the document and this can easily get too much.

## Property passes (QQmlSA::PropertyPass)

These run every time a property is written from, written to or has a binding created on.

Because there are many more instances of this happening in a QML document then there are elements this pass works a bit differently than element passes.

When registering a property pass you will need to specify what type and property the pass should work on. You may omit the type name if you're interested in all properties with a certain name (i.e. ``width``) or omit the property name if you're interested in all properties of a certain type.
**In the interest of performance you should always prefer fully specifying what you're interested in if at all possible.**

Note that unlike with Element passes you may register the same pass multiple times with different types and properties. This is so you can use one pass across different types and properties.

### Methods

(All of the following only apply if it happens in the scope of the document)

* ``onBinding`` - Called when a binding on a property gets created.
* ``onRead`` - Called when a property is read. Either as part of a binding or in a script block.
* ``onWrite`` - Called when a property is written to. Usually as part of a script block.

All of these bindings get the following parameters:
* ``element`` - The QQmlSA::Element the property is a part of
* ``propertyName`` - The name of the property that has been changed (mainly interesting when the pass runs on multiple property names)
* ``bindingScope/readScope/writeScope`` - The actual scope the action took place. This can be different from the element in case a property is written/read to from another scope or we are talking about grouped properties.
* ``location`` - A QQmlJS::SourceLocation indicating where this took place. This is useful for printing locations in warnings

Then there are some specific parameters:
* onBinding
  * ``binding`` - A ``QQmlJSMetaPropertyBinding`` property that contains type information about the binding and what kind of binding is being dealt with
  * ``value`` - A QQmlSA::Element mainly used in case it's an object binding (i.e. ``contentItem: Item {}``)
* onWrite
  * ``value`` - A QQmlSA::Element containing the value that is being written to the property. It may also only contain type information if it's not a proper object.

## API available to all passes

In these methods passes can use the following methods to interact with qmllint further:
* ``void emitWarning(QAnyStringView message, QQmlJS::SourceLocation srcLocation = QQmlJS::SourceLocation())``
  * (should only be used in ElementPass::run and PropertyPass:onRead/Write/Binding) emits a warning highlighting the piece of code given in source location.
* ``QQmlSA::Element resolveType(QAnyStringView moduleName, QAnyStringView typeName)`` - Allows you to obtain any type from any module i.e. for later comparisons in ``shouldRun``. Use this in the constructor of your pass as doing this every time in ``shouldRun`` would be very expensive.

## TODOs
- Stabilize the API
- Make the API public
  - Don't expose QQmlJSMeta* types or QQmlJSScopes (currently ``QQmlSA::Element == QQmlJSScope``)
    - Instead: Create a wrapper around these types
