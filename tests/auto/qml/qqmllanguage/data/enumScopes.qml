import QtQml 2.15
import EnumScopeTest 1.0

QtObject {
    property NonSingleton n: NonSingleton {
        id: nonSingleton
    }

    property bool singletonUnscoped: Singleton.enumProperty === Singleton.EnumValue2
    property bool singletonScoped: Singleton.enumProperty === Singleton.EnumType.EnumValue2
    property bool nonSingletonUnscoped: nonSingleton.enumProperty === NonSingleton.EnumValue2
    property bool nonSingletonScoped: nonSingleton.enumProperty === NonSingleton.EnumType.EnumValue2

    property int singletonScopedValue: EnumProviderSingleton.Expected.Value
    property int singletonUnscopedValue: EnumProviderSingleton.Value
}
