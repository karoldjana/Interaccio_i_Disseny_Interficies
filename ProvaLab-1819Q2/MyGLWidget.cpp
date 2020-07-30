#include "MyGLWidget.h"

#include <iostream>

MyGLWidget::MyGLWidget (QWidget* parent) : QOpenGLWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);  // per rebre events de teclat
  xClick = yClick = 0;
  connect(&timer, SIGNAL(timeout()), this, SLOT(mourePilota()));  // per al moviment de la pilota
  DoingInteractive = NONE;
}

MyGLWidget::~MyGLWidget ()
{
  if (program != NULL)
    delete program;
}

void MyGLWidget::initializeGL ()
{
  // Cal inicialitzar l'ús de les funcions d'OpenGL
  initializeOpenGLFunctions();  

  glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (d'esborrat)
  glEnable(GL_DEPTH_TEST);
  carregaShaders();
  creaBuffersModel();
  creaBuffersPilota();
  creaBuffersCub();
  creaBuffersTerra();

  iniEscena();
  iniCamera();
}

void MyGLWidget::posicionafocus() {
	if(focEsc) posFocus = glm::vec3(0,10,0);
    else posFocus = glm::vec3(0,0,0);
    glUniform1i(focEscLoc, int(focEsc));
    glUniform3fv(posFocusLoc, 1, &posFocus[0]);
}

void MyGLWidget::iniEscena ()
{
  minEsc = glm::vec3(-10, 0 , -7);
  maxEsc = glm::vec3(10, 4, 7);  
  centreEsc = (minEsc+maxEsc)/2.f;
  radiEsc = glm::distance(minEsc, maxEsc)/2.f;

  posPorter = glm::vec3(9.0, 0.0, 0.0);  // posició inicial del porter
  posPilota = glm::vec3(7.0, 0.0, 0.0);  // posició inicial de la pilota
  dirInicialPilota();    // direcció inicial de la pilota
  focEsc = false;
}

void MyGLWidget::iniCamera ()
{
  angleY = M_PI/3.0;
  angleX = 0;
  ra = 1.f;
  fov = 2.0 * asin(radiEsc/(radiEsc*2));
  fovini = fov;
  segcam = false;
  projectTransform ();
  viewTransform ();
}

void MyGLWidget::paintGL () 
{
// Aquest codi és necessari únicament per a MACs amb pantalla retina.
#ifdef __APPLE__
  GLint vp[4];
  glGetIntegerv (GL_VIEWPORT, vp);
  ample = vp[2];
  alt = vp[3];
#endif

// En cas de voler canviar els paràmetres del viewport, descomenteu la crida següent i
// useu els paràmetres que considereu (els que hi ha són els de per defecte)
//  glViewport (0, 0, ample, alt);
  
  // Esborrem el frame-buffer i el depth-buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  projectTransform();
  viewTransform();

  // Activem el VAO per a pintar el terra 
  glBindVertexArray (VAO_Terra);

  modelTransformIdent ();
  // pintem terra
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Activem el VAO per a pintar el cub
  glBindVertexArray (VAO_Cub);

  modelTransformCub ();
  // pintem cub
  glDrawArrays(GL_TRIANGLES, 0, 36);
  
  glBindVertexArray (VAO_Cub);

  modelTransformCub2 ();
  // pintem cub
  glDrawArrays(GL_TRIANGLES, 0, 36);
  
  glBindVertexArray (VAO_Cub);

  modelTransformCub3 ();
  // pintem cub
  glDrawArrays(GL_TRIANGLES, 0, 36);

  // Activem el VAO per a pintar el Patricio
  glBindVertexArray (VAO_Patr);

  modelTransformModel ();
  // Pintem Patricio
  glDrawArrays(GL_TRIANGLES, 0, patr.faces().size()*3);
  
  glBindVertexArray (VAO_Pil);

  modelTransformPilota ();
  // Pintem Pilota
  glDrawArrays(GL_TRIANGLES, 0, pil.faces().size()*3);
  
  glBindVertexArray(0);
}

void MyGLWidget::resizeGL (int w, int h) 
{
  ample = w;
  alt = h;
  ra = float(ample)/float(alt);
  if(ra >= 1) {
	  fov = fovini;
  }
  else {
	  fov = 2.0 * atan(tan(fovini/2.0)/ra);
  }
}

void MyGLWidget::modelTransformModel ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  TG = glm::translate(TG, posPorter);
  TG = glm::rotate(TG, float(-M_PI/2), glm::vec3(0, 1, 0));
  TG = glm::scale(TG, glm::vec3(4*escalaModel, 4*escalaModel, 4*escalaModel));
  TG = glm::translate(TG, -centreBaseModel);
  
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::modelTransformPilota ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  TG = glm::translate(TG, posPilota);
  TG = glm::scale(TG, glm::vec3(escalaPil, escalaPil, escalaPil));
  TG = glm::translate(TG, -centreBasePil);
  
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::modelTransformIdent ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::modelTransformCub ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  TG = glm::translate(TG, glm::vec3(0,0,-6.9));
  TG = glm::scale(TG, glm::vec3(20, 2, 0.2));
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
  }

