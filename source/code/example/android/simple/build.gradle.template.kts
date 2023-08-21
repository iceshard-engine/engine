
plugins {
    $(ProjectPlugins)
}

afterEvaluate {

    /**
     * IBT Integration for FastBuild build system.
     *  To properly handle our build system, we create two additional tasks that mimic the behavior of built-in CMakeBuild / NdkBuild steps.
     *  To do so we run a build command for each supported Confiuration and ABI. After the build is finished we copy the build results
     *  back to the src/main/jniLibs/{abi} folder so it can be properly grabbed by Gradle during the MergeJniLibFolders task
     *
     * NOTE: Currently only ARM64 is supported as a value ABI target using the default IBT Pipeline.
     *  You can add additional ABI support by calling the proper pipeline for the given ABI.
     */
    android.buildTypes.configureEach {
        val buildConfig = this.name.replaceFirstChar { it.titlecase() }
        val buildPipeline = "Android${android.compileSdk}"
        val abiList = listOf(Pair("ARMv8", "arm64-v8a"))

        for (abi in abiList)
        {
            val copyLibTask = tasks.register<Copy>("copy${buildConfig}FastbuildBinaries") {
                from("$(ProjectOutputDir)")
                include("*.so")
                into("src/main/jniLibs/${abi.second}")
            }

            val fbuildTask = tasks.register<Exec>("compile${buildConfig}${abi.first}UsingFastbuild") {
                workingDir("$(WorkspaceDir)")
                executable("$(WorkspaceDir)/$(ScriptFile)")
                commandLine(listOf("$(WorkspaceDir)/$(ScriptFile)", "build", "-t", "all-${buildPipeline}-${buildConfig}"))
                finalizedBy(copyLibTask)
            }

            tasks["merge${buildConfig}NativeLibs"].dependsOn(fbuildTask)
            tasks["merge${buildConfig}JniLibFolders"].dependsOn(copyLibTask)
        }
    }
}

android {
    compileSdk = $(CompileSDK)
    // buildToolsVersion = "$ (AndroidBuildToolsVersion)"

    sourceSets {
        getByName("main") {
            res.setSrcDirs(listOf("$(ProjectDir)/src/main/res"))
            java.setSrcDirs(listOf("$(ProjectDir)/src/main/java"))
            manifest.srcFile("$(ProjectDir)/src/main/AndroidManifest.xml")
        }
    }

    namespace = "$(Namespace)"

    defaultConfig {

        // Uniquely identifies the package for publishing.
        applicationId = "$(ApplicationId)"

        // Defines the minimum API level required to run the app.
        minSdk = $(MinSDK)

        // Specifies the API level used to test the app.
        targetSdk = $(TargetSDK)

        // Defines the version number of your app.
        versionCode = $(VersionCode)

        // Defines a user-friendly version name for your app.
        versionName = "$(VersionName)"
    }

    buildTypes {
        $(ProjectCustomConfigurationTypes)
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.4.0")
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
}
