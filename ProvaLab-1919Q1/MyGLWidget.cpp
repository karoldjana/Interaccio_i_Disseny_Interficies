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
  createBuffersModel();
  createBuffersPilota();
  createBuffersPorteria();
  createBuffersTerra();

  iniEscena();
  iniCamera();
  iniIlum();
}

void MyGLWidget::iniEscena ()
{
  minEsc = glm::vec3(-10,0,-6);
  maxEsc = glm::vec3(10,6,6);
  posPort = glm::vec3(-7.0,0,0);
  radiEsc = (glm::distance(minEsc,maxEsc))/2.0f;
  emit sendsign(0);

  posPilota = glm::vec3(6.0, 0.0, 0.0);  // posició inicial de la pilota
  anglePilota = 0;  // angle inicial del xut
}

void MyGLWidget::iniIlum () {
	colFocus = glm::vec3(0.8,0.8,0.8);
    glUniform3fv (colFocusLoc, 1, &colFocus[0]);
}

void MyGLWidget::iniCamera ()
{
  angleY = M_PI/4.0;
  angleX = 0;
  fov = 2.0*asin(radiEsc/(2*radiEsc));
  fovini = fov;
  vrp = ((minEsc + maxEsc)/ 2.f);
  ra = 1.0;
  zn = radiEsc;
  zf = 3*radiEsc;
  
  projectTransform ();
  viewTransform ();
  primerap = false;
}

void MyGLWidget::paintGL ()
{
    #ifdef __APPLE__
        GLint vp[4];
        glGetIntegerv (GL_VIEWPORT, vp);
        int ample = vp[2];
        int alt = vp[3];
        glViewport (0, 0, ample, alt);
    #else
    glViewport (0, 0, width(), height());
    #endif

    // Esborrem el frame-buffer i el depth-buffer
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    viewTransform();
    projectTransform();

    // Activem el VAO per a pintar el terra
    glBindVertexArray (VAO_Terra);

    modelTransformIdent ();
    // pintem terra
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Activem el VAO per a pintar la porteria
    glBindVertexArray (VAO_Port);

    modelTransformIdent ();
    // pintem porteria
    glDrawArrays(GL_TRIANGLES, 0, 66);

    // Activem el VAO per a pintar el Patricio
    glBindVertexArray (VAO_Patr);

    modelTransformModel ();
    // Pintem Patricio
    glDrawArrays(GL_TRIANGLES, 0, patr.faces().size()*3);
    
	glBindVertexArray (VAO_Patr);
	
    modelTransformModel2();
    
    glDrawArrays(GL_TRIANGLES, 0, patr.faces().size()*3);
    
    glBindVertexArray (VAO_Pil);

    modelTransformPilota ();
    // Pintem Pilota
    glDrawArrays(GL_TRIANGLES, 0, pil.faces().size()*3);

    glBindVertexArray(0);
}

void MyGLWidget::resizeGL (int w, int h)
{
	ra = float(w) / float(h);
	if (ra < 1.0) {
		fov = 2.0*atan(tan(fovini/2.0)/ra);
	}
	else {
		fov = fovini;
	}
}

void MyGLWidget::modelTransformModel ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  TG = glm::translate(TG, glm::vec3(8.0,0,0));
  TG = glm::rotate(TG, float(-90*M_PI/180.0), glm::vec3(0, 1, 0));
  TG = glm::scale(TG, glm::vec3(4*escalaModel, 4*escalaModel, 4*escalaModel));  
  TG = glm::translate(TG, -centreBaseModel);

  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::modelTransformModel2 ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  TG = glm::translate(TG, posPort);
  TG = glm::rotate(TG, float(90*M_PI/180.0), glm::vec3(0, 1, 0));
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

void MyGLWidget::projectTransform ()
{
  glm::mat4 Proj;  // Matriu de projecció
  if(!primerap) {
	Proj = glm::perspective(fov, ra, zn, zf);
  }
  else {
	  float fov2 = M_PI/2;
	  float zn2 = 0.01;
	  float zf2 = 100;
	  Proj = glm::perspective(fov2,ra,zn2,zf2);
  }
  glUniformMatrix4fv (projLoc, 1, GL_FALSE, &Proj[0][0]);
}

void MyGLWidget::viewTransform ()
{
	if (! primerap) {
	  glm::mat4 View(1.f);  // Matriu de posició i orientació
	  View= glm::translate(View, glm::vec3(0,0,-2*radiEsc));
	  View = glm::rotate(View, angleX, glm::vec3(1, 0, 0));
	  View = glm::rotate(View, -angleY, glm::vec3(0, 1, 0));
	  View= glm::translate(View, -vrp);

	  glUniformMatrix4fv (viewLoc, 1, GL_FALSE, &View[0][0]);
	}
	else {
	  glm::mat4 View(1.f);  // Matriu de posició i orientació
	  View= glm::lookAt(glm::vec3(8,5,0), glm::vec3(-7,5,0), glm::vec3(0,1,0));
	  glUniformMatrix4fv (viewLoc, 1, GL_FALSE, &View[0][0]);
	}
}