void MyGLWidget::modelTransformCub2 ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  TG = glm::translate(TG, glm::vec3(0,0,6.9));
  TG = glm::scale(TG, glm::vec3(20, 2, 0.2));
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::modelTransformCub3 ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  TG = glm::translate(TG, glm::vec3(-9.9,0,0));
  TG = glm::scale(TG, glm::vec3(0.2, 2, 14));
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
 }

void MyGLWidget::projectTransform ()
{
  glm::mat4 Proj;  // Matriu de projecció
  if (!segcam) Proj = glm::perspective(fov, ra, radiEsc, 3.0f*radiEsc);
  else Proj = glm::perspective(float(M_PI/2), ra, float(0.01), 100.f);
  glUniformMatrix4fv (projLoc, 1, GL_FALSE, &Proj[0][0]);
}

void MyGLWidget::viewTransform ()
{
  glm::mat4 View(1.f);  // Matriu de posició i orientació
  if(!segcam) {
	  View = glm::translate(View, glm::vec3(0, 0, -2*radiEsc));
	  View = glm::rotate(View, angleX, glm::vec3(1, 0, 0));
	  View = glm::rotate(View, -angleY, glm::vec3(0, 1, 0));
	  View = glm::translate(View, -centreEsc);
  }
  else {
	  View = glm::lookAt(glm::vec3(0,18,0), glm::vec3(0,0,0), glm::vec3(-1,0,0));
  }
  glUniformMatrix4fv (viewLoc, 1, GL_FALSE, &View[0][0]);
}

void MyGLWidget::iniciPilota()
{
  timer.stop();
  posPilota = glm::vec3(7.0, 0.0, 0.0);
  dirInicialPilota();    // direcció inicial de la pilota
}

void MyGLWidget::dirInicialPilota()    // No cal modificar aquest métode
{
  int randNum = rand() % 90 + 1;
  randNum -= 45;
  float rad = randNum * M_PI / 180.0f;
  dirPilota[0] = -cos(rad);
  dirPilota[1] = 0;
  dirPilota[2] = sin(rad);
}

void MyGLWidget::mourePilota()  // No cal modificar aquest métode
{
  makeCurrent();
  timer.start(100);
  mouPilota(dirPilota);
  if (posPilota[0] >= 10.0)  // ens l'han colat
    tractamentGol();
}

void MyGLWidget::mouPilota(glm::vec3 dir)  // No cal modificar aquest métode
{
  posPilota += dir;

  rebotaParets();
  if (rebotaPorter())
    canviaDireccio();
  else  
    if (posPilota[0] <= -10.0)
    {
      posPilota[0] = -11.0;
      timer.stop();
    }
  update();
}

void MyGLWidget::rebotaParets() 
{
  if (posPilota.z <= -6.3 || posPilota.z >= 6.3){
      dirPilota.z = -dirPilota.z;
  }
  else if (posPilota.x <= -9.3){
      dirPilota.x = -dirPilota.x;
  }
} 

void MyGLWidget::tractamentGol()
{
  timer.stop();
  //  Exercici 2-c): Falta el tractament que cal quan ens fan gol
} 

bool MyGLWidget::rebotaPorter()     // No cal modificar aquest métode
{
  return ((posPilota[0] > 7.5) && (posPilota[0] < 8.5) && 
          (posPilota[2] >= posPorter[2]-1.5) && 
          (posPilota[2] <= posPorter[2]+1.5));
}

