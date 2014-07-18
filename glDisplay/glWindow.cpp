/* ********************************************************************
    Plugin "GLDisplay" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut f�r Technische Optik (ITO),
    Universit�t Stuttgart, Germany

    This file is part of a plugin for the measurement software itom.
  
    This itom-plugin is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public Licence as published by
    the Free Software Foundation; either version 2 of the Licence, or (at
    your option) any later version.

    itom and its plugins are distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
    General Public Licence for more details.

    You should have received a copy of the GNU Library General Public License
    along with itom. If not, see <http://www.gnu.org/licenses/>.
*********************************************************************** */
#if linux
    #include <unistd.h>
#endif

#include <qevent.h>
#include <qstring.h>
#include <qstringlist.h>
#include <iostream>
#include <qfileinfo.h>
#include <qdir.h>
#include <qimage.h>

#if (defined WIN32)
        #define NOMINMAX
        #include <Windows.h>
        #include <gl/GL.h>
        #include <gl/GLU.h>
#endif

#include "common/retVal.h"

#include "glWindow.h"
#define _USE_MATH_DEFINES  // needs to be defined to enable standard declartions of PI constant
#include "math.h"

//----------------------------------------------------------------------------------------------------------------------------------

    
//! fragment and vertex shaders for gl v2 and gl v3
//! the fragment shader multiplies input vertices with the transformation matrix MVP, the
//! fragment shader calculates the texture pixel (and color) for each pixel. In addition a 
//! gamma correction can be applied using a simple lookup vektor (lutarr)

const char *VERTEX_SHADER_SOURCE130 = "  \
#version 130                    \n\
                                \
uniform mat4 MVP;               \
in vec4 position;               \
out vec2 TexCoord;              \
                                \
void main()                     \
{                                \
    gl_Position = MVP * vec4(position.xy, 0.0, 1.0);    \
    TexCoord = position.zw;     \
}                               \
";

const char *FRAGMENT_SHADER_SOURCE130 = " \
#version 130                        \n\
                                    \
uniform sampler2D textureObject;          \
uniform int gamma;                  \
uniform mat4 color;                 \
in vec2 TexCoord;                    \
uniform vec3 lutarr[256];           \
out vec4 FragColor;                    \
                                    \
void main()                            \
{                                    \
	if (gamma == 0) \
	{               \
		float c = texture(textureObject, TexCoord).r; \
		FragColor = color * vec4(c,c,c,1.0); \
	}               \
	else            \
	{               \
		int col = int(texture(textureObject, TexCoord).r * 255.0);  \
        FragColor = color * vec4(lutarr[col], 1.0);      \
	}           \
}                                   \
";

const char *VERTEX_SHADER = " \
#version 130                        \n\
\
uniform mat4 MVP; \
\
in vec4 vertex; \
in vec2 textureCoordinate; \
out vec2 TexCoord;              \
\
void main(void) \
{ \
    gl_Position = MVP * vertex; \
    TexCoord = textureCoordinate; \
}";

const char *FRAGMENT_SHADER = " \
#version 130                        \n\
\
uniform sampler2D textureObject;          \
uniform vec4 color; \
in vec2 TexCoord;                    \
\
out vec4 fragColor; \
\
void main(void) \
{ \
    fragColor = color * texture(textureObject, TexCoord); \
}";

	//texture2d is deprecated since shader language 1.3 (version 130), use texture instead




//----------------------------------------------------------------------------------------------------------------------------------
GLWindow::GLWindow(const QGLFormat &format, QWidget *parent, const QGLWidget *shareWidget, Qt::WindowFlags f)
    : QGLWidget(format, parent, shareWidget, f)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
