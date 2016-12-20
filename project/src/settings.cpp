#include "settings.h"
#include <QFile>
#include <QSettings>

Settings settings;


/**
  [SETTINGS]
  Loads the application settings, or, if no saved settings are available,
  loads default values for the settings. You can change the defaults here.
**/
void Settings::loadSettingsOrDefaults() {
    // Set the default values below
    QSettings s("CS123", "Final");

    // Scene picker
    modeScene = s.value("mode", MODE_SCENE1).toInt();

    // Light Intensitites
    l1Intensity = s.value("l1Slider", 1).toInt();
    l2Intensity = s.value("l2Slider", 1).toInt();
    l3Intensity = s.value("l3Slider", 1).toInt();

    // Passes / Features
    useStochastic = s.value("cbStochastic", false).toBool();
    useAO = s.value("cbAO", false).toBool();
    useNM = s.value("cbNM", false).toBool();
    useDOF = s.value("cbDOF", false).toBool();
    aperture = s.value("apertureSlider", 1).toInt();
    focalLength = s.value("focalSlider", 1).toInt();

    // Samples
    numSamples = s.value("samplesSlider", 1).toInt();

    // Lighting equation components
    useAmbient = s.value("cbAmbient", true).toBool();
    useDiffuse = s.value("cbDiffuse", true).toBool();
    useSpecular = s.value("cbSpecular", true).toBool();
    useShadows = s.value("cbShadows", false).toBool();
    useReflections = s.value("cbReflectoins", false).toBool();
    useTextures = s.value("cbTextures", false).toBool();
    useEnvironment = s.value("cbEnviro", false).toBool();

    // Camera
    useAnimation = s.value("cbAnimation", true).toBool();
}

void Settings::saveSettings() {
    QSettings s("CS123", "Final");

    // Scene Picker
    s.setValue("mode", modeScene);

    // Light Intensities
    s.setValue("l1Slider", l1Intensity);
    s.setValue("l2Slider", l2Intensity);
    s.setValue("l3Slider", l3Intensity);

    // Passes / Features
    s.setValue("cbStochastic", useStochastic);
    s.setValue("cbAO", useAO);
    s.setValue("cbDOF", useDOF);
    s.setValue("cbNM", useNM);
    s.setValue("apertureSlider", aperture);
    s.setValue("focalSlider", focalLength);

    // Samples
    s.setValue("samplesSlider", numSamples);

    // Lighting equation components
    s.setValue("cbAmbient", useAmbient);
    s.setValue("cbDiffuse", useDiffuse);
    s.setValue("cbSpecular", useSpecular);
    s.setValue("cbShadows", useShadows);
    s.setValue("cbReflections", useReflections);
    s.setValue("cbTerrain", useTextures);
    s.setValue("cbEnviro", useEnvironment);

    // Camera
    s.setValue("cbAnimation", useAnimation);
}
