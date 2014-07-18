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

#ifndef GLWINDOW_H
#define GLWINDOW_H

#define NOMINMAX        // we need this define to remove min max macros from M$ includes, otherwise we get problems within params.h


#include <QGLWidget>
#include <qvector.h>

#if QT_VERSION >= 0x050000
	#include <qopenglfunctions.h>
	#include <qopenglvertexarrayobject.h>
#else
    #include <qglfunctions.h>
    #include <qglshaderprogram.h>
    #include <qglfunctions.h>
#endif

#include "DataObject/dataobj.h"

#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"

//----------------------------------------------------------------------------------------------------------------------------------
class GLWindow : public QGLWidget, protected QGLFunctions
{
    Q_OBJECT

public:
    GLWindow(const QGLFormat &format, QWidget *parent = 0, const QGLWidget *shareWidget = 0, Qt::WindowFlags f = 0);
    ~GLWindow();

protected:

    struct TextureItem
    {
        GLuint texture;
        GLint textureMinFilter;
        GLint textureMagFilter;
        GLint textureWrapS;
        GLint textureWrapT;
        int width;
        int height;
    };

    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();

    ito::RetVal checkGLError();

private:
    QGLShaderProgram shaderProgram;
    QVector<QVector3D> m_vertices;
    QVector<QVector2D> m_textureCoordinates;

    QVector<ito::DataObject> m_objects;
    QVector<TextureItem> m_textures;
    int m_currentTexture;

public slots:
    ito::RetVal addTextures(const ito::DataObject &textures, QSharedPointer<int> nrOfTotalTextures, ItomSharedSemaphore *waitCond = NULL);
    ito::RetVal setColor(const QColor &color);
    ito::RetVal setClearColor(const QColor &color);
    ito::RetVal grabFramebuffer(const QString &filename, ItomSharedSemaphore *waitCond = NULL);
    ito::RetVal setCurrentTexture(const int index);
};

//----------------------------------------------------------------------------------------------------------------------------------

#endif // GLWINDOW_H
