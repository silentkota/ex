/**************************************************************************
 *	 OGL2Widget.cpp
 *   Author: kozubovskiy
 *   Date: 10.01.2015
 *************************************************************************/
#include "ogl2widget.h"
#define HOR_RES 800
#define VERT_RES 600

OGL2Widget::OGL2Widget( QWidget *parent /*= 0*/,  Qt::WindowFlags flags /*= 0 */  )
	: QGLWidget( parent )
{
	QGLFormat f;
	f.setSampleBuffers(true);
	f.setDoubleBuffer(true);
	f.setAlpha(true);
	f.setStencil(true);
	f.setDepth(true);
	f.setOption(QGL::NoDepthBuffer);
	setFormat(f);

	if (!format().alpha())
		qWarning("QGL: Could not get alpha channel; results will be suboptimal");

	if (!format().sampleBuffers())
		qWarning("QGL: Could not get sampleBuffers; results will be suboptimal");

	//Auto update
	setUpdatesEnabled(true);
	m_RedrawTimer.setTimerType(Qt::PreciseTimer);
	connect( &m_RedrawTimer, SIGNAL(timeout()), this, SLOT( updateGL() ) );
	m_RedrawTimer.start( 0 );

	setAttribute( Qt::WA_PaintOnScreen );
	setAttribute( Qt::WA_NoSystemBackground );
	setAutoBufferSwap( true );
}

OGL2Widget::~OGL2Widget()
{

}

void OGL2Widget::initializeGL()
{
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	//font shader
	QGLShader *vshaderFont = new QGLShader(QGLShader::Vertex, this);
	const char *vsrcFont =
		"uniform mat4 u_matrix;\n"
		"attribute highp vec4 a_vertex;\n"
		"attribute vec2 a_TexCoordinate;\n"
		"varying vec2 v_TexCoordinate;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 vert = vec4( a_vertex.x, a_vertex.y, 0.0, a_vertex.w );\n"
		"    gl_Position = u_matrix * vert;\n"
		"	 v_TexCoordinate = a_TexCoordinate;\n" 
		"}\n";

	vshaderFont->compileSourceCode( vsrcFont );

	QGLShader *fshaderFont = new QGLShader(QGLShader::Fragment, this);
	const char *fsrcFont =
		"uniform lowp vec3 u_fontcolor;\n"
		"uniform sampler2D u_Texture;\n" 
		"varying vec2 v_TexCoordinate;\n" 
		"void main(void)\n"		
		"{\n"
		" vec4 gammaRGB = vec4( 1.0,1.0,1.0, 1.3 );\n"
		" lowp vec4 v_ColorText = texture2D(u_Texture, v_TexCoordinate).rgba;\n"
		" vec4 color = vec4( u_fontcolor[0], u_fontcolor[1], u_fontcolor[2], v_ColorText.a );\n"
		" color = pow(color, 1.0 / gammaRGB);\n"
		" gl_FragColor.rgba = color;\n"
		"}\n";	
	fshaderFont->compileSourceCode( fsrcFont );

	if( !m_P.SProgramFont.addShader(vshaderFont) );
		qWarning("QGL: Could not vshaderFont " );
	if( !m_P.SProgramFont.addShader(fshaderFont) );
		qWarning("QGL: Could not fshaderFont " );
	if( !m_P.SProgramFont.link() );
		qWarning("QGL: Could not SProgramFont link " );

	//main shader
	QGLShader *vshader1 = new QGLShader(QGLShader::Vertex, this);
	const char *vsrc1 =
		"uniform mat4 u_matrix;\n"
		"attribute highp vec4 a_vertex;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = u_matrix * a_vertex; \n"
		"}\n";
	vshader1->compileSourceCode(vsrc1);

	QGLShader *fshader1 = new QGLShader(QGLShader::Fragment, this);
	const char *fsrc1 =
		"uniform lowp vec4 u_color;\n"
		"void main(void)\n"
		"{\n"
		"	 gl_FragColor = u_color;"
		"}\n";
	fshader1->compileSourceCode(fsrc1);

	if( !m_P.SProgram.addShader(vshader1) );
		qWarning("QGL: Could not vshader1 " );
	if( !m_P.SProgram.addShader(fshader1) );
		qWarning("QGL: Could not fshader1 " );
	if( !m_P.SProgram.link() );
		qWarning("QGL: Could not SProgramFont link " );

	m_P.VertexAttr		= m_P.SProgram.attributeLocation("a_vertex");
	m_P.ColorUniform	= m_P.SProgram.uniformLocation("u_color");
	m_P.UMatrix			= m_P.SProgram.uniformLocation("u_matrix");

	QMatrix4x4 modelview1;
	modelview1.translate( -1.0f, 1.0f , 0.0f );
	modelview1.scale( 2.0f * 1.0f/HOR_RES, - 2.0f * 1.0f/VERT_RES );

	m_P.SProgramFont.bind();
	m_P.SProgramFont.setUniformValue( m_P.UMatrixTex, modelview1 );
	m_P.SProgramFont.release();

	m_P.SProgram.bind();
	m_P.SProgram.setUniformValue( m_P.UMatrix, modelview1 );
	m_P.SProgram.release();

	m_GLParam = &m_P;

	glEnable( GL_MULTISAMPLE );
	glDisable (GL_DEPTH_TEST);
	glDepthMask (GL_FALSE);
	glEnable(GL_DITHER );
}

void OGL2Widget::paintGL()
{
	static float i = 0;
	enum direction{ UP = 0, DOWN };
	static direction direct= DOWN;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLDrawRect( 100, 100  + i, 50, 100, QColor("green"));

	if( i > 300 )
		direct = UP;

	if( i < 0 )
		direct = DOWN;

	if( direct == DOWN )
		i += 0.1;
	else
		i -=0.1;

}

void OGL2Widget::resizeGL(int w, int h)
{
	glViewport(0, 0, (GLint)w, (GLint)h);
}

void OGL2Widget::GLDrawRect(float x, float y,  float width, float height,  QColor &ddcolor, float alpha  )
{
	m_GLParam->Vertices.clear();
	m_GLParam->SProgram.bind();
	m_GLParam->SProgram.enableAttributeArray( m_GLParam->VertexAttr );

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glLineWidth( 1.0 );
	m_GLParam->SProgram.setUniformValue(m_GLParam->ColorUniform, ddcolor );
	m_GLParam->Vertices.clear();
	m_GLParam->Vertices << QVector3D( x, y, 0.0f );
	m_GLParam->Vertices << QVector3D( x, y+height, 0.0f );
	m_GLParam->Vertices << QVector3D( x + width, y, 0.0f );

	m_GLParam->Vertices << QVector3D( x, y+height, 0.0f );
	m_GLParam->Vertices << QVector3D( x + width, y, 0.0f );
	m_GLParam->Vertices << QVector3D( x + width, y+height, 0.0f );
	m_GLParam->SProgram.setAttributeArray( m_GLParam->VertexAttr, m_GLParam->Vertices.constData() );
	glDrawArrays (  GL_TRIANGLES, 0, m_GLParam->Vertices.size() );
	glDisable(GL_BLEND);

	m_GLParam->SProgram.disableAttributeArray( m_GLParam->VertexAttr );
	m_GLParam->SProgram.release();
}