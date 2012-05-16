// This code was modified from
// http://fabiensanglard.net/shadowmapping/index.php

#ifdef _WIN32
#include "windows.h"
#endif

#ifdef __APPLE__
#include <gl.h>
#include <glut.h>
#include <glu.h>
#else
#include "GL/glew.h"
#include "GL/glut.h"
#include "GL/glu.h"
#endif

#ifdef _WIN32
#include "glext.h"
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "influence_opengl.h"

#define Y_OFFSET 0

// TODO: It would be much more efficient to use a 1-d kernel and separate convolution into
//       2 passes (horizontal & vertical). This means switching between shaders.
const float kernels[] = {0.003,0.012,0.021,0.012,0.003,
                         0.012,0.060,0.100,0.060,0.012,
                         0.021,0.100,0.166,0.100,0.021,
                         0.012,0.060,0.100,0.060,0.012,
                         0.003,0.012,0.021,0.012,0.003};

#ifdef _WIN32
// As microsoft did not maintain openGL after version 1.1, Windows platform need to go throught this crap ; macosX and Linux are fine.
// This block simply retries openGL function needed for this example.
// I recommend to use GLEW instead of going this way. This is done this way only to ease beginner's compilation and portability


PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;

// FrameBuffer (FBO) gen, bin and texturebind
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT ;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT ;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT ;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT ;

// Shader functions
PFNGLCREATEPROGRAMOBJECTARBPROC  glCreateProgramObjectARB ;
PFNGLUSEPROGRAMOBJECTARBPROC     glUseProgramObjectARB   ;
PFNGLCREATESHADEROBJECTARBPROC   glCreateShaderObjectARB ;
PFNGLSHADERSOURCEARBPROC         glShaderSourceARB        ;
PFNGLCOMPILESHADERARBPROC        glCompileShaderARB       ;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
PFNGLATTACHOBJECTARBPROC         glAttachObjectARB        ;
PFNGLLINKPROGRAMARBPROC          glLinkProgramARB         ;
PFNGLGETUNIFORMLOCATIONARBPROC   glGetUniformLocationARB  ;
PFNGLUNIFORM1IARBPROC            glUniform1iARB           ;
PFNGLACTIVETEXTUREARBPROC		  glActiveTextureARB;
PFNGLGETINFOLOGARBPROC           glGetInfoLogARB          ;

void getOpenGLFunctionPointers(void)
{
	// FBO
	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
	glGenFramebuffersEXT		= (PFNGLGENFRAMEBUFFERSEXTPROC)		wglGetProcAddress("glGenFramebuffersEXT");
	glBindFramebufferEXT		= (PFNGLBINDFRAMEBUFFEREXTPROC)		wglGetProcAddress("glBindFramebufferEXT");
	glFramebufferTexture2DEXT	= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
	glCheckFramebufferStatusEXT	= (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
	
	//Shaders
	glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)wglGetProcAddress("glCreateProgramObjectARB");
	glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)wglGetProcAddress("glUseProgramObjectARB");
	glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)wglGetProcAddress("glCreateShaderObjectARB");
	glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)wglGetProcAddress("glShaderSourceARB"); 
	glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)wglGetProcAddress("glCompileShaderARB"); 
	glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)wglGetProcAddress("glGetObjectParameterivARB"); 
	glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)wglGetProcAddress("glAttachObjectARB"); 
	glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)wglGetProcAddress("glGetInfoLogARB");
	glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)wglGetProcAddress("glLinkProgramARB");
	glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)wglGetProcAddress("glGetUniformLocationARB");
	glUniform1iARB = (PFNGLUNIFORM1IARBPROC)wglGetProcAddress("glUniform1iARB");
	
}
#endif

// Hold id of the framebuffer for light POV rendering
GLuint fboId;

// colour values will be rendered to this texture when using fboId framebuffer
GLuint fieldTexIds[2] = {0,0};

GLhandleARB fieldShaderId;
GLuint fieldUniform;
GLuint kernelsUniform;
GLuint gainUniform;

GLuint src = 0, dest = 1;
int update_rate = 100;

struct _agent agents[maxAgents];
float borderGain = 5;

int showField = 0;