void MyGLWidget::canviaDireccio()     // No cal modificar aquest métode
{
  // Inventem una nova direcció quan la pilota rebota amb el porter
  dirPilota = posPilota-posPorter;
} 

void MyGLWidget::keyPressEvent(QKeyEvent* event) 
{
  makeCurrent();
  switch (event->key()) {
    case Qt::Key_Up: { // moviment de la pilota
      if (posPilota[0] == 7.0)
        mourePilota ();
      break;
    }
    case Qt::Key_I: { // reinicia posició pilota
      iniciPilota ();
      break;
    }
    case Qt::Key_F: { // reinicia posició pilota
      focEsc = !focEsc;
      posicionafocus();
      break;
    }
    case Qt::Key_S: { // reinicia posició pilota
      segcam = !segcam;
      break;
    }
    default: event->ignore(); break;
  }
  update();
}

void MyGLWidget::mousePressEvent (QMouseEvent *e)
{
  xClick = e->x();
  yClick = e->y();

  if (e->button() & Qt::LeftButton &&
      ! (e->modifiers() & (Qt::ShiftModifier|Qt::AltModifier|Qt::ControlModifier)))
  {
    DoingInteractive = ROTATE;
  }
}

void MyGLWidget::mouseReleaseEvent( QMouseEvent *)
{
  DoingInteractive = NONE;
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *e)
{
  makeCurrent();
  if (DoingInteractive == ROTATE)
  {
    // Fem la rotació
    angleY += (e->x() - xClick) * M_PI / 180.0;
    angleX -= (e->y() - yClick) * M_PI / 180.0;
    viewTransform ();
  }

  xClick = e->x();
  yClick = e->y();

  update ();
}

void MyGLWidget::calculaCapsaModel (Model &p, float &escala, glm::vec3 &centreBase)
{
  // Càlcul capsa contenidora i valors transformacions inicials
  float minx, miny, minz, maxx, maxy, maxz;
  minx = maxx = p.vertices()[0];
  miny = maxy = p.vertices()[1];
  minz = maxz = p.vertices()[2];
  for (unsigned int i = 3; i < p.vertices().size(); i+=3)
  {
    if (p.vertices()[i+0] < minx)
      minx = p.vertices()[i+0];
    if (p.vertices()[i+0] > maxx)
      maxx = p.vertices()[i+0];
    if (p.vertices()[i+1] < miny)
      miny = p.vertices()[i+1];
    if (p.vertices()[i+1] > maxy)
      maxy = p.vertices()[i+1];
    if (p.vertices()[i+2] < minz)
      minz = p.vertices()[i+2];
    if (p.vertices()[i+2] > maxz)
      maxz = p.vertices()[i+2];
  }
  escala = 1.0/(maxy-miny);
  centreBase[0] = (minx+maxx)/2.0; centreBase[1] = miny; centreBase[2] = (minz+maxz)/2.0;
}

