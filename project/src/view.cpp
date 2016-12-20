#include "view.h"

#include "cs123_lib/resourceloader.h"
#include "cs123_lib/errorchecker.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <iostream>
#include "settings.h"
#include <QImage>
#include <string>

#include "openglshape.h"
#include "gl/textures/Texture2D.h"
#include "gl/shaders/ShaderAttribLocations.h"
#include "sphere.h"
#include "cube.h"

#include "SceneBuilder.h"

using namespace CS123::GL;

/**
  [VIEW] Loads shader programs and geometry, executes FBO pipeline to render the
  ray program to the full screen quad, using FBO ping ponging
  Settings passed into View via Settings data struct
**/
View::View(QGLFormat format, QWidget *parent)
    : QGLWidget(format, parent),
      m_width(width()), m_height(height()),
      m_phongProgram(0), m_textureProgram(0), m_rayProgram(0),
      m_envCubeID1(0), m_envCubeID2(0), m_envCubeProgram(0),
      m_diffuseID(0), m_normalID(0),
      m_woodDiffuseID(0), m_woodNormalID(0),
      m_plasterDiffuseID(0), m_plasterNormalID(0),
      m_textures(nullptr),
      m_quad(nullptr), m_envCube(nullptr), m_square(nullptr),
      m_angleX(-0.0f), m_angleY(0.0f), m_zoom(10.f),
      m_view(glm::mat4x4(1.f)), m_scale(glm::mat4x4(1.f)),
      m_rayFBO1(nullptr), m_rayFBO2(nullptr),
      m_firstPass(true), m_evenPass(true),
      m_numPasses(0),
      m_timer(this),
      m_fps(60.0f),
      m_increment(0),
      m_animationIncrement(0)
{
    // Set up 60 FPS draw loop.
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    m_timer.start(1000.0f / m_fps);
}

// Clean up textures
View::~View()
{
    glDeleteTextures(1, &m_diffuseID);
    glDeleteTextures(1, &m_normalID);
    glDeleteTextures(1, &m_envCubeID1);
    glDeleteTextures(1, &m_woodDiffuseID);
    glDeleteTextures(1, &m_woodNormalID);
    glDeleteTextures(1, &m_plasterDiffuseID);
    glDeleteTextures(1, &m_plasterNormalID);
}


