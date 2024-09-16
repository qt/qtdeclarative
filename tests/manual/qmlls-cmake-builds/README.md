# Testing the automatic qmlls CMake type registration

First of all, setup qmlls in your favorite editor (see https://www.qt.io/blog/whats-new-in-qml-language-server-qmlls-shipped-with-qt-6.6 for instructions).

## Steps

1. Open the manual test using the `CMakeLists.txt` (for QtC, for example) or the folder in which the `CMakeLists.txt` lies (for VS Code, for example) in the editor prepared for qmlls.
2. Make sure the project is configured. It does not need to be built, just configured via CMake.
3. In your editor, open the `Main.qml` and `helloworld.h`.
4. Modify the `helloworld.h` file by commenting the existing `Q_PROPERTY myPPP` out and save `helloworld.h`.
5. In the Main.qml file, write some code (add or delete a newline). It should complain about the binding to `myPPP` in
`HelloWorld`, as the property does not exist anymore. It should also not propose myPPP as autocompletion
in `HelloWorld` anymore.

6. Repeat steps 4 + 5 with your own modifications and check that the modification in the `helloworld.h` can be seen in
the `Main.qml` file, without having to rebuild the project yourself.
