import org.gradle.internal.extensions.stdlib.capitalized

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "com.example.flycube"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.example.flycube"
        minSdk = 30
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    buildFeatures {
        prefab = true
    }
    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.31.5"
        }
    }
}

androidComponents {
    onVariants { variant ->
        val variantName = variant.name.capitalized()
        val variantBuildType = variant.buildType!!.capitalized()
        val assetsDir =
            layout.buildDirectory.dir("intermediates/assets_cmake/${variant.name}").get()

        variant.externalNativeBuild!!.arguments.add("-DANDROID_ASSETS_DIR=${assetsDir}")
        variant.sources.assets!!.addStaticSourceDirectory(assetsDir.asFile.absolutePath)

        afterEvaluate {
            val mergeAssetsTask = tasks.named("merge${variantName}Assets")
            val externalNativeBuildTask =
                tasks.named("externalNativeBuild${variantBuildType}")
            mergeAssetsTask {
                dependsOn(externalNativeBuildTask)
            }
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.games.activity)
}