// Initializes shader programs, texture maps, environment cubes
void View::initializeGL() {
    ResourceLoader::initializeGlew();
    glEnable(GL_DEPTH_TEST);

    // Set the color to set the screen when the color buffer is cleared.
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Create shader programs.
    m_phongProgram = ResourceLoader::createShaderProgram(
                ":/shaders/phong.vert", ":/shaders/phong.frag");
    m_textureProgram = ResourceLoader::createShaderProgram(
                ":/shaders/quad.vert", ":/shaders/texture.frag");
    m_rayProgram = ResourceLoader::createShaderProgram(
                ":/shaders/quad.vert", ":/shaders/ray.frag");
    m_compositeProgram = ResourceLoader::createShaderProgram(
                ":/shaders/quad.vert", ":/shaders/composite.frag");
    m_envCubeProgram = ResourceLoader::createShaderProgram(
                ":/shaders/cube.vert", ":/shaders/envMap.frag");

    GLint maxAttach = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);

    std::cout << "Max color attachments: " << maxAttach << std::endl;

    // Positions and UV coordinates to draw a fullscreen quad
    // (triangle strip, 4 vertices, position followed by UVs)
    std::vector<GLfloat> quadData = {-1.f, -1.f, 0.f, \
                                     0.f, 0.f,\
                                    -1.f, 1.f, 0.f, \
                                     0.f, 1.f, \
                                    1.f, -1.f, 0.f, \
                                     1.f, 0.f, \
                                    1.f, 1.f, 0.f,\
                                    1.f, 1.f};
    m_quad = std::make_unique<OpenGLShape>();
    m_quad->setVertexData(&quadData[0], quadData.size(), VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLE_STRIP, 4);
    m_quad->setAttribute(ShaderAttrib::POSITION, 3, 0, VBOAttribMarker::DATA_TYPE::FLOAT, false);
    m_quad->setAttribute(ShaderAttrib::TEXCOORD0, 2, 3*sizeof(GLfloat), VBOAttribMarker::DATA_TYPE::FLOAT, false);
    m_quad->buildVAO();

    // Environment map geo (if you want to draw just the env cube)
    std::vector<GLfloat> cubeData = CUBE_VERTEX_POSITIONS;
    m_envCube = std::make_unique<OpenGLShape>();
    m_envCube->setVertexData(&cubeData[0], cubeData.size(), VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLES, NUM_CUBE_VERTICES);
    m_envCube->setAttribute(ShaderAttrib::POSITION, 3, 0, VBOAttribMarker::DATA_TYPE::FLOAT, false);
    m_envCube->buildVAO();

    // Print the max FBO dimension.
    GLint maxRenderBufferSize;
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &maxRenderBufferSize);
    std::cout << "Max FBO size: " << maxRenderBufferSize << std::endl;

    // Textures list
    m_textures = std::make_unique<QMap<QString, QImage>>();

    // Load textures and normal maps
    QImage diffuse = View::loadTexture("../data/metal_diffuse.jpg");
    QImage normal = View::loadTexture("../data/metal_normal.jpg");
    QImage woodDiffuse = View::loadTexture("../data/wood_diffuse.jpg");
    QImage woodNormal = View::loadTexture("../data/wood_normal.jpg");
    QImage plasterDiffuse = View::loadTexture("../data/plaster_diffuse.jpg");
    QImage plasterNormal = View::loadTexture("../data/plaster_normal.jpg");

    glGenTextures(1, &m_diffuseID);
    glGenTextures(1, &m_normalID);
    glGenTextures(1, &m_woodDiffuseID);
    glGenTextures(1, &m_woodNormalID);
    glGenTextures(1, &m_plasterDiffuseID);
    glGenTextures(1, &m_plasterNormalID);
    View::buildTextureMap(diffuse, m_diffuseID);
    View::buildTextureMap(normal, m_normalID);
    View::buildTextureMap(woodDiffuse, m_woodDiffuseID);
    View::buildTextureMap(woodNormal, m_woodNormalID);
    View::buildTextureMap(plasterDiffuse, m_plasterDiffuseID);
    View::buildTextureMap(plasterNormal, m_plasterNormalID);

    // Load env cube textures and build env map
    QImage front1 = View::loadTexture("../data/negz.jpg");
    QImage back1 = View::loadTexture("../data/posz.jpg");
    QImage top1 = View::loadTexture("../data/posy.jpg");
    QImage bottom1 = View::loadTexture("../data/negy.jpg");
    QImage left1 = View::loadTexture("../data/negx.jpg");
    QImage right1 = View::loadTexture("../data/posx.jpg");

    QImage front2 = View::loadTexture("../data/negz1.jpg");
    QImage back2 = View::loadTexture("../data/posz1.jpg");
    QImage top2 = View::loadTexture("../data/posy1.jpg");
    QImage bottom2 = View::loadTexture("../data/negy1.jpg");
    QImage left2 = View::loadTexture("../data/negx1.jpg");
    QImage right2 = View::loadTexture("../data/posx1.jpg");

    glGenTextures(1, &m_envCubeID1);
    glGenTextures(1, &m_envCubeID2);
    View::buildEnvMap(front1, back1, top1, bottom1, left1, right1, m_envCubeID1);
    View::buildEnvMap(front2, back2, top2, bottom2, left2, right2, m_envCubeID2);
}

// Build a 2D texture map given a QImage type and a texture ID
void View::buildTextureMap(const QImage &image, GLuint textureID){
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Build a 3D texture cube given 6 Qimages (faces of the cube) and the texture handle
void View::buildEnvMap(const QImage &front, const QImage &back, const QImage &top, const QImage &bottom, const QImage &left, const QImage &right, GLuint textureHandle) {
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureHandle);

    // set image data as a side of the cube map
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, front.width(), front.width(), 0, GL_RGBA, GL_UNSIGNED_BYTE, front.bits());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, back.width(), back.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, back.bits());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, top.width(), top.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, top.bits());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, bottom.width(), bottom.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bottom.bits());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, left.width(), left.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, left.bits());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, right.width(), right.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, right.bits());

    // texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// Loads and returns a QImage type
// Storing it in m_textures (map)
QImage View::loadTexture(std::string path){
    QString string = QString::fromStdString(path);
    // check that string is valid and file exists
    // and map doesn't already have this file
    if (string.length() > 0 && View::textureExists(string) && !m_textures->contains(string)){
        QImage image = QImage(string);
        m_textures->insert(string, image);
        assert(!image.isNull());
        std::cout << "Loaded texture: " << path << std::endl;
        return image;
    }
    std::cout << "Failed to load texture: " << path << std::endl;
    return QImage();
}