void MyGLWidget::creaBuffersModel ()
{
  // Carreguem el model de l'OBJ - Atenció! Abans de crear els buffers!
  patr.load("./models/legoman.obj");

  // Calculem la capsa contenidora del model
  calculaCapsaModel (patr, escalaModel, centreBaseModel);
  
  // Creació del Vertex Array Object del Patricio
  glGenVertexArrays(1, &VAO_Patr);
  glBindVertexArray(VAO_Patr);

  // Creació dels buffers del model patr
  GLuint VBO_Patr[6];
  // Buffer de posicions
  glGenBuffers(6, VBO_Patr);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Patr[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_vertices(), GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  // Buffer de normals
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Patr[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_normals(), GL_STATIC_DRAW);

  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Patr[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_matamb(), GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Patr[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_matdiff(), GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Patr[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_matspec(), GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Patr[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3, patr.VBO_matshin(), GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  glBindVertexArray(0);
}

void MyGLWidget::creaBuffersPilota ()
{
  // Carreguem el model de l'OBJ - Atenció! Abans de crear els buffers!
  pil.load("./models/Pilota.obj");

  // Calculem la capsa contenidora del model
  calculaCapsaModel (pil, escalaPil, centreBasePil);
  
  // Creació del Vertex Array Object del Patricio
  glGenVertexArrays(1, &VAO_Pil);
  glBindVertexArray(VAO_Pil);

  // Creació dels buffers del model patr
  GLuint VBO_Pil[6];
  // Buffer de posicions
  glGenBuffers(6, VBO_Pil);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Pil[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*pil.faces().size()*3*3, pil.VBO_vertices(), GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  // Buffer de normals
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Pil[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*pil.faces().size()*3*3, pil.VBO_normals(), GL_STATIC_DRAW);

  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Pil[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*pil.faces().size()*3*3, pil.VBO_matamb(), GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Pil[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*pil.faces().size()*3*3, pil.VBO_matdiff(), GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Pil[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*pil.faces().size()*3*3, pil.VBO_matspec(), GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Pil[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*pil.faces().size()*3, pil.VBO_matshin(), GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  glBindVertexArray(0);
}

void MyGLWidget::creaBuffersTerra ()
{
  // Dades del terra
  // VBO amb la posició dels vèrtexs
  glm::vec3 posterra[4] = {
        glm::vec3(-10.0, 0.0, -7.0),
        glm::vec3(-10.0, 0.0,  7.0),
        glm::vec3( 10.0, 0.0, -7.0),
        glm::vec3( 10.0, 0.0,  7.0)
  }; 

  // VBO amb la normal de cada vèrtex
  glm::vec3 normt (0,1,0);
  glm::vec3 normterra[4] = {
	normt, normt, normt, normt
  };

  // Definim el material del terra
  glm::vec3 amb(0,0.1,0);
  glm::vec3 diff(0.2,0.6,0.1);
  glm::vec3 spec(0.5,0.5,0.5);
  float shin = 100;

  // Fem que aquest material afecti a tots els vèrtexs per igual
  glm::vec3 matambterra[4] = {
	amb, amb, amb, amb
  };
  glm::vec3 matdiffterra[4] = {
	diff, diff, diff, diff
  };
  glm::vec3 matspecterra[4] = {
	spec, spec, spec, spec
  };
  float matshinterra[4] = {
	shin, shin, shin, shin
  };

// Creació del Vertex Array Object del terra
  glGenVertexArrays(1, &VAO_Terra);
  glBindVertexArray(VAO_Terra);

  GLuint VBO_Terra[6];
  glGenBuffers(6, VBO_Terra);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(posterra), posterra, GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normterra), normterra, GL_STATIC_DRAW);

  // Activem l'atribut normalLoc
  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matambterra), matambterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matdiffterra), matdiffterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matspecterra), matspecterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matshinterra), matshinterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  glBindVertexArray(0);
}

void MyGLWidget::creaBuffersCub ()
{
  // Dades del cub
  // Vèrtexs del cub
  glm::vec3 vertexs[8] = {
       /* 0*/ glm::vec3( -0.5, 0.0, -0.5),  /* 1*/ glm::vec3( 0.5, 0.0, -0.5),
       /* 2*/ glm::vec3( -0.5, 1.0, -0.5),  /* 3*/ glm::vec3( 0.5, 1.0, -0.5),
       /* 4*/ glm::vec3( -0.5, 0.0, 0.5),  /* 5*/ glm::vec3( 0.5, 0.0, 0.5),
       /* 6*/ glm::vec3( -0.5, 1.0, 0.5),  /* 7*/ glm::vec3( 0.5, 1.0, 0.5)
  };

  // VBO amb la posició dels vèrtexs
  glm::vec3 poscub[36] = {  // 12 triangles
       vertexs[0], vertexs[2], vertexs[1],
       vertexs[1], vertexs[2], vertexs[3],
       vertexs[5], vertexs[1], vertexs[7],
       vertexs[1], vertexs[3], vertexs[7],
       vertexs[2], vertexs[6], vertexs[3],
       vertexs[3], vertexs[6], vertexs[7],
       vertexs[0], vertexs[4], vertexs[6],
       vertexs[0], vertexs[6], vertexs[2],
       vertexs[0], vertexs[1], vertexs[4],
       vertexs[1], vertexs[5], vertexs[4],
       vertexs[4], vertexs[5], vertexs[6],
       vertexs[5], vertexs[7], vertexs[6]
  };

  // VBO amb la normal de cada vèrtex
  glm::vec3 normals[6] = {
       /* 0*/ glm::vec3( 1.0, 0.0,  0.0),  /* 1*/ glm::vec3( -1.0, 0.0, 0.0),
       /* 2*/ glm::vec3( 0.0, 1.0,  0.0),  /* 3*/ glm::vec3( 0.0, -1.0, 0.0),
       /* 4*/ glm::vec3( 0.0, 0.0,  1.0),  /* 5*/ glm::vec3( 0.0, 0.0, -1.0)
  };
  glm::vec3 normcub[36] = {
       normals[5], normals[5], normals[5],
       normals[5], normals[5], normals[5],
       normals[0], normals[0], normals[0],
       normals[0], normals[0], normals[0],
       normals[2], normals[2], normals[2],
       normals[2], normals[2], normals[2],
       normals[1], normals[1], normals[1],
       normals[1], normals[1], normals[1],
       normals[3], normals[3], normals[3],
       normals[3], normals[3], normals[3],
       normals[4], normals[4], normals[4],
       normals[4], normals[4], normals[4]
  };

  // Definim el material del cub
  glm::vec3 amb(0.05,0.05,0.2);
  glm::vec3 diff(0.2,0.2,0.8);
  glm::vec3 spec(0.7,0.7,0.7);
  float shin = 100;

  // Fem que aquest material afecti a tots els vèrtexs per igual
  glm::vec3 matambcub[36] = {
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb
  };
  glm::vec3 matdiffcub[36] = {
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff
  };
  glm::vec3 matspeccub[36] = {
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec
  };
  float matshincub[36] = {
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin
  };

// Creació del Vertex Array Object del cub
  glGenVertexArrays(1, &VAO_Cub);
  glBindVertexArray(VAO_Cub);

  GLuint VBO_Cub[6];
  glGenBuffers(6, VBO_Cub);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(poscub), poscub, GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normcub), normcub, GL_STATIC_DRAW);

  // Activem l'atribut normalLoc
  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matambcub), matambcub, GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matdiffcub), matdiffcub, GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matspeccub), matspeccub, GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matshincub), matshincub, GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  glBindVertexArray(0);
}