void (*vfgl_DrawCallback)() = 0;

// Loading shader function
GLhandleARB loadShader(char* filename, unsigned int type)
{
	FILE *pfile;
	GLhandleARB handle;
	const GLcharARB* files[1];
	
	// shader Compilation variable
	GLint result;				// Compilation code result
	GLint errorLoglength ;
	char* errorLogText;
	GLsizei actualErrorLogLength;
	
	char buffer[400000];
	memset(buffer,0,400000);
	
	// This will raise a warning on MS compiler
	pfile = fopen(filename, "rb");
	if(!pfile)
	{
		printf("Sorry, can't open file: '%s'.\n", filename);
		exit(0);
	}
	
	fread(buffer,sizeof(char),400000,pfile);
	//printf("%s\n",buffer);
	
	
	fclose(pfile);
	
	handle = glCreateShaderObjectARB(type);
	if (!handle)
	{
		//We have failed creating the vertex shader object.
		printf("Failed creating vertex shader object from file: %s.",filename);
		exit(0);
	}
	
	files[0] = (const GLcharARB*)buffer;
	glShaderSourceARB(
					  handle, //The handle to our shader
					  1, //The number of files.
					  files, //An array of const char * data, which represents the source code of theshaders
					  NULL);
	
	glCompileShaderARB(handle);
	
	//Compilation checking.
	glGetObjectParameterivARB(handle, GL_OBJECT_COMPILE_STATUS_ARB, &result);
	
	// If an error was detected.
	if (!result)
	{
		//We failed to compile.
		printf("Shader '%s' failed compilation.\n",filename);
		
		//Attempt to get the length of our error log.
		glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &errorLoglength);
		
		//Create a buffer to read compilation error message
		errorLogText = malloc(sizeof(char) * errorLoglength);
		
		//Used to get the final length of the log.
		glGetInfoLogARB(handle, errorLoglength, &actualErrorLogLength, errorLogText);
		
		// Display errors.
		printf("%s\n",errorLogText);
		
		// Free the buffer malloced earlier
		free(errorLogText);

        exit(1);
	}
	
	return handle;
}

void loadFieldShader()
{
	GLhandleARB vertexShaderHandle;
	GLhandleARB fragmentShaderHandle;
	
	vertexShaderHandle   = loadShader("VertexShader.c",GL_VERTEX_SHADER);
	fragmentShaderHandle = loadShader("FragmentShader.c",GL_FRAGMENT_SHADER);
	
	fieldShaderId = glCreateProgramObjectARB();
	
	glAttachObjectARB(fieldShaderId,vertexShaderHandle);
	glAttachObjectARB(fieldShaderId,fragmentShaderHandle);
	glLinkProgramARB(fieldShaderId);

	fieldUniform = glGetUniformLocationARB(fieldShaderId, "field");
    if (fieldUniform == -1) {
        printf("Error getting uniform `%s'.\n", "field");
        exit(1);
    }

	kernelsUniform = glGetUniformLocationARB(fieldShaderId, "kernels");
    if (kernelsUniform == -1) {
        printf("Error getting uniform `%s'.\n", "kernels");
        exit(1);
    }

	gainUniform = glGetUniformLocationARB(fieldShaderId, "gain");
    if (gainUniform == -1) {
        printf("Error getting uniform `%s'.\n", "gain");
        exit(1);
    }

    glUseProgramObjectARB(fieldShaderId);
    glUniform1fvARB(kernelsUniform, 25, kernels);
    glUniform1fARB(gainUniform, 0.99999);
}

void generateFBO()
{
	//GLfloat borderColor[4] = {0,0,0,0};
	
	GLenum FBOstatus;

    glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
    glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
    glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);

    int i;
    for (i=0; i<2; i++)
    {
        GLuint *id = &fieldTexIds[i];
        glGenTextures(1, id);
        glBindTexture(GL_TEXTURE_2D, *id);
	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
        // Remove artefact on the edges
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
        //glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
	
        // No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available 
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F_ARB,
                      WIDTH, HEIGHT, 0, GL_RGBA,
                      GL_FLOAT, 0);
    }

	glBindTexture(GL_TEXTURE_2D, 0);
	
	// create a framebuffer object
	glGenFramebuffersEXT(1, &fboId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);
	
	// attach the texture to FBO color attachment point
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, fieldTexIds[0], 0);

    // attach the texture to FBO color attachment point
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                              GL_TEXTURE_2D, fieldTexIds[1], 0);

	// check FBO status
	FBOstatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT) {
		printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
        exit(1);
    }

	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glClear( GL_COLOR_BUFFER_BIT);

	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glClear( GL_COLOR_BUFFER_BIT);
	
	// switch back to window-system-provided framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void setupMatrices()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void update(void)
{
    // Nothing to do.
}