// Helper function to verify texture file exists
bool View::textureExists(QString filePath){
    QFileInfo file = QFileInfo(filePath);
    bool out = false;
    if (file.exists() && file.isFile()){
        out = true;
    }
    return out;
}

// Helper to scale range to range
float View::scale(float oldMin, float oldMax, float newMin, float newMax, float val){
    float oldRange = oldMax - oldMin;
    float newRange = newMax - newMin;
    float newVal = ((val - oldMin) * newRange / oldRange) + newMin;
    return newVal;
}

// Repaints the canvas. Called 60 times per second.
// Inherited from GLWidget type
void View::tick()
{
    update();
}

// The main drawing call
void View::paintGL() {
    m_increment++; // always increment time
    glClear(GL_COLOR_BUFFER_BIT);
    drawRayScene();
}

// For fun
void View::drawEnvCube(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthMask(GL_FALSE);
    glUseProgram(m_envCubeProgram);
    glm::mat4 cubeView = glm::mat4(glm::mat3(m_view));
    glUniformMatrix4fv(glGetUniformLocation(m_envCubeProgram, "view"), 1, GL_FALSE, glm::value_ptr(cubeView));
    glUniformMatrix4fv(glGetUniformLocation(m_envCubeProgram, "projection"), 1, GL_FALSE, glm::value_ptr(m_projection));
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeID1);
    m_envCube->draw();
    glDepthMask(GL_TRUE);
}

