#ifndef VIEW_H
#define VIEW_H
#include "GL/glew.h"
#include <QGLWidget>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QImage>
#include <QFileInfo>
#include <QMap>
#include <QRgb>

#include "glm/glm.hpp"            // glm::vec*, mat*, and basic glm functions
#include "glm/gtx/transform.hpp"  // glm::translate, scale, rotate
#include "glm/gtc/type_ptr.hpp"   // glm::value_ptr

#include <memory>  // std::unique_ptr

#include "gl/datatype/FBO.h"

class OpenGLShape;

using namespace CS123::GL;

// Primitive and Light type both in same enums
// so #define SPHERE, CUBE, CONE, etc. in ray.frag are ensured
// to not conflict
enum class ShapeType{
    SPHERE,
    CUBE,
    CONE,
    CYLINDER,
    NO_INTERSECT,
    LIGHT_POINT,
    LIGHT_DIRECTIONAL
};

// [DATA TYPES]
////////////////////////////////////////////////////////////////////////
// Final output of data Raytracing spits out to write to color attachments
struct RayData{
    float rayT;
    int rayPrimitiveType;
    glm::vec4 rayNormal;
    glm::vec4 rayPoint;
    glm::vec4 rayDirection;
};

struct GlobalData{
    float ka; // global ambient coefficient
    float kd; // global diffuse coefficeint
    float ks; // global specular coefficient
    float kt; // global transparency coefficient
};

// Light Data
// Just point lights at the moment
struct LightObject{
    glm::vec4 color;
    glm::vec4 pos; // Only applicable for point lights
    glm::vec4 dir; // Only applicable for directional lights
    glm::vec3 function; // attenuation function
    ShapeType type; // Can be LIGHT_POINT, LIGHT_DIRECTIONAL
};

// [SCENE]
//////////////////////////////////////////

// [ENVIRONMENT CUBE MAPS] (via)
// http://www.humus.name/index.php?page=Textures

// Harcoded data
// GlobalData, LightData (1 per scene atm)
// SceneObjects [5 per scene atm]
const GlobalData globalData = {0.5f, 0.5f, 0.5f, 0.0f};

// Light Object 1
const LightObject lightObject1 = {glm::vec4(0.8, 0.8, 0.8, 1.0), glm::vec4(0.0, 0.0, 10.0, 1.0), glm::vec4(0.f), glm::vec3(0.9, 0.0, 0.0), ShapeType::LIGHT_POINT};

// Light Object 2
const LightObject lightObject2 = {glm::vec4(0.2, 0.2, 0.2, 1.0), glm::vec4(-10.0, 0.0, -10.0, 1.0), glm::vec4(0.f), glm::vec3(0.9, 0.0, 0.0), ShapeType::LIGHT_POINT};

// Light Object 3
const LightObject lightObject3 = {glm::vec4(0.3, 0.3, 0.2, 1.0), glm::vec4(0.0, 8.0, 10.0, 1.0), glm::vec4(0.f), glm::vec3(0.5, 0.0, 0.0), ShapeType::LIGHT_POINT};

struct SceneObject;

class View : public QGLWidget {
    Q_OBJECT

public:
    View(QGLFormat format, QWidget *parent = 0);
    ~View();
    void settingsChanged();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *e);

protected slots:
    /** Repaints the canvas. Called 60 times per second by m_timer. */
    void tick();

private:
    void drawRayScene();
    void drawEnvCube();

    float scale(float oldMin, float oldMax, float newMin, float newMax, float val);

    void rebuildMatrices();
    void clearPasses();
    GLuint getEnvMap(int modeScene);

    // Texture mapping
    void buildEnvMap(const QImage &front, const QImage &back, const QImage &top, const QImage &bottom, const QImage &left, const QImage &right, GLuint textureHandle);
    void buildTextureMap(const QImage &image, GLuint textureID);

    std::unique_ptr<QMap<QString, QImage>> m_textures; //pointer to a map of QString, QImage pairs
    bool textureExists(QString filePath);
    QImage loadTexture(std::string string);

    int m_width;
    int m_height;

    GLuint m_phongProgram;
    GLuint m_textureProgram;
    GLuint m_rayProgram;
    GLuint m_compositeProgram;
    GLuint m_envCubeProgram;

    GLuint m_envCubeID1;
    GLuint m_envCubeID2;

    GLuint m_diffuseID;
    GLuint m_normalID;

    GLuint m_plasterDiffuseID;
    GLuint m_plasterNormalID;

    GLuint m_woodDiffuseID;
    GLuint m_woodNormalID;

    std::unique_ptr<OpenGLShape> m_quad;
    std::unique_ptr<OpenGLShape> m_envCube;
    std::unique_ptr<OpenGLShape> m_square;

    std::shared_ptr<FBO> m_rayFBO1;
    std::shared_ptr<FBO> m_rayFBO2;
    bool m_firstPass;
    bool m_evenPass;
    int m_numPasses;

    glm::mat4 m_view, m_projection, m_scale;

    /** For mouse interaction. */
    float m_angleX, m_angleY, m_zoom;
    QPoint m_prevMousePos;

    /** Timer calls tick() 60 times per second. */
    QTimer m_timer;
    float m_fps;

    /** Incremented on every call to paintGL. */
    int m_increment;
    int m_animationIncrement; // used to track the last animation time (used for paused scenes)

    // Camera setting constants
    const float CAMERA_FAR = 50.f;
    const float CAMERA_NEAR = 0.1f;
    const float CAMERA_FOV = 45.f;
};

struct SceneObject{
    ShapeType primitive; // Can be SPHERE, CUBE, CONE, CYLINDER
    glm::mat4x4 objectToWorld; // cumulative transformation matrix
    glm::vec4 cDiffuse;
    glm::vec4 cAmbient;
    glm::vec4 cSpecular;
    glm::vec4 cReflective;
    float shininess;

    // Texture properties
    float blend;
    int texID;
    float repeatU;
    float repeatV;
};

#endif // VIEW_H