void MyGLWidget::carregaShaders()
{
  // Creem els shaders per al fragment shader i el vertex shader
  QOpenGLShader fs (QOpenGLShader::Fragment, this);
  QOpenGLShader vs (QOpenGLShader::Vertex, this);
  // Carreguem el codi dels fitxers i els compilem
  fs.compileSourceFile("shaders/basicLlumShader.frag");
  vs.compileSourceFile("shaders/basicLlumShader.vert");
  // Creem el program
  program = new QOpenGLShaderProgram(this);
  // Li afegim els shaders corresponents
  program->addShader(&fs);
  program->addShader(&vs);
  // Linkem el program
  program->link();
  // Indiquem que aquest és el program que volem usar
  program->bind();

  // Obtenim identificador per a l'atribut “vertex” del vertex shader
  vertexLoc = glGetAttribLocation (program->programId(), "vertex");
  // Obtenim identificador per a l'atribut “normal” del vertex shader
  normalLoc = glGetAttribLocation (program->programId(), "normal");
  // Obtenim identificador per a l'atribut “matamb” del vertex shader
  matambLoc = glGetAttribLocation (program->programId(), "matamb");
  // Obtenim identificador per a l'atribut “matdiff” del vertex shader
  matdiffLoc = glGetAttribLocation (program->programId(), "matdiff");
  // Obtenim identificador per a l'atribut “matspec” del vertex shader
  matspecLoc = glGetAttribLocation (program->programId(), "matspec");
  // Obtenim identificador per a l'atribut “matshin” del vertex shader
  matshinLoc = glGetAttribLocation (program->programId(), "matshin");
  posFocusLoc = glGetAttribLocation (program->programId(), "posFocus");
  focEscLoc = glGetAttribLocation (program->programId(), "focEsc");
  // Demanem identificadors per als uniforms del vertex shader
  transLoc = glGetUniformLocation (program->programId(), "TG");
  projLoc = glGetUniformLocation (program->programId(), "proj");
  viewLoc = glGetUniformLocation (program->programId(), "view");
}

void MyGLWidget::segonacamera () {
	makeCurrent();
	segcam = !segcam;
	update();
}
