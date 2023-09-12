
plugins {
    $(ProjectPlugins)
}

android {
    // Here we use TargetSDK to have possible access to newer APIs
    compileSdk = $(TargetSDK)
    // buildToolsVersion = "$ (AndroidBuildToolsVersion)"

    namespace = "$(Namespace)"

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
    }

    buildTypes {
        /**
         * This macro generates required (by IBT) build type base changes.
         *   It's recommended to apply your own changes with additional 'getByName' clauses or replace this macro with the generated results.
         */
        $(ProjectCustomConfigurationTypes)
    }

    sourceSets {
        getByName("main") {
            res.setSrcDirs(listOf("$(ProjectDir)/src/main/res"))
            java.setSrcDirs(listOf("$(ProjectDir)/src/main/java"))
            manifest.srcFile("$(ProjectDir)/src/main/AndroidManifest.xml")
        }

        $(ProjectJNISources)
    }
}

afterEvaluate {
    /**
     * IBT Integration for FastBuild build system.
     *  To properly handle our build system, we create a new additional tasks that mimic the behavior of built-in CMakeBuild / NdkBuild steps.
     *  Because gradle requires the outputs to be stored in a sub-folder with a specific ABI name, IBT is handling this by default.
     *
     * NOTE: Currently only ARM64 is supported as a value ABI target using the default IBT Pipeline.
     *  You can add additional ABI support by calling the proper pipeline for the given ABI.
     */
    android.buildTypes.configureEach {
        val buildConfig = this.name.replaceFirstChar { it.titlecase() }
        val buildPipeline = "Android$(CompileSDK)"
        val abiList = listOf("ARMv8")

        for (abi in abiList)
        {
            val fbuildTask = tasks.register<Exec>("compile${buildConfig}${abi}UsingFastbuild") {
                workingDir("$(WorkspaceDir)")
                executable("$(WorkspaceDir)/$(ScriptFile)")
                commandLine(listOf("$(WorkspaceDir)/$(ScriptFile)", "build", "-t", "all-${buildPipeline}-${buildConfig}"))
            }

            tasks["merge${buildConfig}NativeLibs"].dependsOn(fbuildTask)
        }
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.4.0")
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
}