GLWindow::~GLWindow()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
void GLWindow::initializeGL()
{
    initializeGLFunctions();
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    // Make sure that textures are enabled.
    // I read that ATI cards need this before MipMapping.
    glEnable(GL_TEXTURE_2D);

    qglClearColor(Qt::white);

    shaderProgram.addShaderFromSourceCode(QGLShader::Vertex, VERTEX_SHADER);
    shaderProgram.addShaderFromSourceCode(QGLShader::Fragment, FRAGMENT_SHADER);
    shaderProgram.link();

    QMatrix4x4 unityMatrix;
    unityMatrix.setToIdentity();

    //!> setting up initial gamma lut with linear response for rgb
    GLfloat templut[256][3];
    for (GLint col = 0; col < 256; col++)
    {
        templut[col][0] = col / 255.0;
        templut[col][1] = col / 255.0;
        templut[col][2] = col / 255.0;
    }

    shaderProgram.bind();
    shaderProgram.setUniformValue("MVP", unityMatrix);
    shaderProgram.setUniformValue("textureObject", 0);
    shaderProgram.setUniformValue("gamma", 0);
    shaderProgram.setUniformValueArray("lutarr", &templut[0][0], 256, 3);
    shaderProgram.setUniformValue("color", QColor(Qt::white));
    shaderProgram.release();

    /*m_vertices << QVector3D(-1, -1, 0) << QVector3D(1,1,0) << QVector3D(-1,1,0) <<
        QVector3D(-1,-1,0) << QVector3D(1,-1,0) << QVector3D(1,1,0);*/

    /*m_textureCoordinates << QVector2D(0,0) << QVector2D(5,5) << QVector2D(0,5) <<
        QVector2D(0,0) << QVector2D(5,0) << QVector2D(5,5);*/

    m_vertices << QVector3D(-1, -1, 0) << QVector3D(-1, 1, 0) << QVector3D(1, -1, 0) << QVector3D(1, 1, 0);
    m_textureCoordinates << QVector2D(0,1) << QVector2D(0,0) << QVector2D(1,1) << QVector2D(1,0);

    
}

//----------------------------------------------------------------------------------------------------------------------------------
void GLWindow::resizeGL(int width, int height)
{
    if (height == 0) 
    {
        height = 1;
    }

    glViewport(0, 0, width, height);
}

