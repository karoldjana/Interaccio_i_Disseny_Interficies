#define GLM_FORCE_RADIANS
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "model.h"

class MyGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core 
{
  Q_OBJECT

  public:
    MyGLWidget (QWidget *parent=0);
    ~MyGLWidget ();

  protected:
    // initializeGL - Aqui incluim les inicialitzacions del contexte grafic.
    virtual void initializeGL ( );
    // paintGL - Mètode cridat cada cop que cal refrescar la finestra.
    // Tot el que es dibuixa es dibuixa aqui.
    virtual void paintGL ( );
    // resizeGL - És cridat quan canvia la mida del widget
    virtual void resizeGL (int width, int height);
    // keyPressEvent - Es cridat quan es prem una tecla
    virtual void keyPressEvent (QKeyEvent *event);
    // mouse{Press/Release/Move}Event - Són cridades quan es realitza l'event 
    // corresponent de ratolí
    virtual void mousePressEvent (QMouseEvent *event);
    virtual void mouseReleaseEvent (QMouseEvent *event);
    virtual void mouseMoveEvent (QMouseEvent *event);

  private:
    void createBuffersModel ();
    void createBuffersPilota ();
    void createBuffersPorteria ();
    void createBuffersTerra ();
    void carregaShaders ();
    void iniEscena ();
    void iniCamera ();
    void iniIlum ();
    void projectTransform ();
    void viewTransform ();
    void modelTransformIdent ();
    void modelTransformModel ();
    void modelTransformModel2 ();
    void modelTransformPilota ();
    void calculaCapsaModel (Model &p, float &escala, glm::vec3 &CentreBase);

    void mouPilota (int alfa);
    void iniciPilota ();
    void tractamentGol();
    bool aturaPorter();

    // VAO i VBO names
    GLuint VAO_Patr, VAO_Pil, VAO_Port, VAO_Terra;
    // Program
    QOpenGLShaderProgram *program;
    // uniform locations
    GLuint transLoc, projLoc, viewLoc;
    // attribute locations
    GLuint vertexLoc, normalLoc, matambLoc, matdiffLoc, matspecLoc, matshinLoc, colFocusLoc;

    // model
    Model patr, pil;
    // paràmetres calculats a partir de la capsa contenidora del model
    glm::vec3 centreBaseModel, centreBasePil, vrp, minEsc, maxEsc;
    float escalaModel, escalaPil, fov, fovini;
    // radi de l'escena
    float radiEsc;
    glm::vec3 posPilota, colFocus, posPort;

    typedef  enum {NONE, ROTATE} InteractiveAction;
    InteractiveAction DoingInteractive;
    int xClick, yClick;
    float angleY, angleX, zn, zf,ra;
    bool primerap;

    // per al moviment de la pilota
    QTimer timer;
    int anglePilota;

  public slots:
    void mourePilota ();
    void canvicam ();
    void canviang(int anglePilota);
    void reset();
    
    signals:
    void sendsign(int ang);
    void sendres (bool bol);
};

