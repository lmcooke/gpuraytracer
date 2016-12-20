#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <assert.h>
#include <QGridLayout>
#include <iostream>
#include "databinding.h"
#include "settings.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    QGLFormat qglFormat;
    qglFormat.setVersion(4,0);
    qglFormat.setProfile(QGLFormat::CoreProfile);
    m_ui->setupUi(this);
    QGridLayout *gridLayout = new QGridLayout(m_ui->centralWidget);
    m_view = new View(qglFormat, this);
    m_view->setMinimumSize(100, 100);
    gridLayout->addWidget(m_view, 0, 1);


    settings.loadSettingsOrDefaults();
    dataBind();

    // Restore the UI settings
    QSettings qtSettings("CS123", "Final");
    restoreGeometry(qtSettings.value("geometry").toByteArray());
    restoreState(qtSettings.value("windowState").toByteArray());
}

MainWindow::~MainWindow()
{
    foreach (DataBinding *b, m_bindings) {
        delete b;
    }
    delete m_ui;
    delete m_view;
}


/**
 * @brief MainWindow::dataBind
 * Bind the elements of the UI with their values in Settings
 */
void MainWindow::dataBind() {
#define BIND(b) { DataBinding *_b = (b); m_bindings.push_back(_b); assert(connect(_b, SIGNAL(dataChanged()), this, SLOT(settingsChanged()))); }

    BIND(ChoiceBinding::bindRadioButtons(NUM_MODES, settings.modeScene,
                                    m_ui->modeScene1,
                                    m_ui->modeScene2,
                                    m_ui->modeScene3));

    // Lighting intensities
    BIND(IntBinding::bindSliderAndTextbox(
        m_ui->l1Slider, m_ui->l1Text, settings.l1Intensity, 0.f, 100.f));
    BIND(IntBinding::bindSliderAndTextbox(
        m_ui->l2Slider, m_ui->l2Text, settings.l2Intensity, 0.f, 100.f));
    BIND(IntBinding::bindSliderAndTextbox(
        m_ui->l3Slider, m_ui->l3Text, settings.l3Intensity, 0.f, 100.f));

    // Passes / Features
    BIND(BoolBinding::bindCheckbox(m_ui->cbStochastic, settings.useStochastic));
    BIND(BoolBinding::bindCheckbox(m_ui->cbAO, settings.useAO));
    BIND(BoolBinding::bindCheckbox(m_ui->cbDOF, settings.useDOF));
    BIND(BoolBinding::bindCheckbox(m_ui->cbNM, settings.useNM));
    BIND(IntBinding::bindSliderAndTextbox(
        m_ui->apertureSlider, m_ui->apertureText, settings.aperture, 1, 100));
    BIND(IntBinding::bindSliderAndTextbox(
        m_ui->focalSlider, m_ui->focalText, settings.focalLength, 1, 50));

    // Samples
    BIND(IntBinding::bindSliderAndTextbox(
        m_ui->samplesSlider, m_ui->samplesText, settings.numSamples, 1, 80));

    // Lighting equation components
    BIND(BoolBinding::bindCheckbox(m_ui->cbAmbient, settings.useAmbient));
    BIND(BoolBinding::bindCheckbox(m_ui->cbDiffuse, settings.useDiffuse));
    BIND(BoolBinding::bindCheckbox(m_ui->cbSpecular, settings.useSpecular));
    BIND(BoolBinding::bindCheckbox(m_ui->cbShadows, settings.useShadows));
    BIND(BoolBinding::bindCheckbox(m_ui->cbReflection, settings.useReflections));
    BIND(BoolBinding::bindCheckbox(m_ui->cbTextures, settings.useTextures));
    BIND(BoolBinding::bindCheckbox(m_ui->cbEnviro, settings.useEnvironment));

    // Camera
    BIND(BoolBinding::bindCheckbox(m_ui->cbAnimation, settings.useAnimation));


#undef BIND
}

void MainWindow::settingsChanged() {
    m_view->update();
    m_view->settingsChanged(); // TODO: Might have to move this earlier in this function
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Save the settings before we quit
    settings.saveSettings();
    QSettings qtSettings("CS123", "Final");
    qtSettings.setValue("geometry", saveGeometry());
    qtSettings.setValue("windowState", saveState());

    QMainWindow::closeEvent(event);
}