//----------------------------------------------------------------------------------------------------------------------------------
void GLWindow::paintGL()
{
    QSize size = this->size();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shaderProgram.bind();
    //shaderProgram.setUniformValue("color", QColor(Qt::white));
    shaderProgram.setAttributeArray("vertex", m_vertices.constData());
    shaderProgram.enableAttributeArray("vertex");

    if (m_textures.size() > 0)
    {
        m_currentTexture = qBound(0, m_currentTexture, m_textures.size() - 1);
        const TextureItem &item = m_textures[m_currentTexture];

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, item.texture);
        checkGLError();
        glActiveTexture(0);        
        checkGLError();
        
        if (item.textureWrapS == 0)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //scaled in reality, set by texture coordinates
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, item.textureWrapS);
        }

        if (item.textureWrapT == 0)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); //scaled in reality, set by texture coordinates
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, item.textureWrapT);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, item.textureMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, item.textureMagFilter);
        checkGLError();

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
        checkGLError();

        double scaleX = size.width() / item.width;
        if (item.textureWrapS == 0)
        {
            scaleX = 1;
        }
        double scaleY = size.height() / item.height;
        if (item.textureWrapT == 0)
        {
            scaleY = 1;
        }

        m_textureCoordinates[0].setY(scaleY);
        m_textureCoordinates[2].setY(scaleY);
        m_textureCoordinates[2].setX(scaleX);
        m_textureCoordinates[3].setX(scaleX);

        shaderProgram.setAttributeArray("textureCoordinate", m_textureCoordinates.constData());
        checkGLError();
        shaderProgram.enableAttributeArray("textureCoordinate");
        checkGLError();

        //glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        checkGLError();
    
        shaderProgram.disableAttributeArray("vertex");
        shaderProgram.disableAttributeArray("textureCoordinate");

        glBindTexture(GL_TEXTURE_2D, 0);
        checkGLError();
    }

    shaderProgram.release();
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GLWindow::addTextures(const ito::DataObject &textures, QSharedPointer<int> nrOfTotalTextures, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval;

    m_objects.append(textures);

    int width, height, nrOfItems;
    const cv::Mat *plane = NULL;
    QImage image;
    uchar *linePtr;
    const ito::uint8 *cvLinePtr;
    bool valid;
    ito::DataObjectTagType tag;
    ito::ByteArray tag_;

    ito::DataObject texturesUint8;
    retval += textures.convertTo(texturesUint8, ito::tUInt8);

    if (textures.getDims() == 2)
    {
        nrOfItems = 1;
        width = textures.getSize(1);
        height = textures.getSize(0);
    }
    else if (textures.getDims() == 3)
    {
        nrOfItems = textures.getSize(0);
        width = textures.getSize(2);
        height = textures.getSize(1);
    }
    else
    {
        retval += ito::RetVal(ito::retError, 0, "conversion to 8bit object failed");
    }

    if (!retval.containsError())
    {

        image = QImage(width, height, QImage::Format_ARGB32);

        for (int i = 0; i < nrOfItems; ++i)
        {
            plane = textures.getCvPlaneMat(i);

            for (int r = 0; r < height; ++r)
            {
                linePtr = image.scanLine(r);
                cvLinePtr = plane->ptr(r);

                for (int c = 0; c < width; ++c)
                {
                    ((unsigned int*)linePtr)[c] = qRgba(cvLinePtr[c], cvLinePtr[c], cvLinePtr[c], 255);
                }
            }
            TextureItem item;
            item.texture = bindTexture(image, GL_TEXTURE_2D);
            item.textureMagFilter = GL_NEAREST;
            item.textureMinFilter = GL_NEAREST;
            item.textureWrapS = 0; //GL_REPEAT;
            item.textureWrapT = 0; //GL_REPEAT;
            item.width = width;
            item.height = height;

            tag = textures.getTag("MagFilter", valid);
            if (valid)
            {
                tag_ = tag.getVal_ToString();
                if (tag_ == "GL_NEAREST")
                {
                    item.textureMagFilter = GL_NEAREST;
                }
                else if (tag_ == "GL_LINEAR")
                {
                    item.textureMagFilter = GL_LINEAR;
                }
                else
                {
                    retval += ito::RetVal(ito::retWarning, 0, "invalid value of tag 'MagFilter'");
                }
            }

            tag = textures.getTag("MinFilter", valid);
            if (valid)
            {
                tag_ = tag.getVal_ToString();
                if (tag_ == "GL_NEAREST")
                {
                    item.textureMinFilter = GL_NEAREST;
                }
                else if (tag_ == "GL_LINEAR")
                {
                    item.textureMinFilter = GL_LINEAR;
                }
                else if (tag_ == "GL_LINEAR_MIPMAP_NEAREST")
                {
                    item.textureMinFilter = GL_LINEAR_MIPMAP_NEAREST;
                }
                else if (tag_ == "GL_NEAREST_MIPMAP_NEAREST")
                {
                    item.textureMinFilter = GL_NEAREST_MIPMAP_NEAREST;
                }
                else if (tag_ == "GL_LINEAR_MIPMAP_LINEAR")
                {
                    item.textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
                }
                else
                {
                    retval += ito::RetVal(ito::retWarning, 0, "invalid value of tag 'MinFilter'");
                }
            }

            tag = textures.getTag("wrapS", valid);
            if (valid)
            {
                tag_ = tag.getVal_ToString();
                if (tag_ == "GL_CLAMP_TO_EDGE")
                {
                    item.textureWrapS = GL_CLAMP_TO_EDGE;
                }
                else if (tag_ == "GL_REPEAT")
                {
                    item.textureWrapS = GL_REPEAT;
                }
                else if (tag_ == "GL_MIRRORED_REPEAT")
                {
                    item.textureWrapS = GL_MIRRORED_REPEAT;
                }
                else if (tag_ == "SCALED")
                {
                    item.textureWrapS = 0;
                }
                else
                {
                    retval += ito::RetVal(ito::retWarning, 0, "invalid value of tag 'wrapS'");
                }
            }

            tag = textures.getTag("wrapT", valid);
            if (valid)
            {
                tag_ = tag.getVal_ToString();
                if (tag_ == "GL_CLAMP_TO_EDGE")
                {
                    item.textureWrapT = GL_CLAMP_TO_EDGE;
                }
                else if (tag_ == "GL_REPEAT")
                {
                    item.textureWrapT = GL_REPEAT;
                }
                else if (tag_ == "GL_MIRRORED_REPEAT")
                {
                    item.textureWrapT = GL_MIRRORED_REPEAT;
                }
                else if (tag_ == "SCALED")
                {
                    item.textureWrapT = 0;
                }
                else
                {
                    retval += ito::RetVal(ito::retWarning, 0, "invalid value of tag 'wrapT'");
                }
            }
        
            m_textures.append(item);
        }
    }

    *nrOfTotalTextures = m_textures.size();

    updateGL();

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GLWindow::grabFramebuffer(const QString &filename, ItomSharedSemaphore *waitCond /*= NULL*/)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval;
    QFileInfo finfo(filename);
    QDir filepath(finfo.canonicalPath());
    
    if (filepath.exists() == false)
    {
        retval += ito::RetVal::format(ito::retError,0,"folder '%s' does not exist", finfo.canonicalPath().toLatin1().data());
    }
    else
    {
        updateGL();
        QImage shot = grabFrameBuffer(false);
        bool ok = shot.save(filepath.absoluteFilePath( finfo.fileName() ) );

        if (!ok)
        {
            retval += ito::RetVal(ito::retError,0,"error while saving grabbed framebuffer");
        }
    }

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GLWindow::setColor(const QColor &color)
{
    ito::RetVal retval;
    shaderProgram.bind();
    shaderProgram.setUniformValue("color", color);
    shaderProgram.release();
    updateGL();
    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GLWindow::setClearColor(const QColor &color)
{
    ito::RetVal retval;
    qglClearColor(color);
    updateGL();
    return retval;
}

//---------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GLWindow::checkGLError()
{
    GLenum ret = glGetError();
    ito::RetVal retval;

    switch (ret)
    {
        case GL_NO_ERROR:
            return ito::retOk;
            break;
        case GL_INVALID_ENUM:
            retval += ito::RetVal(ito::retError, ret, "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.");
        case GL_INVALID_VALUE:
            retval += ito::RetVal(ito::retError, ret, "A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.");
        case GL_INVALID_OPERATION:
            retval += ito::RetVal(ito::retError, ret, "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.");
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            retval += ito::RetVal(ito::retError, ret, "The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag.");
        case GL_OUT_OF_MEMORY:
            retval += ito::RetVal(ito::retError, ret, "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.");
        case GL_STACK_UNDERFLOW:
            retval += ito::RetVal(ito::retError, ret, "An attempt has been made to perform an operation that would cause an internal stack to underflow.");
        case GL_STACK_OVERFLOW:
            retval += ito::RetVal(ito::retError, ret, "An attempt has been made to perform an operation that would cause an internal stack to overflow.");
        default:
            retval += ito::RetVal(ito::retError, ret, "an unknown opengl error occurred");
            break;             
    }

    qDebug() << "OpenGL Error: " << retval.errorMessage();

    return retval;
}


//---------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GLWindow::setCurrentTexture(const int index)
{
    m_currentTexture = qBound(0, index, m_textures.size()-1);
    updateGL();
    return ito::retOk;
}