// The main ray draw call
// Ping pongs between two FBO's storing the current render and the previous render
// The ray program will sample the bound prevFBO and blend it with the next render
// via the numPasses as a compositing weight
// the ray program will write to the nextFBO (to become the prevFBO) and also draw to the screen
void View::drawRayScene() {

    auto prevFBO = m_evenPass ? m_rayFBO1 : m_rayFBO2;
    auto nextFBO = m_evenPass ? m_rayFBO2 : m_rayFBO1;
    float firstPass = m_firstPass ? 1.0f : 0.0f;

    // time in seconds, absolute time
    float time = m_increment / static_cast<float>(m_fps);
    // time in seconds for animation. Separately tracked from time to allow for starting/stoping animation
    float animationTime = m_animationIncrement / static_cast<float>(m_fps);

    // If animation is on, incremnet m_animationTime
    if (settings.useAnimation){
        m_animationIncrement++;
    }

    // Bind nextFBO
    // Move render from prevFBO to nextFBO while compositing with current render
    nextFBO->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, m_width, m_height);

    glUseProgram(m_rayProgram);

    // Bind the previous render's fragColor as a sampler for this render
    glActiveTexture(GL_TEXTURE0);
    prevFBO->getColorAttachment(0).bind();

    // with animation
    glm::mat4x4 inverseCam = glm::inverse(m_view) * glm::inverse(m_scale);

    // ---------------- RAY DATA -----------------
    glUniform1i(glGetUniformLocation(m_rayProgram, "prev"), 0);
    glUniform1f(glGetUniformLocation(m_rayProgram, "firstPass"), firstPass);
    glUniform1i(glGetUniformLocation(m_rayProgram, "numPasses"), m_numPasses);

    glUniform2f(glGetUniformLocation(m_rayProgram, "dimensions"), static_cast<float>(m_width), static_cast<float>(m_height));
    glUniformMatrix4fv(glGetUniformLocation(m_rayProgram, "inverseCam"), 1, false, glm::value_ptr(inverseCam));
    glUniform1f(glGetUniformLocation(m_rayProgram, "time"), time);

    // ---------------- GLOBAL DATA -----------------
    glUniform1f(glGetUniformLocation(m_rayProgram, "globalData.ka"), globalData.ka);
    glUniform1f(glGetUniformLocation(m_rayProgram, "globalData.kd"), globalData.kd);
    glUniform1f(glGetUniformLocation(m_rayProgram, "globalData.ks"), globalData.ks);
    glUniform1f(glGetUniformLocation(m_rayProgram, "globalData.kt"), globalData.kt);

    // ---------------- TEXTURE DATA -----------------

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_diffuseID);
    glUniform1i(glGetUniformLocation(m_rayProgram, "metalDiffuseTex"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_normalID);
    glUniform1i(glGetUniformLocation(m_rayProgram, "metalNormalTex"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_woodDiffuseID);
    glUniform1i(glGetUniformLocation(m_rayProgram, "woodDiffuseTex"), 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_woodNormalID);
    glUniform1i(glGetUniformLocation(m_rayProgram, "woodNormalTex"), 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, m_plasterDiffuseID);
    glUniform1i(glGetUniformLocation(m_rayProgram, "plasterDiffuseTex"), 5);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, m_plasterNormalID);
    glUniform1i(glGetUniformLocation(m_rayProgram, "plasterNormalTex"), 6);

    // ---------------- ENVIRONMENT MAP --------------

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, View::getEnvMap(settings.modeScene));
    glUniform1i(glGetUniformLocation(m_rayProgram, "envMap"), 7);

    // ---------------- LIGHT(S) ------------------

    // Point light (pos no dir)
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject1.color"), 1, glm::value_ptr(lightObject1.color));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject1.pos"), 1, glm::value_ptr(lightObject1.pos));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject1.dir"), 1, glm::value_ptr(lightObject1.dir));
    glUniform3fv(glGetUniformLocation(m_rayProgram, "lightObject1.function"), 1, glm::value_ptr(lightObject1.function));
    glUniform1i(glGetUniformLocation(m_rayProgram, "lightObject1.type"), static_cast<int>(lightObject1.type));

    // Point light (pos no dir)
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject2.color"), 1, glm::value_ptr(lightObject2.color));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject2.pos"), 1, glm::value_ptr(lightObject2.pos));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject2.dir"), 1, glm::value_ptr(lightObject2.dir));
    glUniform3fv(glGetUniformLocation(m_rayProgram, "lightObject2.function"), 1, glm::value_ptr(lightObject2.function));
    glUniform1i(glGetUniformLocation(m_rayProgram, "lightObject2.type"), static_cast<int>(lightObject2.type));

    // Directional light (dir no pos)
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject3.color"), 1, glm::value_ptr(lightObject3.color));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject3.pos"), 1, glm::value_ptr(lightObject3.pos));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "lightObject3.dir"), 1, glm::value_ptr(lightObject3.dir));
    glUniform3fv(glGetUniformLocation(m_rayProgram, "lightObject3.function"), 1, glm::value_ptr(lightObject3.function));
    glUniform1i(glGetUniformLocation(m_rayProgram, "lightObject3.type"), static_cast<int>(lightObject3.type));

    // ---------------- SETTINGS DATA ------------------
    // parse and send the UI's lightIntensity settings as floats [0.0, 1.0]
    float scaledl1Intensity = View::scale(0.f, 100.f, 0.f, 1.f, settings.l1Intensity);
    float scaledl2Intensity = View::scale(0.f, 100.f, 0.f, 1.f, settings.l2Intensity);
    float scaledl3Intensity = View::scale(0.f, 100.f, 0.f, 1.f, settings.l3Intensity);

    glUniform1f(glGetUniformLocation(m_rayProgram, "settings.l1Intensity"), static_cast<float>(scaledl1Intensity));
    glUniform1f(glGetUniformLocation(m_rayProgram, "settings.l2Intensity"), static_cast<float>(scaledl2Intensity));
    glUniform1f(glGetUniformLocation(m_rayProgram, "settings.l3Intensity"), static_cast<float>(scaledl3Intensity));

    // lighting equation
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useAmbient"), static_cast<int>(settings.useAmbient));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useDiffuse"), static_cast<int>(settings.useDiffuse));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useSpecular"), static_cast<int>(settings.useSpecular));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useShadows"), static_cast<int>(settings.useShadows));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useReflections"), static_cast<int>(settings.useReflections));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useTextures"), static_cast<int>(settings.useTextures));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useStochastic"), static_cast<int>(settings.useStochastic));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useTextures"), static_cast<int>(settings.useTextures));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useNM"), static_cast<int>(settings.useNM));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useAO"), static_cast<int>(settings.useAO));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useDOF"), static_cast<int>(settings.useDOF));
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.aperture"), settings.aperture);
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.focalLength"), settings.focalLength);
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.numSamples"), settings.numSamples);
    glUniform1i(glGetUniformLocation(m_rayProgram, "settings.useEnvironment"), static_cast<int>(settings.useEnvironment));

    // ---------------- SCENE OBJECT(S) ------------------
    std::vector<SceneObject> scene = SceneBuilder::getScene(animationTime);

    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject1.primitive"), static_cast<int>(scene[0].primitive));
    glUniformMatrix4fv(glGetUniformLocation(m_rayProgram, "sceneObject1.objectToWorld"), 1, false, glm::value_ptr(scene[0].objectToWorld));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject1.cDiffuse"), 1, glm::value_ptr(scene[0].cDiffuse));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject1.cAmbient"), 1, glm::value_ptr(scene[0].cAmbient));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject1.cSpecular"), 1, glm::value_ptr(scene[0].cSpecular));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject1.cReflective"), 1, glm::value_ptr(scene[0].cReflective));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject1.shininess"), scene[0].shininess);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject1.blend"), scene[0].blend);
    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject1.texID"), static_cast<int>(scene[0].texID));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject1.repeatU"), scene[0].repeatU);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject1.repeatV"), scene[0].repeatV);

    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject2.primitive"), static_cast<int>(scene[1].primitive));
    glUniformMatrix4fv(glGetUniformLocation(m_rayProgram, "sceneObject2.objectToWorld"), 1, false, glm::value_ptr(scene[1].objectToWorld));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject2.cDiffuse"), 1, glm::value_ptr(scene[1].cDiffuse));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject2.cAmbient"), 1, glm::value_ptr(scene[1].cAmbient));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject2.cSpecular"), 1, glm::value_ptr(scene[1].cSpecular));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject2.cReflective"), 1, glm::value_ptr(scene[1].cReflective));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject2.shininess"), scene[1].shininess);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject2.blend"), scene[1].blend);
    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject2.texID"), static_cast<int>(scene[1].texID));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject2.repeatU"), scene[1].repeatU);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject2.repeatV"), scene[1].repeatV);

    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject3.primitive"), static_cast<int>(scene[2].primitive));
    glUniformMatrix4fv(glGetUniformLocation(m_rayProgram, "sceneObject3.objectToWorld"), 1, false, glm::value_ptr(scene[2].objectToWorld));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject3.cDiffuse"), 1, glm::value_ptr(scene[2].cDiffuse));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject3.cAmbient"), 1, glm::value_ptr(scene[2].cAmbient));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject3.cSpecular"), 1, glm::value_ptr(scene[2].cSpecular));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject3.cReflective"), 1, glm::value_ptr(scene[2].cReflective));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject3.shininess"), scene[2].shininess);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject3.blend"), scene[2].blend);
    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject3.texID"), static_cast<int>(scene[2].texID));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject3.repeatU"), scene[2].repeatU);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject3.repeatV"), scene[2].repeatV);

    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject4.primitive"), static_cast<int>(scene[3].primitive));
    glUniformMatrix4fv(glGetUniformLocation(m_rayProgram, "sceneObject4.objectToWorld"), 1, false, glm::value_ptr(scene[3].objectToWorld));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject4.cDiffuse"), 1, glm::value_ptr(scene[3].cDiffuse));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject4.cAmbient"), 1, glm::value_ptr(scene[3].cAmbient));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject4.cSpecular"), 1, glm::value_ptr(scene[3].cSpecular));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject4.cReflective"), 1, glm::value_ptr(scene[3].cReflective));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject4.shininess"), scene[3].shininess);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject4.blend"), scene[3].blend);
    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject4.texID"), static_cast<int>(scene[3].texID));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject4.repeatU"), scene[3].repeatU);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject4.repeatV"), scene[3].repeatV);

    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject5.primitive"), static_cast<int>(scene[4].primitive));
    glUniformMatrix4fv(glGetUniformLocation(m_rayProgram, "sceneObject5.objectToWorld"), 1, false, glm::value_ptr(scene[4].objectToWorld));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject5.cDiffuse"), 1, glm::value_ptr(scene[4].cDiffuse));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject5.cAmbient"), 1, glm::value_ptr(scene[4].cAmbient));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject5.cSpecular"), 1, glm::value_ptr(scene[4].cSpecular));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject5.cReflective"), 1, glm::value_ptr(scene[4].cReflective));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject5.shininess"), scene[4].shininess);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject5.blend"), scene[4].blend);
    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject5.texID"), static_cast<int>(scene[4].texID));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject5.repeatU"), scene[4].repeatU);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject5.repeatV"), scene[4].repeatV);

    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject6.primitive"), static_cast<int>(scene[5].primitive));
    glUniformMatrix4fv(glGetUniformLocation(m_rayProgram, "sceneObject6.objectToWorld"), 1, false, glm::value_ptr(scene[5].objectToWorld));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject6.cDiffuse"), 1, glm::value_ptr(scene[5].cDiffuse));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject6.cAmbient"), 1, glm::value_ptr(scene[5].cAmbient));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject6.cSpecular"), 1, glm::value_ptr(scene[5].cSpecular));
    glUniform4fv(glGetUniformLocation(m_rayProgram, "sceneObject6.cReflective"), 1, glm::value_ptr(scene[5].cReflective));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject6.shininess"), scene[5].shininess);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject6.blend"), scene[5].blend);
    glUniform1i(glGetUniformLocation(m_rayProgram, "sceneObject6.texID"), static_cast<int>(scene[5].texID));
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject6.repeatU"), scene[5].repeatU);
    glUniform1f(glGetUniformLocation(m_rayProgram, "sceneObject6.repeatV"), scene[5].repeatV);

    // draw  full screen quad
    m_quad->draw();
    glUseProgram(0);

    // Now draw the particles from nextFBO
    nextFBO->unbind();
    glUseProgram(m_compositeProgram);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, m_width, m_height);

    glActiveTexture(GL_TEXTURE0);
    nextFBO->getColorAttachment(0).bind();
    glUniform1i(glGetUniformLocation(m_compositeProgram, "tex"), 0);

    m_quad->draw();
    glUseProgram(0);

    m_numPasses += 1;
    m_firstPass = false;
    m_evenPass = !m_evenPass;
}

