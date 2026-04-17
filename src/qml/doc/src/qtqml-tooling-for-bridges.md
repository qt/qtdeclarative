# QML tooling for Qt Bridge projects

The Qt Bridge project will need to generate some files in the build folder for qmllint and qmlls to find and
understand the QML types defined by that Qt Bridge project.
For each QML Module defined by the project, files like qmldir, qmltypes, resources files etc should be generated
in the build folder.

## Files to generate for a QML Module

### qmldir

Tooling like qmlls or qmllint, just like the QML runtime, detect folders with a qmldir file as QML Modules.
The qmldir format is described [here](https://doc.qt.io/qt-6/qtqml-modules-qmldir.html).
The location of the folder with the qmldir should be in the import path and should follow the URI. For example, module
`MyModule` and `org.com.mycompany` should have their respective qmldir files at `/path/to/importpath/MyModule/qmldir`
and `/path/to/importpath/org/com/mycompany/qmldir`. The other files of the QML Modules, like qml files, qmltypes, and
so on, should be in the same folder as the qmldir.

TLDR: the qmldir needs at least
* a `module` entry,
* a `typeinfo` entry, for the types exposed by a Qt Bridge project,
* `depends` entries for all the QML Modules that this QML Module depends on,
* either:
* * the list of QML files in the module if you don't have a resource system, or
* * a `prefer` entry referring to the location of the QML files in the resource system

### .qmltypes

The qmldir file refers to a .qmltypes file via a [typeinfo directive](https://doc.qt.io/qt-6/qtqml-modules-qmldir.html#type-description-file-declaration).
The qmldir and qmltypes file are usually in the same folder. The .qmltypes file describes types usable in QML but not
defined via a QML file, for example a QML type from a bridges project. You can generate .qmltypes files with
qmltyperegistrar from JSON.

#### qmltyperegistrar

qmltyperegistrar is the tool that generates the .qmltypes files for the tooling. It requires multiple arguments:

* `--generate-qmltypes`: destination of the qmltypes to generate, `/path/to/MyModule/plugins.qmltypes` for example,
* `--import-name`: URI of the QML module for which the qmltypes file is generated, for example `MyModule`,
* `--major-version`: major version of the module, usually `254` if you don't need any QML Module versioning,
* `--minor-version`: minor version of the module, usually `254` if you don't need any QML Module versioning,
* `--foreign-types`: comma (`,`) separated list of .json files of dependent QML Modules, for example,
```
/path/to/qt/metatypes/qt6core_metatypes.json,/path/to/qt/metatypes/qt6qml_metatypes.json,/path/to/qt/metatypes/qt6quick_metatypes.json,/path/to/qt/metatypes/qt6gui_metatypes.json,/path/to/qt/metatypes/qt6qmlmeta_metatypes.json
```
* and finally the input .json file for the current QML Module as argument.

See [MOC's JSON output](#mocs-json-output) on what information qmltyperegistrar requires to generate .qmltypes file.

Note that qmltyperegistrar also has a mode to generate C++ QML type registration code, which is not needed in a Qt Bridge project.

### Resource files

Resource files files end in `.qrc` and follow the XML format. Qmllint and qmlls use resource files to map source files
to build folders. Each QML Module generates three .qrc files, described in the following subsections.

Note that the QML tooling does not actively try to use the resource file system.

#### QML file resources

C++ CMake projects generate this resource file at `/path/to/build/.qt/rcc/X_raw_qml_0.qrc`, where `X` stands for the CMake
target name of the QML Module.

The .qrc file mapping from the qrc path to the source QML file looks like:
```xml
<RCC>
  <qresource prefix="/qt/qml/MyModule/">
    <file alias="Main.qml">/path/to/Main.qml</file>
    <file alias="SomeItem.qml">/path/to/SomeItem.qml</file>
    ...
  </qresource>
</RCC>
```

where:
* `MyModule` is the URI of the QML module this .qrc file belongs to,
* `/qt/qml/MyModule/` describes the qrc path of the module `MyModule`,
* `Main.qml` is a qml file in MyModule, and
* `/path/to/Main.qml` the path to the QML file in the source folder of the user project. Analog for `SomeItem.qml` and `/path/to/SomeItem.qml`, and for each other QML file in the QML Module.

The `/qt/qml/` qrc file system prefix matters for the QML runtime, but not for the QML tooling.

#### Qmldir files resources

C++ CMake projects generate this resource file at `/path/to/build/.qt/rcc/qmake_X.qrc`, where `X` stands for the QML Module URI.

The .qrc file mapping from the qrc path to the build qmldir file looks like:

```xml
<RCC>
  <qresource prefix="/qt/qml/MyModule">
    <file alias="qmldir">/path/to/buildfolder/MyModule/qmldir</file>
  </qresource>
</RCC>
```
where:
* `MyModule` is the URI of the QML module this .qrc file belongs to,
* `/qt/qml/MyModule/` describes the qrc path of the module `MyModule`,
* `qmldir` defines the current QML Module, and
* `/path/to/buildfolder/` is the path to the build folder that contains the MyModule QML Module, and inside the qmldir file.

## How to call qmlls and qmllint

### qmlls

The LSP client is responsible for passing the relevant information to qmlls. The easiest way to do it in Qt 6.10 is
to write a `.qmlls.build.ini` file in the build folder and to call `qmlls` binary with the `-B /path/to/buildfolder` argument.

#### .qmlls.build.ini content

qmlls from 6.10.0 reads the `.qmlls.build.ini` file from the build folder to figure out import paths, documentation paths, etc.
The `.qmlls.build.ini` file is expected to be in `/path/to/build/.qt/.qmlls.build.ini`, where `/path/to/build` is
the path passed to qmlls via the LSP client.

The .qmlls.build.ini file starts with a `[General]` section. That section contains the documentation directory:
```
[General]
docDir=/path/to/qt/doc
version=2
```
where `/path/to/qt/` is the path to the Qt installation.

Starting with Qt 6.12, the `[workspace]` section has a list of workspaces in QSettings's list style.
Each entry in the list contains a source folder, the import paths and the resource files. The first entry starts at 1, not 0.
Here is an example:

```
[General]
docDir=/path/to/qt/doc
version=2

[workspaces]
size=2

1\sourcePath="/path/to/folder1"
1\importPaths="/path/to/qt/qml"
1\resourceFiles="/path/to/resource1.qrc"

2\sourcePath="/path/to/folder2"
2\importPaths="/path/to/qt/qml"
2\resourceFiles="/path/to/resource2.qrc"
```

where:
* `sourcePath` is the folder where in C++ the `CMakeLists.txt` with the `qt_add_qml_module(...)` call
definining the QML Module would be located.
* `importPaths` describes the import paths to use for QML files of the current module.
On windows, `;` separates the different import paths. Linux and macOS use `:` instead.
* `resourceFiles` describes all resource files of the current module.
On windows, `;` separates the different resource files. Linux and macOS use `:` instead.

Note that `importPaths` contains the import paths of:
* the Qt kit that the project is using,
* all QML Modules that this QML Module depends on, see also the `depends` entries of the current QML Module's [qmldir](#qmldir) file.

#### .qmlls.build.ini content before 6.12
Qmlls from Qt 6.12 and later also support the previous format where source folders have their own
ini-section and have `/` escaped with `<SLASH>`.
For example, the QML Module at `/tmp/untitled` would have the section name `[<SLASH>tmp<SLASH>untitled]`.

```
[General]
docDir=/path/to/qt/doc
version=1

[<SLASH>tmp<SLASH>untitled]
importPaths="/path/to/qt/qml"
```

### qmllint

The CMake build system calls qmllint in the C++/CMake world.

To call qmllint without CMake, pass import paths as described before in the [.qmlls.build.ini part](#qmllsbuildini-content) via the
`-I` option. To pass multiple import paths, use `-I` multiple times. In addition to the imports path specified this way, qmllint
will use its own default import path, the current directory, and all qmldir found inside the current directory. To disable the automatic
adding of import paths, use the `--bare` option.

Then, pass the resource files (`.qrc` files) to qmllint via the `--resource` commandline option. Qmllint reads the resource files to correctly
map QML files from their build path to their source paths. The content of the .qrc files is described earlier in the
[resource file section](#resource-files).

User `--bare` to avoid adding the default import path, and the current directory and auto-imports any qmldir found in the current directory.

Next, pass `--bare` and the file to be linted to qmllint. `--bare` ensures that only the import paths passed on the commandline
are used for the linting.

A qmllint invokation on a Main.qml file, which is in the `MyModule` QML module and depends on a QML module located in `/path/to/build/`
would look like
```
qmllint                                                           \
    -I /path/to/build/                                            \
    -I /path/to/qt/qml                                            \
    --resource /path/to/build/.qt/rcc/qmake_mymodule.qrc          \
    --resource /path/to/build/.qt/rcc/appmymodule_raw_qml_0.qrc   \
    --bare                                                        \
    /path/to/source/MyModule/Main.qml

```
for example.


## MOC's JSON output

This section describes how to generate the JSON required by qmltyperegistrar for the .qmltypes files.

The root element of the JSON is a list, like
```
[
    {
        "classes": [ ... ],
        "inputFile": "MyItem.h",
        "outputRevision": 69
    },
    ...
]
```
for example.

where:
* classes describes the QML Elements. In this section, *QML elements* refer to Objects that can be used from
QML and that are defined in your Qt Bridge language.
* inputFile is the filename of the source file that defined the QML Elements. Qmlls uses this filename to
"jump to definition", for example. Multiple files with the same name but different locations should be avoided
in the project structure.
* outputRevision is the MOC's revision, currently hard-coded in MOC's source code.

### Describing the QML Elements

The classes list contains objects like:
```
{
    "classInfos": [
        {
            "name": "QML.Element",
            "value": "myGadget"
        }
    ],
    "className": "MyGadget",
    "gadget": true,
    "lineNumber": 6,
    "properties": [
        {
            "constant": false,
            "designable": true,
            "final": false,
            "index": 0,
            "lineNumber": 10,
            "member": "m_myInt",
            "name": "myInt",
            "required": false,
            "scriptable": true,
            "stored": true,
            "type": "int",
            "user": false
        },
        {
            "constant": false,
            "designable": true,
            "final": false,
            "index": 1,
            "lineNumber": 13,
            "member": "m_myInt2",
            "name": "myInt2",
            "required": false,
            "scriptable": true,
            "stored": true,
            "type": "int",
            "user": false
        }
    ],
    "qualifiedClassName": "MyGadget"
}
```

The next subsections explain what each field should contain in detail.

### classInfos

`classInfos` describes how a type should be registered in the QML type system and is a list of objects of the
form:
```
"classInfos": [
    {
        "name": "QML.Element",
        "value": "myGadget"
    },
    ...
],
```
In a C++ CMake project, the classInfos are set via C++'s QML_* and Q_CLASSINFO macros. Here is a "translation" table from
QML_* C++ macro to class info object:

| corresponding C++ macro                                  | name(s) (as string)                    | value(s)                                                               |
|----------------------------------------------------------|----------------------------------------|------------------------------------------------------------------------|
| QML_ELEMENT                                              | QML.Element                            | "auto"                                                                 |
| QML_NAMED_ELEMENT("name")                                | QML.Element                            | "name"                                                                 |
| QML_ANONYMOUS                                            | QML.Element                            | "anonymous"                                                            |
| QML_UNCREATABLE("reason...")                             | QML.Creatable, QML.UncreatableReason   | "false", "reason..."                                                   |
| QML_VALUE_TYPE("name")                                   | QML.Element                            | "name"                                                                 |
| QML_CONSTRUCTIBLE_VALUE                                  | QML.Creatable, QML.CreationMethod      | "true", "construct"                                                    |
| QML_STRUCTURED_VALUE                                     | QML.Creatable, QML.CreationMethod      | "true", "structured"                                                   |
| QML_SINGLETON                                            | QML.Singleton                          | "true"                                                                 |
| QML_ADDED_IN_VERSION(x,y)                                | QML.AddedInVersion                     | `QString::number(QTypeRevision::fromVersion(x, y).toEncodedVersion())` |
| QML_EXTRA_VERSION(x,y)                                   | QML.ExtraVersion                       | `QString::number(QTypeRevision::fromVersion(x, y).toEncodedVersion())` |
| QML_REMOVED_IN_VERSION(X,Y)                              | QML.RemovedInVersion                   | `QString::number(QTypeRevision::fromVersion(x, y).toEncodedVersion())` |
| QML_ATTACHED(attachedType)                               | QML.Attached                           | attachedType's name                                                    |
| QML_EXTENDED(extendedType)                               | QML.Extended                           | extendedType's name                                                    |
| QML_EXTENDED_NAMESPACE(extendedNamespace)                | QML.Extended, QML.ExtensionIsNamespace | extendedNamespace's name, "true"                                       |
| QML_NAMESPACE_EXTENDED(extendedNamespace)                | QML.Extended                           | extendedNamespace's name                                               |
| QML_SEQUENTIAL_CONTAINER(valueType)                      | QML.Sequence                           | valueType's name                                                       |
| QML_FOREIGN(foreignType)                                 | QML.Foreign                            | foreignType's name                                                     |
| QML_UNAVAILABLE                                          | QML.Foreign                            | "QQmlTypeNotAvailable"                                                 |
| QML_FOREIGN_NAMESPACE(foreignNamespace)                  | QML.Foreign, QML.ForeignIsNamespace    | foreignNamespace's name, "true"                                        |
| QML_CUSTOMPARSER                                         | QML.HasCustomParser                    | "true"                                                                 |
| QML_USING(originalType)                                  | QML.Using                              | originalType's name                                                    |
| QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_APPEND                 | QML.ListPropertyAssignBehavior         | "Append"                                                               |
| QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE_IF_NOT_DEFAULT | QML.ListPropertyAssignBehavior         | "ReplaceIfNotDefault"                                                  |
| QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE                | QML.ListPropertyAssignBehavior         | "Replace"                                                              |


Direct usage of the `Q_CLASSINFO` macro in C++ is used to:
* define default and parent properties via `DefaultProperty` and `ParentProperty`, see also [here](https://doc.qt.io/qt-6/qtqml-cppintegration-definetypes.html#specifying-default-and-parent-properties-for-qml-object-types)
* tweak enum scope behavior via `RegisterEnumClassesUnscoped`, see also [here](https://doc.qt.io/qt-6/qtqml-cppintegration-data.html#enumeration-types)
* define deferred or immediate properties via `DeferredPropertyNames` and `ImmediatePropertyNames`


### className
className is the name of the class. In the C++ world, this is the name of the C++ class that contains the QML_ELEMENT
macro, like

```
"className": "MyClass"
```
for example.

### qualifiedClassName
className is the fully-qualified name of the class. In the C++ world, this is the name of the C++ class that contains the QML_ELEMENT
macro, like

```
"className": "MyNamespace::MyClass"
```

for example.

### lineNumber
The line number where this Class is defined. Qmlls uses this information for "go to definition" from QML to the Qt Bridge language, for example

```
"lineNumber": 45
```

### properties
Properties contain a list of properties with following fields:

| Property field | Description                                    | Type                                                  |
|----------------|------------------------------------------------|-------------------------------------------------------|
| bindable       | name of the QBindable\<T> member               | boolean                                               |
| constant       | true for constant properties                   | boolean                                               |
| final          | true for final properties                      | boolean                                               |
| index          | index of the property in the QMetaObject       | int                                                   |
| lineNumber     | the line number where this property is defined | int                                                   |
| member         | name of the member accessor                    | string                                                |
| name           | name of the property                           | string                                                |
| notify         | name of the change signal                      | string                                                |
| privateClass   | name of the private class                      | string                                                |
| read           | name of the getter                             | string                                                |
| required       | true for required properties                   | boolean                                               |
| reset          | name of the reset method                       | string                                                |
| revision       | revision of the property                       | int, obtained via `QTypeRevision::toEncodedVersion()` |
| type           | name of the type of the property               | string                                                |
| write          | name of the setter                             | string                                                |

Some field names correspond to the arguments of Q_PROPERTY, see its documentation
[here](https://doc.qt.io/qt-6/properties.html).


For example:

```
"properties": [
    {
        "constant": false,
        "final": false,
        "index": 0,
        "lineNumber": 10,
        "name": "myInt",
        "required": false,
        "type": "int",
    },
    ...
],
```

### enums
Enums contain a list of enums with following fields:

| Enum field | Description                                         | Type           |
|------------|-----------------------------------------------------|----------------|
| alias      | alternative name of the enum                        | string         |
| isClass    | true for enums defined with `enum class ...` in C++ | boolean        |
| isFlag     | true for QFlags                                     | boolean        |
| lineNumber | line number where this enum is defined              | int            |
| name       | name of the enum                                    | string         |
| type       | type of the enum                                    | string         |
| values     | list of enum values                                 | list\<string>  |

For example:

```
"enums": [
    {
        "isClass": false,
        "isFlag": false,
        "lineNumber": 33,
        "name": "HelloEnum",
        "values": [
            "HelloEnumValue",
            "HelloEnumValue2"
        ]
    },
    ...
]
```

### methods
The field methods contains a list of methods with following fields:

| Method field         | Description                                               | Type                                                  |
|----------------------|-----------------------------------------------------------|-------------------------------------------------------|
| access               | method access, can be `public`, `protected`, or `private` | string                                                |
| arguments            | method argument names and types                           | list of objects with `name` and `type` string fields  |
| index                | index of the method in the QMetaObject                    | int                                                   |
| isCloned             | true when this method is cloned (e.g. overloads)          | boolean                                               |
| isConstructor        | true for JS constructors                                  | boolean                                               |
| isConst              | true for const methods                                    | boolean                                               |
| isJavaScriptFunction | true for variable argument functions in JS                | boolean                                               |
| lineNumber           | line number where this method is defined                  | int                                                   |
| name                 | name of the method                                        | string                                                |
| returnType           | type of the return value                                  | string                                                |
| revision             | revision of the method                                    | int, obtained via `QTypeRevision::toEncodedVersion()` |


For example:

```
"methods": [
    {
        "access": "public",
        "arguments": [
            {
                "name": "index",
                "type": "int"
            },
            {
                "name": "count",
                "type": "int"
            }
        ],
        "index": 3,
        "lineNumber": 40,
        "name": "remove",
        "returnType": "void"
    },
    ...
]
```

Note that for `isCloned`, in C++, the following code:
```
Q_INVOKABLE void foo(int i = 42)
```
generates two methods: one with one argument, and one without arguments to support the default argument.
This might require some care when interacting with Qt Bridge languages that support default parameters.

### signals
The field signals contains a list of signals. Signals have the same fields as [methods](#methods), like
```
"signals": [
    {
        "access": "public",
        "index": 0,
        "lineNumber": 60,
        "name": "myPropertyChanged",
        "returnType": "void"
    },
]
```
for example.

### slots
The field slots contains a list of slots. Slots have the same fields as [methods](#methods), like

```
"slots": [
    {
        "access": "public",
        "index": 0,
        "lineNumber": 70,
        "name": "mySlots",
    },
]
```
for example.

### constructors
The field constructors contains a list of JS constructors. JS constructors can be called from JS
with the JS `new` operator, not to be confused with C++ or the Qt Bridge language constructor.
JS constructors have the same fields as [methods](#methods), like
```
"constructors": [
    {
        "access": "public",
        "index": 3,
        "lineNumber": 60,
        "name": "MyConstructor",
        "isConstructor": true
    },
]
```
for example.

### object
True when type is an object, false when it is a gadget or namespace, like

```
"object": true
```
for example.

### gadget
True when type is a gadget, false when it is an object or namespace, like
```
"gadget": true
```
for example.

### namespace
True when exposing a namespace, false for objects or gadgets, like
```
"namespace": true
```
for example.


### superClasses

The field superClasses contains a list with one base type with following fields:

| Base type field | Description                                                 | Type   |
|-----------------|-------------------------------------------------------------|--------|
| access          | access modifier, can be `public`, `protected`, or `private` | string |
| name            | name of the base type                                       | string |

like
```
"superClasses": [
    {
        "access": "public",
        "name": "QObject"
    }
]
```
for example.

Note that multiple super classes are currently not supported, use the [interfaces](#interfaces) field as a substitute for
multiple inheritance.

### interfaces

The field interfaces contains a list of interface names this type implements, like
```
"interfaces": [ { "className": "QQmlParserStatus", }, ... ]
```
for example.

In C++, this is written by the
[Q_INTERFACES or QML_IMPLEMENTS_INTERFACES](https://doc.qt.io/qt-6/qobject.html#Q_INTERFACES) macros.