void drawFullScreenQuad()
{
	// Square
	glColor4f(0.3f,0.3f,0.3f,0.3f);
	glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glTexCoord2f(0,0);
	glVertex2f(0,0);
    glTexCoord2f(0,1);
	glVertex2f(0, HEIGHT);
    glTexCoord2f(1,1);
	glVertex2f(WIDTH, HEIGHT);
    glTexCoord2f(1,0);
	glVertex2f(WIDTH, 0);
	glEnd();
}

void drawAgents()
{
    float data[5*5*4];
    float gain, sin_spin, cos_spin, dir[2], flow;
    int i;
    for (i=0; i < maxAgents; i++)
    {
        if (agents[i].pos[0] > -1 && agents[i].pos[1] > -1)
        {
            // todo: spin should be read from agent data structure
            sin_spin = sin(agents[i].spin);
            cos_spin = cos(agents[i].spin);
            dir[0] = agents[i].dir[0];
            dir[1] = agents[i].dir[1];
            gain = agents[i].gain;
            flow = agents[i].flow;
            glReadPixels(agents[i].pos[0]-2, agents[i].pos[1]-2+Y_OFFSET,
                         5, 5,
                         GL_RGBA, GL_FLOAT, data);
            glBegin(GL_POINTS);
            glColor4f(data[4*4+0+2*5*4]+cos_spin*gain*(1-flow)+dir[0]*flow,
                      data[4*4+1+2*5*4]+sin_spin*gain*(1-flow)+dir[1]*flow,
                      data[4*4+2+2*5*4]+cos_spin*-gain*(1-flow)-dir[0]*flow,
                      data[4*4+3+2*5*4]+sin_spin*-gain*(1-flow)-dir[1]*flow);
            glVertex2i(agents[i].pos[0]+2, agents[i].pos[1]);

            glColor4f(data[2*4+0+4*5*4]+sin_spin*-gain*(1-flow)+dir[0]*flow,
                      data[2*4+1+4*5*4]+cos_spin*gain*(1-flow)+dir[1]*flow,
                      data[2*4+2+4*5*4]+sin_spin*gain*(1-flow)-dir[0]*flow,
                      data[2*4+3+4*5*4]+cos_spin*-gain*(1-flow)-dir[1]*flow);
            glVertex2f(agents[i].pos[0], agents[i].pos[1]+2);

            glColor4f(data[0*4+0+2*5*4]+cos_spin*-gain*(1-flow)+dir[0]*flow,
                      data[0*4+1+2*5*4]+sin_spin*-gain*(1-flow)+dir[1]*flow,
                      data[0*4+2+2*5*4]+cos_spin*gain*(1-flow)-dir[0]*flow,
                      data[0*4+3+2*5*4]+sin_spin*gain*(1-flow)-dir[1]*flow);
            glVertex2i(agents[i].pos[0]-2, agents[i].pos[1]);

            glColor4f(data[2*4+0+0*5*4]+sin_spin*gain*(1-flow)+dir[0]*flow,
                      data[2*4+1+0*5*4]+cos_spin*-gain*(1-flow)+dir[1]*flow,
                      data[2*4+2+0*5*4]+sin_spin*-gain*(1-flow)-dir[0]*flow,
                      data[2*4+3+0*5*4]+cos_spin*gain*(1-flow)-dir[1]*flow);
            glVertex2f(agents[i].pos[0], agents[i].pos[1]-2);
            glEnd();

            // we will read agent environment here for efficicency
            agents[i].obs[0] = data[2*4+0+2*5*4];
            agents[i].obs[1] = data[2*4+1+2*5*4];
            agents[i].obs[2] = data[2*4+2+2*5*4];
            agents[i].obs[3] = data[2*4+3+2*5*4];
        }
    }
}