// This is called at the beginning of the program between initializeGL and
// the first paintGL call, as well as every time the window is resized.
void View::resizeGL(int w, int h) {
    m_width = w;
    m_height = h;
    // Initialize FBOs here, with dimensions m_width and m_height.
    // Pass in TextureParameters::WRAP_METHOD::CLAMP_TO_EDGE as the last parameter

    View::clearPasses();
    View::rebuildMatrices();
}

// Mouse interaction code below.
void View::mousePressEvent(QMouseEvent *event) {
    m_prevMousePos = event->pos();
    View::clearPasses();
}

void View::mouseMoveEvent(QMouseEvent *event) {
    m_angleX += 3 * (event->x() - m_prevMousePos.x()) / (float) width();
    m_angleY += 3 * (event->y() - m_prevMousePos.y()) / (float) height();
    m_prevMousePos = event->pos();
    View::rebuildMatrices();
    View::clearPasses();
}

void View::wheelEvent(QWheelEvent *event) {
    m_zoom -= event->delta() / 100.f;
    View::rebuildMatrices();
    View::clearPasses();
}

// Rebuild matrices for camera
void View::rebuildMatrices() {
    m_view = glm::translate(glm::vec3(0, 0, -m_zoom)) *
             glm::rotate(m_angleY, glm::vec3(1.f,0.f,0.f)) *
             glm::rotate(m_angleX, glm::vec3(0.f,1.f,0.f));

    float farPlane = std::max(CAMERA_FAR, CAMERA_NEAR + 100.f * FLT_EPSILON);
    float h = farPlane * glm::tan(glm::radians(CAMERA_FOV/2));
    float aspectRatio = m_width/m_height;
    float w = aspectRatio * h;
    m_scale = glm::scale(glm::vec3(1.f/w, 1.f/h, 1.f/farPlane));
    m_projection = glm::perspective(0.8f, (float)width()/height(), 0.1f, 100.f);
    update();
}

// Get the current env map depending on scene mode
GLuint View::getEnvMap(int modeScene)
{
    if (modeScene == 0) {
        return m_envCubeID2;
    } else {
        return m_envCubeID1;
    }
}

// Clear out the current number of passes
// Called whenever settings are changed or camera moves
void View::clearPasses(){
    m_numPasses = 0.f;
    m_firstPass = true;
    m_rayFBO1 = std::make_shared<FBO>(1, FBO::DEPTH_STENCIL_ATTACHMENT::NONE, m_width, m_height, TextureParameters::WRAP_METHOD::CLAMP_TO_EDGE, TextureParameters::FILTER_METHOD::NEAREST);//, GL_FLOAT);
    m_rayFBO2 = std::make_shared<FBO>(1, FBO::DEPTH_STENCIL_ATTACHMENT::NONE, m_width, m_height, TextureParameters::WRAP_METHOD::CLAMP_TO_EDGE, TextureParameters::FILTER_METHOD::NEAREST);//, GL_FLOAT);
}

// View::settingsChanged
// Called when settings are changed on the UI
void View::settingsChanged() {
    // Upon settings changed, reset numPasses to 0 and firstPass to true
    View::clearPasses();
}