void MyGLWidget::iniciPilota()
{
  timer.stop();
  posPilota = glm::vec3(6.0, 0.0, 0.0);
}

void MyGLWidget::mourePilota()  // No cal modificar aquest métode
{
  makeCurrent();
  timer.start(50);
  mouPilota(anglePilota);
  if (posPilota[0] <= -8.0)  // hem fet gol
    tractamentGol();
}

void MyGLWidget::mouPilota(int alfa) // No cal modificar aquest métode
{
  float rad = float(alfa) * M_PI / 180.0f;
  posPilota[0] -= cos(rad)/2.0;
  posPilota[2] += sin(rad)/2.0;

  if (aturaPorter())
    timer.stop();
  else
    if (posPilota[0] <= -8.0)
    {
      posPilota[0] = -9.0;
      posPilota[2] = 0.0;
      timer.stop();
    }
  update();
}

void MyGLWidget::tractamentGol()
{
  colFocus = glm::vec3(0.8,0,0);
  glUniform3fv (colFocusLoc, 1, &colFocus[0]);
}

bool MyGLWidget::aturaPorter()
{
  bool atura = false;

	if (posPilota.z >= (posPort.z -1) and posPilota.z <= (posPort.z +1) and (posPilota.x >= -6.3 and posPilota.x <= -5.8)) {
		atura = true;
		  colFocus = glm::vec3(0,0.8,0);
		glUniform3fv (colFocusLoc, 1, &colFocus[0]);
	}

  return atura;
}

