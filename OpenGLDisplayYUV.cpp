#include "OpenGLDisplayYUV.h"

#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QCoreApplication>
#include <QResizeEvent>
#include <QTimer>
#include <QDebug>

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

namespace
{
    //Vertex matrix
    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         -1.0f, 1.0f,
         1.0f, 1.0f,
    };

    //Texture matrix
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };
}

struct OpenGLDisplayYUV::OpenGLDisplayYUVImpl
{
    OpenGLDisplayYUVImpl(QObject* ownerPtr)
        : mBufYuv(nullptr)
        , mRepaintTimer(new QTimer(ownerPtr))
        , mEnabled(true)
        , mShaderProgram(new QOpenGLShaderProgram(ownerPtr))
        , mTextureY(new QOpenGLTexture(QOpenGLTexture::Target2D))
        , mTextureU(new QOpenGLTexture(QOpenGLTexture::Target2D))
        , mTextureV(new QOpenGLTexture(QOpenGLTexture::Target2D))
    { }

    unsigned char**                     mBufYuv;
    QTimer*                             mRepaintTimer;
    bool                                mEnabled;

    QOpenGLShader*                      mVShader;
    QOpenGLShader*                      mFShader;
    QOpenGLShaderProgram*               mShaderProgram;

    QScopedPointer<QOpenGLTexture>      mTextureY;
    QScopedPointer<QOpenGLTexture>      mTextureU;
    QScopedPointer<QOpenGLTexture>      mTextureV;

    int                                 mTextureUniformY, mTextureUniformU, mTextureUniformV;
    GLsizei                             mVideoW, mVideoH;

};

/*************************************************************************/

OpenGLDisplayYUV::OpenGLDisplayYUV(QWidget* parent)
    : QOpenGLWidget(parent)
    , impl(new OpenGLDisplayYUVImpl(this))
{
    setAttribute(Qt::WA_OpaquePaintEvent);
//    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    impl->mRepaintTimer->setInterval(45);
    connect(impl->mRepaintTimer, SIGNAL(timeout()), this, SLOT(update()));
    impl->mRepaintTimer->start();
}

OpenGLDisplayYUV::~OpenGLDisplayYUV()
{
    makeCurrent();
}

void OpenGLDisplayYUV::DisplayVideoFrame(unsigned char **data, int frameWidth, int frameHeight)
{
    impl->mVideoW = frameWidth;
    impl->mVideoH = frameHeight;
    impl->mBufYuv = data;
//    update();
}

void OpenGLDisplayYUV::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);

    /* Modern opengl rendering pipeline relies on shaders to handle incoming data.
     *  Shader: is a small function written in OpenGL Shading Language (GLSL).
     * GLSL is the language that makes up all OpenGL shaders.
     * The syntax of the specific GLSL language requires the reader to find relevant information. */

    impl->mEnabled = impl->mShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/yuv.vert");
    if(!impl->mEnabled)
        qDebug() << QString("Texture shader failed: %1").arg(impl->mShaderProgram->log());

    impl->mShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/yuv.frag");
    if(!impl->mEnabled)
        qDebug() << QString("YUV2RGB shader failed: %1").arg(impl->mShaderProgram->log());

    // Bind the property vertexIn to the specified location ATTRIB_VERTEX, this property
    // has a declaration in the vertex shader source
    impl->mShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    // Bind the attribute textureIn to the specified location ATTRIB_TEXTURE, the attribute
    // has a declaration in the vertex shader source
    impl->mShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);
    //Link all the shader programs added to
    impl->mShaderProgram->link();
    //activate all links
    impl->mShaderProgram->bind();
    // Read the position of the data variables tex_y, tex_u, tex_v in the shader, the declaration
    // of these variables can be seen in
    // fragment shader source
    impl->mTextureUniformY = impl->mShaderProgram->uniformLocation("tex_y");
    impl->mTextureUniformU = impl->mShaderProgram->uniformLocation("tex_u");
    impl->mTextureUniformV = impl->mShaderProgram->uniformLocation("tex_v");

    // Set the value of the vertex matrix of the attribute ATTRIB_VERTEX and format
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    // Set the texture matrix value and format of the attribute ATTRIB_TEXTURE
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    // Enable the ATTRIB_VERTEX attribute data, the default is off
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    // Enable the ATTRIB_TEXTURE attribute data, the default is off
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    glEnable(GL_TEXTURE_2D);

    impl->mTextureY->create();
    impl->mTextureU->create();
    impl->mTextureV->create();

    impl->mTextureY->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    impl->mTextureY->setWrapMode(QOpenGLTexture::ClampToEdge);
    impl->mTextureU->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    impl->mTextureU->setWrapMode(QOpenGLTexture::ClampToEdge);
    impl->mTextureV->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    impl->mTextureV->setWrapMode(QOpenGLTexture::ClampToEdge);

    glClearColor (0.3f, 0.3f, 0.3f, 0.0); // set the background color
    glDepthFunc(GL_LEQUAL);
//    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void OpenGLDisplayYUV::resizeGL(int w, int h)
{
    if(h == 0)// prevents being divided by zero
        h = 1;// set the height to 1

    // Set the viewport
    glViewport(0, 0, w, h);
//    update();
}

void OpenGLDisplayYUV::paintGL()
{
    if (!impl->mEnabled || !impl->mBufYuv)
        return; //RET

    // Load y data texture
    // Activate the texture unit GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    // Use the texture generated from y to generate texture
    glBindTexture(GL_TEXTURE_2D, impl->mTextureY->textureId());

    // Use the memory mBufYuv data to create a real y data texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, impl->mVideoW, impl->mVideoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, impl->mBufYuv[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Load u data texture
    glActiveTexture(GL_TEXTURE1);//Activate texture unit GL_TEXTURE1
    glBindTexture(GL_TEXTURE_2D, impl->mTextureU->textureId());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, impl->mVideoW / 2, impl->mVideoH / 2
                 , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, impl->mBufYuv[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Load v data texture
    glActiveTexture(GL_TEXTURE2);//Activate texture unit GL_TEXTURE2
    glBindTexture(GL_TEXTURE_2D, impl->mTextureV->textureId());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, impl->mVideoW / 2, impl->mVideoH / 2
                 , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, impl->mBufYuv[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Specify y texture to use the new value can only use 0, 1, 2, etc. to represent
    // the index of the texture unit, this is the place where opengl is not humanized
    //0 corresponds to the texture unit GL_TEXTURE0 1 corresponds to the
    // texture unit GL_TEXTURE1 2 corresponds to the texture unit GL_TEXTURE2
    glUniform1i(impl->mTextureUniformY, 0);
    // Specify the u texture to use the new value
    glUniform1i(impl->mTextureUniformU, 1);
    // Specify v texture to use the new value
    glUniform1i(impl->mTextureUniformV, 2);
    // Use the vertex array way to draw graphics
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void OpenGLDisplayYUV::closeEvent(QCloseEvent *e)
{
    emit closed();
    e->accept();
}
