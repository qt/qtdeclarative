plugins {
    id("com.android.application")
    id("org.qtproject.qt.gradleplugin") version "1.+"
}

//! [build.gradle QtBuild config]
QtBuild {
    // Relative for Qt (Installer or MaintenanceTool) installations.
    qtPath = file("../../../../../../../6.11.3")
    projectPath = file("../../qtabstractlistmodel")
}
//! [build.gradle QtBuild config]

android {
    namespace = "com.example.qtabstractlistmodel_kotlin"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.example.qtabstractlistmodel_kotlin"
        minSdk = 28
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        named("release") {
            isMinifyEnabled = false
            setProguardFiles(listOf(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro"))
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlin {
        jvmToolchain(17)
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("com.google.android.material:material:1.12.0")
}