void MyGLWidget::keyPressEvent(QKeyEvent* event)
{
  makeCurrent();
  switch (event->key()) {
    case Qt::Key_Up: { // moviment de la pilota
      if (posPilota[0] == 6.0)
        mourePilota ();
      break;
    }
    case Qt::Key_Left: { // moviment de la pilota
      if (posPort.z >= -3) {
        posPort.z += 0.15;
        modelTransformModel2();
	}
      break;
    }
    case Qt::Key_Right: { // moviment de la pilota
      if (posPilota[0] <= 3) {
        posPort.z -= 0.15;
        modelTransformModel2();
	}
      break;
    }
    case Qt::Key_I: { // reinicia posició pilota
      iniciPilota ();
      iniIlum();
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
  // Aqui cal que es calculi i s'apliqui la rotacio com s'escaigui...
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

void MyGLWidget::createBuffersModel ()
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

void MyGLWidget::createBuffersPilota ()
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

void MyGLWidget::createBuffersTerra ()
{
  // Dades del terra
  // VBO amb la posició dels vèrtexs
  glm::vec3 posterra[4] = {
        glm::vec3(-10.0, 0.0, -6.0),
        glm::vec3(-10.0, 0.0,  6.0),
        glm::vec3( 10.0, 0.0, -6.0),
        glm::vec3( 10.0, 0.0,  6.0)
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

void MyGLWidget::createBuffersPorteria ()
{
  // Dades de la porteria
  // Vèrtexs de la porteria
  glm::vec3 vertexs[16] = {
       /* 0*/ glm::vec3( -7.0, 0.0,  4.5),  /* 1*/ glm::vec3( -7.0, 0.0,  4.0),
       /* 2*/ glm::vec3( -7.0, 5.5,  4.0),  /* 3*/ glm::vec3( -7.0, 6.0,  4.5),
       /* 4*/ glm::vec3( -7.0, 5.5, -4.0),  /* 5*/ glm::vec3( -7.0, 6.0, -4.5),
       /* 6*/ glm::vec3( -7.0, 0.0, -4.0),  /* 7*/ glm::vec3( -7.0, 0.0, -4.5),
       /* 8*/ glm::vec3(-10.0, 0.0,  4.0),  /* 9*/ glm::vec3( -9.5, 0.0,  3.5),
       /*10*/ glm::vec3(-10.0, 5.5,  4.0),  /*11*/ glm::vec3( -9.5, 5.0,  3.5),
       /*12*/ glm::vec3(-10.0, 0.0, -4.0),  /*13*/ glm::vec3( -9.5, 0.0, -3.5),
       /*14*/ glm::vec3(-10.0, 5.5, -4.0),  /*15*/ glm::vec3( -9.5, 5.0, -3.5)
  };

  // VBO amb la posició dels vèrtexs
  glm::vec3 posporteria[66] = {  // 22 triangles
       vertexs[0], vertexs[1], vertexs[2],
       vertexs[0], vertexs[2], vertexs[3],
       vertexs[3], vertexs[2], vertexs[4],
       vertexs[3], vertexs[4], vertexs[5],
       vertexs[5], vertexs[4], vertexs[6],
       vertexs[5], vertexs[6], vertexs[7],
       vertexs[8], vertexs[0], vertexs[3],
       vertexs[8], vertexs[3], vertexs[10],
       vertexs[1], vertexs[9], vertexs[2],
       vertexs[2], vertexs[9], vertexs[11],
       vertexs[7], vertexs[12], vertexs[14],
       vertexs[7], vertexs[14], vertexs[5],
       vertexs[6], vertexs[4], vertexs[15],
       vertexs[6], vertexs[15], vertexs[13],
       vertexs[3], vertexs[5], vertexs[10],
       vertexs[10], vertexs[5], vertexs[14],
       vertexs[2], vertexs[11], vertexs[4],
       vertexs[11], vertexs[15], vertexs[4],
       vertexs[11], vertexs[9], vertexs[13],
       vertexs[11], vertexs[13], vertexs[15],
       vertexs[8], vertexs[10], vertexs[12],
       vertexs[12], vertexs[10], vertexs[14],
  };

  // VBO amb la normal de cada vèrtex
  glm::vec3 normals[10] = {
       /* 0*/ glm::vec3( 1.0, 0.0,  1.0),  /* 1*/ glm::vec3( 1.0, 0.0, -1.0),
       /* 2*/ glm::vec3( 0.0, 0.0,  1.0),  /* 3*/ glm::vec3( 0.0, 0.0, -1.0),
       /* 4*/ glm::vec3( 0.0, 1.0,  0.0),  /* 5*/ glm::vec3( 0.0,-1.0,  0.0),
       /* 6*/ glm::vec3( 1.0, 0.0,  0.0),  /* 7*/ glm::vec3(-1.0, 0.0,  0.0),
       /* 8*/ glm::vec3( 1.0,-1.0,  0.0),  /* 9*/ glm::vec3( 1.0, 1.0,  0.0)
  };
  glm::vec3 normporteria[66] = {
       normals[0], normals[1], normals[1],
       normals[0], normals[1], normals[0],
       normals[9], normals[8], normals[8],
       normals[9], normals[8], normals[9],
       normals[1], normals[0], normals[0],
       normals[1], normals[0], normals[1],
       normals[2], normals[0], normals[0],
       normals[2], normals[0], normals[2],
       normals[1], normals[3], normals[1],
       normals[1], normals[3], normals[3],
       normals[1], normals[3], normals[3],
       normals[1], normals[3], normals[1],
       normals[0], normals[0], normals[2],
       normals[0], normals[2], normals[2],
       normals[4], normals[4], normals[4],
       normals[4], normals[4], normals[4],
       normals[5], normals[5], normals[5],
       normals[5], normals[5], normals[5],
       normals[6], normals[6], normals[6],
       normals[6], normals[6], normals[6],
       normals[7], normals[7], normals[7],
       normals[7], normals[7], normals[7],
  };

  // Definim el material del terra
  glm::vec3 amb(0.1,0.1,0);
  glm::vec3 diff(0.8,0.8,0.1);
  glm::vec3 spec(0.5,0.5,0.5);
  float shin = 100;

  // Fem que aquest material afecti a tots els vèrtexs per igual
  glm::vec3 matambporteria[66] = {
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb,
	amb, amb, amb, amb, amb, amb
  };
  glm::vec3 matdiffporteria[66] = {
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff,
	diff, diff, diff, diff, diff, diff
  };
  glm::vec3 matspecporteria[66] = {
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec,
	spec, spec, spec, spec, spec, spec
  };
  float matshinporteria[66] = {
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin,
	shin, shin, shin, shin, shin, shin
  };

// Creació del Vertex Array Object de la porteria
  glGenVertexArrays(1, &VAO_Port);
  glBindVertexArray(VAO_Port);

  GLuint VBO_Port[6];
  glGenBuffers(6, VBO_Port);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Port[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(posporteria), posporteria, GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_Port[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normporteria), normporteria, GL_STATIC_DRAW);

  // Activem l'atribut normalLoc
  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Port[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matambporteria), matambporteria, GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Port[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matdiffporteria), matdiffporteria, GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Port[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matspecporteria), matspecporteria, GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Port[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matshinporteria), matshinporteria, GL_STATIC_DRAW);

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
  fs.compileSourceFile("shaders/fragshad.frag");
  vs.compileSourceFile("shaders/vertshad.vert");
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

  // Demanem identificadors per als uniforms del vertex shader
  transLoc = glGetUniformLocation (program->programId(), "TG");
  projLoc = glGetUniformLocation (program->programId(), "proj");
  viewLoc = glGetUniformLocation (program->programId(), "view");
  colFocusLoc = glGetUniformLocation (program->programId(), "colFocus");

}

void MyGLWidget::canvicam() {
	makeCurrent();
	primerap = !primerap;
	viewTransform();
	update();
}

void MyGLWidget::canviang(int ang) {
	makeCurrent();
	anglePilota = -ang;
	update();
}

void MyGLWidget::reset() {
	makeCurrent();
	iniIlum();
	iniciPilota();
	iniEscena();
	iniCamera();
	posPort = glm::vec3(-7,0,0);
	emit sendres(primerap);
	viewTransform();
	projectTransform();
	update();
}

