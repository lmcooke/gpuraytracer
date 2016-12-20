#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>

// Enumeration values for the modes from which the user can choose in the GUI.
enum Mode
{
    MODE_SCENE1,
    MODE_SCENE2,
    MODE_SCENE3,
    NUM_MODES
};

/**

    @struct Settings
    @brief  Stores application settings for the CS123 GUI.

    You can access all app settings through the "settings" global variable.
    The settings will be automatically updated when things are changed in the
    GUI (the reverse is not true however: changing the value of a setting does
    not update the GUI).

*/
struct Settings {
    // Loads settings from disk, or fills in default values if no saved settings exist.
    void loadSettingsOrDefaults();

    // Saves the current settings to disk.
    void saveSettings();

    // Scene Selection
    int modeScene;           // The currently selected scene //TODO: Update naming

    // Lighting intensities
    int l1Intensity;    // The intensity for light 1
    int l2Intensity;    // The intensity for light 2
    int l3Intensity;    // The intensity for light 3

    // Passes / Features
    bool useStochastic; // Stochastic Sampling
    bool useAO;         // Ambient Occlusion
    bool useNM;         // Normal Mapping
    bool useDOF;        // Depth of Field
    int aperture;     // Depth of field aperture size
    int focalLength;  // Depth of field focal length

    // Samples
    int numSamples;

    // Lighting equation components
    bool useAmbient;
    bool useDiffuse;
    bool useSpecular;
    bool useShadows;
    bool useReflections;
    bool useTextures;
    bool useEnvironment;

    // Animation
    bool useAnimation;

};

// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H