void drawBorder()
{
    if (!borderGain)
        return;
    glBegin(GL_LINES);
    glColor4f(borderGain,0,-borderGain,0);
    glVertex2f(1, 1);
    glVertex2f(1, HEIGHT-1);
    glColor4f(0,borderGain,0,-borderGain);
    glVertex2f(1, 1);
    glVertex2f(WIDTH-1, 1);
    glColor4f(-borderGain,0,borderGain,0);
    glVertex2f(WIDTH-1, HEIGHT-1);
    glVertex2f(WIDTH-1, 1);
    glColor4f(0,-borderGain,0,borderGain);
    glVertex2f(WIDTH-1, HEIGHT-1);
    glVertex2f(1, HEIGHT-1);
    glEnd();
}

void renderScene(void) 
{
	update();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
	
	glViewport(0,0, WIDTH, HEIGHT);

    // Draw quad to the screen in the corner
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);

    setupMatrices();

    int pass = 1;

    while (pass-- > 0)
    {
        // Swap source and destination
        src = 1-src;
        dest = 1-dest;

        // Draw to the source to update agent positions
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + src);
        drawBorder();
        drawAgents();

        // Draw the shader to destination texture
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + dest);

        glClear(GL_COLOR_BUFFER_BIT);

        //Using the field shader
        glUseProgramObjectARB(fieldShaderId);
        glUniform1iARB(fieldUniform, 7);
        glActiveTextureARB(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, fieldTexIds[src]);

        drawFullScreenQuad();

        glUseProgramObjectARB(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
	
    // Draw quad to the screen in the corner
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    glClear( GL_COLOR_BUFFER_BIT);

    if (showField)
    {
        glActiveTextureARB(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fieldTexIds[dest]);

        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
        glEnable( GL_TEXTURE_2D );
        drawFullScreenQuad();
        glDisable( GL_TEXTURE_2D );

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    else {
        glPointSize(5);
        int i;
        glColor3f(1,1,1);
        glBegin(GL_POINTS);
        for (i=0; i < maxAgents; i++)
        {
            if (agents[i].pos[0] > -1 && agents[i].pos[1] > -1) {
                glVertex2f(agents[i].pos[0], agents[i].pos[1]);
            }
        }
        glEnd();
        glPointSize(0.5);
    }

	glutSwapBuffers();

    if (vfgl_DrawCallback)
        vfgl_DrawCallback();
}

void processNormalKeys(unsigned char key, int x, int y) {
	
	if (key == 27) 
		exit(0);
    if (key == 'f')
        glutFullScreen();
    if (key == ' ')
        showField = 1-showField;
}

void onTimer(int value)
{
    renderScene();
    glutTimerFunc((int)(1000.0/update_rate), onTimer, 0);
}

void vfgl_Init(int argc, char** argv)
{
    int i;
    for (i=0; i < maxAgents; i++) {
        agents[i].pos[0] = -1;//(int)fmod(rand(), WIDTH);
        agents[i].pos[1] = -1;//(int)fmod(rand(), HEIGHT);
        agents[i].gain = 5;
        agents[i].spin = 0;
        agents[i].fade = 0;
        agents[i].dir[0] = 1;
        agents[i].dir[1] = 0;
        agents[i].flow = 0;
    }

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(WIDTH,HEIGHT);
	glutCreateWindow("Influence");

#ifdef GLEW_VERSION
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
#endif
    
	glClearColor(0,0,0,0);

	// This call will grab openGL extension function pointers.
	// This is not necessary for macosx and linux
#ifdef _WIN32
	getOpenGLFunctionPointers();
#endif
	generateFBO();
	loadFieldShader();
	
	glClearColor(0,0,0,0);

	glutDisplayFunc(renderScene);
	//glutIdleFunc(renderScene);
	glutTimerFunc((int)(1000.0/update_rate), onTimer, 0);
	
	glutKeyboardFunc(processNormalKeys);
}

void vfgl_Run()
{
	glutMainLoop();
}
