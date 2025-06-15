
plugins {
    $(ProjectPlugins)
}

android {
    // Here we use TargetSDK to have possible access to newer APIs
    compileSdk = $(TargetSDK)

    namespace = "$(Namespace)"

    // Defines the NDK version to be used for this project
    ndkVersion = "$(NDKVersion)"

    defaultConfig {

        // Uniquely identifies the package for publishing.
        applicationId = "$(ApplicationId)"

        // Defines the minimum API level required to run the app.
        // Note: For native apps the CompileSDK is the MinSDK
        minSdk = $(CompileSDK)

        // Specifies the API level used to test the app.
        targetSdk = $(TargetSDK)

        // Defines the version number of your app.
        versionCode = $(VersionCode)

        // Defines a user-friendly version name for your app.
        versionName = "$(VersionName)"

        // By default keep debug symbols, just remove it in release afterwards
        packaging.jniLibs.keepDebugSymbols += "**/*.so"

        $(IBTBuildSystemIntegration)
    }

    compileOptions {
        targetCompatibility = JavaVersion.VERSION_11
        sourceCompatibility = JavaVersion.VERSION_11
    }

    buildTypes {
        /**
         * This macro generates required (by IBT) build type base changes.
         *   It's recommended to apply your own changes with additional 'getByName' clauses or replace this macro with the generated results.
         */
        $(ProjectCustomConfigurationTypes)

        getByName("profile") {
            // NOTE: Enabled by default for testing.
            signingConfig = signingConfigs.getByName("debug")
        }
    }

    sourceSets {
        getByName("main") {
            res.setSrcDirs(listOf("$(ProjectDir)/src/main/res"))
            java.setSrcDirs(listOf("$(ProjectDir)/src/main/java"))
            manifest.srcFile("$(ProjectDir)/src/main/AndroidManifest.xml")
        }

        $(ProjectJNISources)
    }

    lint {
        baseline = file("lint-baseline.xml")
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.7.1")
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
}
