#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <stdlib.h>

#include <pthread.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ao/ao.h>
#include <mpg123.h>
#define BITS 8

#include <SOIL/SOIL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;


typedef struct rando{
  float a,b;
  int c;  
}rando;


struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

static int temptextureidhack = 0;

int flag=0,flag2=0;
int display_t=0,mouseclickflag=0;
float zoom_camera = 1;
float x_change = 0; //For the camera pan
float y_change = 0; //For the camera pan
int right_mouse_clicked=0;
double new_mouse_pos_x, new_mouse_pos_y;
double mouse_pos_x, mouse_pos_y;
double xmouse,ymouse,cur_xmouse,cur_ymouse;

void drawopening(GLFWwindow* window , glm::mat4 MVP,glm::mat4 VP);
/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
float rectangle_translation=0;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
float block_translation=0;
float block_speed=0;
int score=0,life=20;


void *music(void *vargp)
{
   mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;
        /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    mpg123_open(mh, "game111.mp3");
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

    /* decode and play */
    while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
        ao_play(dev, (char*)buffer, done);

    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

}








/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */


void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
  if(flag2==0)
  {
    switch (button) {
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                right_mouse_clicked=1;
            }
            if (action == GLFW_RELEASE) {
                right_mouse_clicked=0;
            }
            break;
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS) {
                mouseclickflag=1;
            }
            if (action == GLFW_RELEASE) {
                mouseclickflag=0;
            }
            break;
        default:
            break;
    }
  }
}

void check_pan(){
    if(x_change-4.0f/zoom_camera<-4)
        x_change=-4+4.0f/zoom_camera;
    else if(x_change+4.0f/zoom_camera>4)
        x_change=4-4.0f/zoom_camera;
    if(y_change-4.0f/zoom_camera<-4)
        y_change=-4+4.0f/zoom_camera;
    else if(y_change+4.0f/zoom_camera>4)
        y_change=4-4.0f/zoom_camera;
}

void mousescroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset==-1) { 
        zoom_camera /= 1.05; //make it bigger than current size
    }
    else if(yoffset==1){
        zoom_camera *= 1.05; //make it bigger than current size
    }
    if (zoom_camera<=1) {
        zoom_camera =1 ;
    }
    if (zoom_camera>=4) {
        zoom_camera=4;
    }
    if(x_change-4.0f/zoom_camera<-4)
        x_change=-4+4.0f/zoom_camera;
    else if(x_change+4.0f/zoom_camera>4)
        x_change=4-4.0f/zoom_camera;
    if(y_change-3.0f/zoom_camera<-4)
        y_change=-4+4.0f/zoom_camera;
    else if(y_change+4.0f/zoom_camera>4)
        y_change=4-4.0f/zoom_camera;
    Matrices.projection = glm::ortho((float)(-4.0f/zoom_camera+x_change), (float)(4.0f/zoom_camera+x_change), (float)(-4.0f/zoom_camera+y_change), (float)(4.0f/zoom_camera+y_change), 0.1f, 500.0f);
}

int startflag=0;

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.


    if (action == GLFW_RELEASE && flag2==0) {
        switch (key) {
            case GLFW_KEY_UP:
                mousescroll(window,0,+1);
                check_pan();
                break;
            case GLFW_KEY_DOWN:
                mousescroll(window,0,-1);
                check_pan();
                break;
            case GLFW_KEY_RIGHT:
                x_change+=0.1;
                check_pan();
                break;
            case GLFW_KEY_LEFT:
                x_change-=0.1;
                check_pan();
                break;
            case GLFW_KEY_SPACE:
                flag=1;
                // do something ..
                break;
            case GLFW_KEY_N:
                block_speed+=0.01;
                if(block_speed>0.025)
                  block_speed=0.025;
                break;
            case GLFW_KEY_P:
              startflag=1;
              break;
            case GLFW_KEY_M:
                block_speed-=0.005;
                if(block_speed<-0.015)
                  block_speed=-0.015;
                break;

            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
  
}



/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
 Matrices.projection = glm::ortho(-4.0f/zoom_camera, 4.0f/zoom_camera, -4.0f/zoom_camera, 4.0f/zoom_camera, 0.1f, 500.0f);
}

VAO *triangle, *rectangle, *circle ,*color_circle,*rectangle2,*line,*point, *triangle2,*heartrect,*hearttri,*rect_mixed,*circle2,*circle1,*basketrectangle1,*basketrectangle2,*rectangleblack,*rectanglered,*rectanglegreen,*segment;


void createsegment()
{
  static const GLfloat vertex_buffer_data [] = {
    0,0.8,0,
    0,0.7,0,
    0.25,0.8,0,
    0.25,0.8,0,
    0,0.7,0,
    0.25,0.7,0

  };

  static const GLfloat color_buffer_data [] = {
   0,0,1,
   0,0,1,
   0,0,1,
   0,0,1,
   0,0,1,
   0,0,1
  };


  // create3DObject creates and returns a handle to a VAO that can be used later
  segment = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

}

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, -0.75,0, // vertex 0
    -0.75,-1,0, // vertex 1
    0.75,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,1,1, // color 0
    1,1,1, // color 1
    1,1,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createTriangle2 ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, -1.25,0, // vertex 0
    -0.75,-1,0, // vertex 1
    0.75,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,1,1, // color 0
    1,1,1, // color 1
    1,1,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle2 = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// Creates the rectangle object used in this sample code
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -4,0.5,0,
    -4,-0.5,0,
    -3,-0.5,0,
    -4,0.5,0,
    -3,0.5,0,
    -3,-0.5,0

  };

  static const GLfloat color_buffer_data [] = {
    0,0,1, // color 1
   0,0,1, // color 2
0,0,1,
0,0,1,
0,0,1,
0,0,1
  };


  // create3DObject creates and returns a handle to a VAO that can be used later
  rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createline()
{
  static const GLfloat vertex_buffer_data [] = {
    1, 0,0, // vertex 0
    -1,0,0,
    -1,0.02,0, // vertex 1
    -1,0.02,0,
    1,0.02,0,
    1,0,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0,
    1,0,0,
    1,0,0
  };
  line = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}


void createRectangle2()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.45,0.25,0,
    -0.45,-0.25,0,
    0.45,-0.25,0,
    -0.45,0.25,0,
    0.45,0.25,0,
    0.45,-0.25,0

  };

  static const GLfloat color_buffer_data [] = {
    0,0,1, // color 1
   0,0,1, // color 2
0,0,1,
0,0,1,
0,0,1,
0,0,1
  };


  // create3DObject creates and returns a handle to a VAO that can be used later
  rectangle2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}



void createRectanglebasket1()
{
  static const GLfloat vertex_buffer_data [] = {
    -4,-3.2,0,
    -4,-4,0,
    -2,-4,0,
    -4,-3.2,0,
    -2,-4,0,
    -2,-3.2,0
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3

    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  basketrectangle1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}





void createRectanglebasket2()
{
  static const GLfloat vertex_buffer_data [] = {
    0,-3.2,0,
    0,-4,0,
    2,-4,0,
    0,-3.2,0,
    2,-4,0,
    2,-3.2,0
  };

  static const GLfloat color_buffer_data [] = {
    0,1,0, // color 1
    0,1,0, // color 2
    0,1,0, // color 3

    0,1,0, // color 1
    0,1,0, // color 2
    0,1,0, // color 3
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  basketrectangle2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}



void createcolorcircle()
{
  float a[3240],b[3240];
  int j=0;
  for(int i=0;i<360;i++)
  {
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=cos((i*2*M_PI)/360);j++;
    a[j]=sin((i*2*M_PI)/360);j++;
    a[j]=0;j++;
    a[j]=cos(((i+1)*2*M_PI)/360);j++;
    a[j]=sin(((i+1)*2*M_PI)/360);j++;
    a[j]=0;j++;
  }
  for(int i=0;i<3240;i++)
  {
    if(i%3==0)
    {
      b[i]=0.8;
    }
    else
      b[i]=0;
  }
  
  // create3DObject creates and returns a handle to a VAO that can be used later
  color_circle = create3DObject(GL_TRIANGLES, 3*360, a, b, GL_FILL);
}

void createpoint()
{
  float a[3240],b[3240];
  int j=0;
  for(int i=0;i<360;i++)
  {
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=0.1*cos((i*2*M_PI)/360);j++;
    a[j]=0.1*sin((i*2*M_PI)/360);j++;
    a[j]=0;j++;
    a[j]=0.1*cos(((i+1)*2*M_PI)/360);j++;
    a[j]=0.1*sin(((i+1)*2*M_PI)/360);j++;
    a[j]=0;j++;
  }
  for(int i=0;i<3240;i++)
  {
    b[i]=1;
  }
  point=create3DObject(GL_TRIANGLES, 3*360, a, b, GL_FILL);
  
}


void createcircle()
{
  float a[3240],b[3240];
  int j=0;
  for(int i=0;i<360;i++)
  {
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=cos((i*2*M_PI)/360);j++;
    a[j]=sin((i*2*M_PI)/360);j++;
    a[j]=0;j++;
    a[j]=cos(((i+1)*2*M_PI)/360);j++;
    a[j]=sin(((i+1)*2*M_PI)/360);j++;
    a[j]=0;j++;
  }
  for(int i=0;i<3240;i++)
  {
    if(i%3==0)
    {
      b[i]=1;
    }
    else
      b[i]=0;
  }
  
  // create3DObject creates and returns a handle to a VAO that can be used later
  circle = create3DObject(GL_TRIANGLES, 3*360, a, b, GL_FILL);
  circle1=create3DObject(GL_TRIANGLES, 3*360, a, b, GL_FILL);
}



void createcircle2()
{
  float a[3240],b[3240];
  int j=0;
  for(int i=0;i<360;i++)
  {
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=0;j++;
    a[j]=cos((i*2*M_PI)/360);j++;
    a[j]=sin((i*2*M_PI)/360);j++;
    a[j]=0;j++;
    a[j]=cos(((i+1)*2*M_PI)/360);j++;
    a[j]=sin(((i+1)*2*M_PI)/360);j++;
    a[j]=0;j++;
  }
  for(int i=0;i<3240;i++)
  {
    if(i%3==1)
    {
      b[i]=1;
    }
    else
      b[i]=0;
  }
  
  // create3DObject creates and returns a handle to a VAO that can be used later
  circle2 = create3DObject(GL_TRIANGLES, 3*360, a, b, GL_FILL);
}


void createheartrect()
{
  static const GLfloat vertex_buffer_data [] = {
    -4,3.9,0,
    -3.8,3.9,0,
    -4,3.7,0,
    -3.8,3.9,0,
    -3.8,3.7,0,
    -4,3.7,0
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3

    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  heartrect = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createhearttriangle()
{
  static const GLfloat vertex_buffer_data [] = {
    -4,3.7,0,
    -3.6,3.7,0,
    -3.8,3.5,0
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  hearttri = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}


void createRectanglemixed()
{
   static const GLfloat vertex_buffer_data [] = {
    -0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,-0.125,0,
    0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,0.125,0
  };

  static const GLfloat color_buffer_data [] = {
    1,1,1,
    0,0,0,
    0,0,0,
    1,1,1,
    0,0,0,
    0,0,0
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  rect_mixed = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}



float b1_translation=0;
float b2_translation=0;


void drawbaskets(glm::mat4 MVP,glm::mat4 VP,GLFWwindow* window)
{
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translatecircle = glm::translate (glm::vec3(-3+b1_translation, -3.2, 0));        // glTranslatef
  glm::mat4 rotatecircle = glm::rotate((float)(60*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
  Matrices.model *= (translatecircle * rotatecircle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);


  draw3DObject(circle1);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translatecircle22 = glm::translate (glm::vec3(0, -0.3, 0));        // glTranslatef
  glm::mat4 rotatecircle22 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  glm::mat4 myScalingMatrix = glm::scale(glm::vec3(1.0, 1.0 ,0.0));
  Matrices.model *= (translatecircle22 * rotatecircle22*myScalingMatrix);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(heartrect);

   Matrices.model = glm::mat4(1.0f);

  glm::mat4 translatecircle222 = glm::translate (glm::vec3(0.2, -0.3, 0));        // glTranslatef
  glm::mat4 rotatecircle222 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  glm::mat4 myScalingMatrix2 = glm::scale(glm::vec3(1.0, 1.0 ,0.0));
  Matrices.model *= (translatecircle222 * rotatecircle222*myScalingMatrix2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(heartrect);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translatecircle2222 = glm::translate (glm::vec3(0, -0.3, 0));        // glTranslatef
  glm::mat4 rotatecircle2222 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  glm::mat4 myScalingMatrix22 = glm::scale(glm::vec3(1.0, 1.0 ,0.0));
  Matrices.model *= (translatecircle2222 * rotatecircle2222*myScalingMatrix22);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(hearttri );


  Matrices.model = glm::mat4(1.0f);

  translatecircle = glm::translate (glm::vec3(-3+b1_translation, -3.5, 0));        // glTranslatef
  rotatecircle = glm::rotate((float)(60*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
  Matrices.model *= (translatecircle * rotatecircle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);


  draw3DObject(circle1);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangleb1 = glm::translate (glm::vec3(b1_translation, 0, 0));        // glTranslatef
  glm::mat4 rotateRectangleb1 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(1,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangleb1 * rotateRectangleb1);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(basketrectangle1);


  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translatecircle2 = glm::translate (glm::vec3(1+b2_translation, -3.2, 0));        // glTranslatef
  glm::mat4 rotatecircle2 = glm::rotate((float)(60*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
  Matrices.model *= (translatecircle2 * rotatecircle2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);


  draw3DObject(circle2);


  Matrices.model = glm::mat4(1.0f);

   translatecircle2 = glm::translate (glm::vec3(1+b2_translation, -3.5, 0));        // glTranslatef
   rotatecircle2 = glm::rotate((float)(60*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
  Matrices.model *= (translatecircle2 * rotatecircle2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);


  draw3DObject(circle2);



  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangleb2 = glm::translate (glm::vec3(b2_translation, 0, 0));        // glTranslatef
  glm::mat4 rotateRectangleb2 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(1,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangleb2 * rotateRectangleb2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(basketrectangle2);


  if (glfwGetKey(window, GLFW_KEY_RIGHT ) == GLFW_PRESS && (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL ) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL ) == GLFW_PRESS)){
    b1_translation+=0.1;
    if(b1_translation>6)
      b1_translation=6;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT ) == GLFW_PRESS && (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL ) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL ) == GLFW_PRESS) ) {
    b1_translation-=0.1;
    
    if(b1_translation<0)
      b1_translation=0;
  }


  if (glfwGetKey(window, GLFW_KEY_LEFT ) == GLFW_PRESS && (glfwGetKey(window, GLFW_KEY_LEFT_ALT ) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT ) == GLFW_PRESS) ) {
    b2_translation-=0.1;
    
    if(b2_translation<-4.2)
      b2_translation=-4.2;
  }
  if (glfwGetKey(window, GLFW_KEY_RIGHT ) == GLFW_PRESS && (glfwGetKey(window, GLFW_KEY_LEFT_ALT ) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT ) == GLFW_PRESS)){
    b2_translation+=0.1;
   
     if(b2_translation>2)
      b2_translation=2;
  }


}



float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
float laser_x=0,laser_y=0,theta,angtrans,xtrans,xvel;
int angflag=0;
float x1_shake=0,y_shake=0,x2_shake=0;

void drawlaser(glm::mat4 MVP,glm::mat4 VP)
{
  if(flag==1)
  {
    if(angflag==0)
    {
      angflag=1;
      theta=(rectangle_rotation)*M_PI/180.0f;
      angtrans=rectangle_translation;
      xtrans=-3.05;
      xvel=0.15;
    }
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateline = glm::translate (glm::vec3(xtrans+laser_x, angtrans+laser_y, 0));        // glTranslatef
  glm::mat4 rotateline = glm::rotate((float)(theta), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateline * rotateline);
  MVP = VP * Matrices.model;

  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);


  draw3DObject(line);
  laser_x+=xvel;
  laser_y=(tan(theta)*(laser_x));
  x1_shake=x2_shake=y_shake=0;
  if((xtrans+laser_x)>=-2.8 && (xtrans+laser_x<=-1.25)&&(angtrans+laser_y>=2) && (angtrans+laser_y<2.22)&&tan(theta)>=0)
  {
    theta=2*(90-theta);
    xtrans=xtrans+laser_x;
    angtrans=angtrans+laser_y;
    laser_x=0;
    laser_y=0;
    x1_shake=0.1;
  }
  if((xtrans+laser_x)>=-0.7 && (xtrans+laser_x<=0.8)&&(angtrans+laser_y>=-1.6) && (angtrans+laser_y<-1.4)&&tan(theta)<=0)
  {
    theta=-2*(90-theta);
    xtrans=xtrans+laser_x;
    angtrans=angtrans+laser_y;
    laser_x=0;
    laser_y=0;
    y_shake=0.1;
  }
  if((xtrans+laser_x)<=2.75 && (xtrans+laser_x>=1.2)&&(angtrans+laser_y>=2) && (angtrans+laser_y<2.22)&&tan(theta)>=0)
  {
    theta=2*(90-theta);
    xtrans=xtrans+laser_x;
    angtrans=angtrans+laser_y;
    laser_x=0;
    laser_y=0;
    x2_shake=0.1;
  }
  if(laser_x>9 || laser_y>6)
  {
    laser_x=0;laser_y=0;
    flag=0;
    angflag=0;
  }
}
}



void drawmirror(glm::mat4 MVP,glm::mat4 VP)
{
  int x;
  float y;
  for(int i=0;i<2;i++)
  {
    if(i==0)
    {
      x=2;
    
    y=x2_shake;
  }
    else
    {
      x=-2;
      y=x1_shake;
    }
   Matrices.model = glm::mat4(1.0f);
  
  /* Render your scene */
  glm::mat4 translateTriangle = glm::translate (glm::vec3(x, 3.0f+y, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
 
  Matrices.model *=  (translateTriangle * rotateTriangle); 
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(triangle);
}

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateTriangle1 = glm::translate (glm::vec3(0, -0.5f-y_shake, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle1 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  Matrices.model *=  (translateTriangle1 * rotateTriangle1);  
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(triangle2 );


}



void createRectangleblack ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,-0.125,0,
    0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,0.125,0
  };

  static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0.1,0.1,0.1, // color 3
    0,0,0, // color 1
    0,0,0, // color 2
    0.1,0.1,0.1, // color 3
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  rectangleblack = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createRectanglered ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,-0.125,0,
    0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,0.125,0
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    0.6,0,0, // color 3
    1,0,0, // color 1
    0.6,0,0, // color 2
    1,0,0, // color 3
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  rectanglered = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createRectanglegreen ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,-0.125,0,
    0.25,-0.125,0,
    -0.25,0.125,0,
    0.25,0.125,0
  };

  static const GLfloat color_buffer_data [] = {
    0,1,0, // color 1
    0,1,0, // color 2
    0,0.6,0, // color 3
    0,1,0, // color 1
    0,1,0, // color 2
    0,0.6,0, // color 3
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  rectanglegreen = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}



void drawgameover(glm::mat4 MVP,glm::mat4 VP,int a,int b,int c,int d, int e, int f ,int g,float trans)
{
  
  glm::mat4 translateRectangle1 = glm::translate (glm::vec3(-1*trans, 0, 0));   
  if(a==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglea = glm::translate (glm::vec3(-1*trans, 3-3, 0));        // glTranslatef
  glm::mat4 rotateRectanglea = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglea * rotateRectanglea);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }
  if(f==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglef = glm::translate (glm::vec3(0.7-trans, 3.45-3, 0));        // glTranslatef
  glm::mat4 rotateRectanglef = glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglef * rotateRectanglef );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
}
  if(b==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangleb = glm::translate (glm::vec3(-0.45-trans, 3.7-3, 0));        // glTranslatef
  glm::mat4 rotateRectangleb = glm::rotate((float)(-90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangleb * rotateRectangleb );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }

  if(g==1)
  {

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangleg = glm::translate (glm::vec3(-1*trans, 2.65-3, 0));        // glTranslatef
  glm::mat4 rotateRectangleg = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangleg * rotateRectangleg );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }

  if(c==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglec = glm::translate (glm::vec3(-0.45-trans, 3.35-3, 0));        // glTranslatef
  glm::mat4 rotateRectanglec = glm::rotate((float)(-90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglec * rotateRectanglec );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }

  if(e==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglee = glm::translate (glm::vec3(0.7-trans, 3.1-3, 0));        // glTranslatef
  glm::mat4 rotateRectanglee = glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglee * rotateRectanglee );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }


  if(d==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangled = glm::translate (glm::vec3(-1*trans, 2.3-3, 0));        // glTranslatef
  glm::mat4 rotateRectangled = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangled * rotateRectangled);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(segment);
  }

}


void drawover(glm::mat4 MVP,glm::mat4 VP)
{
  float transt2=-2.5;

  drawgameover(MVP,VP,0,0,0,0,1,0,1,transt2);transt2+=0.5;
  drawgameover(MVP,VP,1,0,0,1,1,1,1,transt2);transt2+=0.5;
  drawgameover(MVP,VP,0,1,1,1,1,1,0,transt2);transt2+=0.5;
  drawgameover(MVP,VP,1,1,1,1,1,1,0,transt2);transt2+=0.85;
  drawgameover(MVP,VP,1,0,0,1,1,1,1,transt2);transt2+=0.5;
  drawgameover(MVP,VP,1,1,1,0,0,1,0,transt2);transt2+=0.25;
  drawgameover(MVP,VP,1,1,0,0,1,1,0,transt2);transt2+=0.5;
  drawgameover(MVP,VP,1,1,1,0,1,1,1,transt2);transt2+=0.5;
  drawgameover(MVP,VP,0,0,1,1,0,0,1,transt2);transt2+=0.15;
  drawgameover(MVP,VP,1,0,0,1,1,1,0,transt2);transt2+=0.5;
  
}



void drawtext(glm::mat4 MVP,glm::mat4 VP,int a,int b,int c,int d, int e, int f ,int g,float trans)
{
  
  glm::mat4 translateRectangle1 = glm::translate (glm::vec3(-1*trans, 0, 0));   
  if(a==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglea = glm::translate (glm::vec3(-1*trans+3.5, 3, 0));        // glTranslatef
  glm::mat4 rotateRectanglea = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglea * rotateRectanglea);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }
  if(f==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglef = glm::translate (glm::vec3(0.7-trans+3.5, 3.45, 0));        // glTranslatef
  glm::mat4 rotateRectanglef = glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglef * rotateRectanglef );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
}
  if(b==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangleb = glm::translate (glm::vec3(-0.45-trans+3.5, 3.7, 0));        // glTranslatef
  glm::mat4 rotateRectangleb = glm::rotate((float)(-90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangleb * rotateRectangleb );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }

  if(g==1)
  {

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangleg = glm::translate (glm::vec3(-1*trans+3.5, 2.65, 0));        // glTranslatef
  glm::mat4 rotateRectangleg = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangleg * rotateRectangleg );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }

  if(c==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglec = glm::translate (glm::vec3(-0.45-trans+3.5, 3.35, 0));        // glTranslatef
  glm::mat4 rotateRectanglec = glm::rotate((float)(-90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglec * rotateRectanglec );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }

  if(e==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectanglee = glm::translate (glm::vec3(0.7-trans+3.5, 3.1, 0));        // glTranslatef
  glm::mat4 rotateRectanglee = glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglee * rotateRectanglee );
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(segment);
  }


  if(d==1)
  {
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangled = glm::translate (glm::vec3(-1*trans+3.5, 2.3, 0));        // glTranslatef
  glm::mat4 rotateRectangled = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangled * rotateRectangled);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(segment);
  }

}




float temp_b1,temp_b2;

void drawblocks(rando *an[],glm::mat4 MVP,glm::mat4 VP,GLFWwindow* window)
{
  for(int i=0;i<1000;i++)
  {
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle = glm::translate (glm::vec3(an[i]->a, block_translation+an[i]->b, 0));        // glTranslatef
  glm::mat4 rotateRectangle = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle * rotateRectangle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  
  if(an[i]->c==0)
    draw3DObject(rectanglered);
  else if(an[i]->c==1)
    draw3DObject(rectanglegreen);
  else if(an[i]->c==2)
    draw3DObject(rectangleblack);
  else if(an[i]->c==4)
  {
    draw3DObject(rect_mixed);
  }

  float l_x[101],l_y[101];float l_x11=xtrans+laser_x,l_y11=angtrans+laser_y;
  float l_x1=l_x11+cos(theta),l_y1=l_y11+sin(theta),l_x2=l_x11-cos(theta),l_y2=l_y11-sin(theta),l_x3=l_x11-0.5*cos(theta),
  l_y3=l_y11-0.5*sin(theta),l_x4=l_x11+0.5*cos(theta),l_y4=l_y11+0.5*sin(theta);

   if(((l_x11>= -0.025+an[i]->a) && (l_x11<= 0.025+an[i]->a) &&(l_y11>=-0.125+block_translation+an[i]->b)&&(l_y11<=0.125+block_translation+an[i]->b))||
    ((l_x1>= -0.025+an[i]->a) && (l_x1<= 0.025+an[i]->a) &&(l_y1>=-0.125+block_translation+an[i]->b)&&(l_y1<=0.125+block_translation+an[i]->b))||
    ((l_x2>= -0.025+an[i]->a) && (l_x2<= 0.025+an[i]->a) &&(l_y2>=-0.125+block_translation+an[i]->b)&&(l_y2<=0.125+block_translation+an[i]->b))||
    ((l_x3>= -0.025+an[i]->a) && (l_x3<= 0.025+an[i]->a) &&(l_y3>=-0.125+block_translation+an[i]->b)&&(l_y3<=0.125+block_translation+an[i]->b))||
    ((l_x4>= -0.025+an[i]->a) && (l_x4<= 0.025+an[i]->a) &&(l_y4>=-0.125+block_translation+an[i]->b)&&(l_y4<=0.125+block_translation+an[i]->b)))
   {
    an[i]->b-=8.5;
    laser_x=0;laser_y=0;
    flag=0;
    angflag=0;
    if(an[i]->c==2 && flag2==0)
      score+=5;
    else if(an[i]->c==4)
    {
      if(life<=15)
          life+=5;
    }
    else
    {
      life--;
      if(life==0)
        flag2=1;
    }
   }

  else{
  for(int j=0;j<101;j++)
  {
  l_x[j]=l_x11+(i/100)*cos(theta),l_y[j]=l_y11+(i/100)*sin(theta);  
  if(((l_x[j]>= -0.025+an[i]->a) && (l_x[j]<= 0.025+an[i]->a) &&(l_y[j]>=-0.125+block_translation+an[i]->b)&&(l_y[j]<=0.125+block_translation+an[i]->b)))
  {
    an[i]->b-=8.5;
    laser_x=0;laser_y=0;
    flag=0;
    angflag=0;
    if(an[i]->c==2 && flag2==0)
      score+=5;
    else if(an[i]->c==4)
      { 
        if(life<=15)
          life+=5;
    }
    else
    {
      life--;
      if(life==0)
        flag2=1;
    }
    break;

      }
    }
  }
  float b_x=-0.025+an[i]->a,b_y=0.125+block_translation+an[i]->b;

  if(-2+b1_translation>=b2_translation && -2+b1_translation<=2+b2_translation)
  {
    if(b1_translation - temp_b1 <0 || b2_translation - temp_b2 >=0 )
    {
      b1_translation-=1;
    }
    else
      b1_translation+=1;
    if(b1_translation>6)
      b1_translation=6;
    else if(b1_translation<0)
      b1_translation=0;
    continue;
  }
  else if(2+b2_translation>=-4+b1_translation && 2+b2_translation<=-2+b1_translation)
  {
    if(b2_translation - temp_b2 <0 || b1_translation - temp_b1 >=0)
    {
      b2_translation-=0.5;
    }
    else
      b2_translation+=0.5;
    if(b2_translation<-4.2)
      b2_translation=-4.2;
    else if(b2_translation>2)
      b2_translation=2;
   continue; 
  }
  else if(-4+b1_translation>=b2_translation && -4+b1_translation<=2+b2_translation)
  {
    if(b2_translation - temp_b2 <0 || b1_translation - temp_b1 >=0)
    {
      b2_translation-=0.5;
    }
    else
      b2_translation+=0.5;
    if(b2_translation<-4.2)
      b2_translation=-4.2;
    else if(b2_translation>2)
      b2_translation=2;
   continue; 
  }
  else if(b2_translation>=-4+b1_translation && b2_translation<=-2+b1_translation)
  {
     if(b1_translation - temp_b1 <0 || b2_translation - temp_b2 >=0 )
    {
      b1_translation-=1;
    }
    else
      b1_translation+=1;
    if(b1_translation>6)
      b1_translation=6;
    else if(b1_translation<0)
      b1_translation=0;
    continue;
  } 

  else if(b_x>=b2_translation&& b_x <=2+b2_translation && b_y >= -4 && b_y<=-3.2)
  {
    if(an[i]->c==1 && flag2==0)
      score+=3;
    else if(an[i]->c==2)
      flag2=1;
    else if(an[i]->c==0 && flag2==0)
      life--;
    
     an[i]->b-=8.5;
  }
  else if(b_x>=-4+b1_translation&& b_x <=-2+b1_translation && b_y >= -4 && b_y<=-3.2)
  {
    if(an[i]->c==0 && flag2==0)
      score+=3;
    else if(an[i]->c==2 )
      flag2=1;
    else if(an[i]->c==1&&flag2==0)
      life--;
    
     an[i]->b-=8.5;
  }
  temp_b1=b1_translation,temp_b2=b2_translation;
}
block_translation-=0.025+block_speed;
}




/* Render the scene with openGL */
/* Edit this function according to your assignment */
double test,test2;
void draw (rando *an[],GLFWwindow* window)
{

  glfwGetCursorPos(window, &new_mouse_pos_x, &new_mouse_pos_y);
    if(right_mouse_clicked==1){
        x_change+=new_mouse_pos_x-mouse_pos_x;
        y_change-=new_mouse_pos_y-mouse_pos_y;
        check_pan();
    }

  test=new_mouse_pos_y-mouse_pos_y;
  test2=new_mouse_pos_x-mouse_pos_x;
  Matrices.projection = glm::ortho((float)(-4.0f/zoom_camera+x_change), (float)(4.0f/zoom_camera+x_change), (float)(-4.0f/zoom_camera+y_change), (float)(4.0f/zoom_camera+y_change), 0.1f, 500.0f);
  glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
  float xpos=3.05,ypos;

  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);

  /* Render your scene */

  

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  if(startflag==0)
    drawopening(window,MVP,VP);
  else{

  drawmirror(MVP,VP);

  if(flag==1)
  {
    drawlaser(MVP,VP);
  }

  Matrices.model = glm::mat4(1.0f);


  glfwGetCursorPos(window, &xmouse, &ymouse);


  int circ_flag=0;
  glfwGetCursorPos(window, &xmouse, &ymouse);
  if(xmouse>=0 && xmouse<=145 && ymouse>=300-(100*rectangle_translation) &&ymouse<=500-(100*rectangle_translation))
  {
    glfwGetCursorPos(window, &cur_xmouse,&cur_ymouse);
    if(mouseclickflag==1)
    {
      if(test>0)
        rectangle_translation-=0.1;
      else
        rectangle_translation+=0.1;
    if(rectangle_translation>2.5)
      rectangle_translation=2.5;
    else if(rectangle_translation<-2)
      rectangle_translation=-2;
     circ_flag=1;
    }
   else
    circ_flag=0;
 }
  else if(xmouse>=100*b1_translation-100 && xmouse<=100*b1_translation+100 && ymouse>=670 &&ymouse<=800 && mouseclickflag==1)
  {
    if(test2>0)
      b1_translation+=0.05;
    else
      b1_translation-=0.05;
    if(b1_translation>6)
      b1_translation=6;
    else if(b1_translation<0)
      b1_translation=0;
  }
  else if(xmouse>=100*b2_translation+400 && xmouse<=100*b2_translation+600 && ymouse>=670 &&ymouse<=800 && mouseclickflag==1)
  {
    if(test2>0)
      b2_translation+=0.05;
    else
      b2_translation-=0.05;
    if(b2_translation<-2)
      b2_translation=-2;
    else if(b2_translation>2)
      b2_translation=2;
  }


  glm::mat4 translatecircle = glm::translate (glm::vec3(-4, rectangle_translation, 0));        // glTranslatef
  glm::mat4 rotatecircle = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
  Matrices.model *= (translatecircle * rotatecircle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  if(circ_flag==1)
  {
    draw3DObject(color_circle);
  }
  else
    draw3DObject(circle);

  if(xmouse>=81 && xmouse<=300 && ymouse>=0 &&ymouse<=800)
  {
    rectangle_rotation=(atan((ymouse - rectangle_translation-400 )/(xmouse+3.05-400))*(180/M_PI));
    if(rectangle_rotation<-54)
      rectangle_rotation=-54;
    else if(rectangle_rotation>54)
      rectangle_rotation=54;
    
  }

   if(xmouse>=81 && xmouse<=800 && ymouse>=0 &&ymouse<=650)
  {
    if(mouseclickflag==1&&flag2==0)
    {
      flag=1;
    }
  }


  double pointx,pointy;
  glfwGetCursorPos(window, &pointx, &pointy);
  pointx=(pointx-400)/100;
  pointy=(-pointy+400)/100;
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectanglep = glm::translate (glm::vec3(pointx,pointy, 0));        // glTranslatef
  glm::mat4 rotateRectanglep = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectanglep * rotateRectanglep);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(point);


  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle = glm::translate (glm::vec3(0, rectangle_translation, 0));        // glTranslatef
  glm::mat4 rotateRectangle = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle * rotateRectangle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(rectangle);


   Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle2 = glm::translate (glm::vec3(-3.05, rectangle_translation, 0));        // glTranslatef
  glm::mat4 rotateRectangle2 = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle2 * rotateRectangle2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(rectangle2);


  if (glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS)
  {
    rectangle_translation+=0.1;
    if(rectangle_translation>2.5)
      rectangle_translation=2.5;
  }
  if (glfwGetKey(window, GLFW_KEY_F ) == GLFW_PRESS)
  {
    rectangle_translation-=0.1;
    if(rectangle_translation<-2)
      rectangle_translation=-2;
  }

  if (glfwGetKey(window, GLFW_KEY_D ) == GLFW_PRESS)
  {
    rectangle_rotation-=1;
    if(rectangle_rotation<-54)
      rectangle_rotation=-54;
    
  }
  if (glfwGetKey(window, GLFW_KEY_A ) == GLFW_PRESS)
  {
    rectangle_rotation+=1;
    if(rectangle_rotation>54)
      rectangle_rotation=54;
  }

  // Increment angles
  float increments = 1;
  drawblocks(an,MVP,VP,window);

  drawbaskets(MVP,VP,window);
  int temptext=score,templife=life;float transt=0,transt2=3.5,transt1=6.5;
  if(temptext==0)
  {
    drawtext(MVP,VP,1,1,1,1,1,1,0,transt);
  }
  while(temptext)
  {
    int digit=temptext%10;
    temptext/=10;
    if(digit==0)
      drawtext(MVP,VP,1,1,1,1,1,1,0,transt);
    else if(digit==1)
      drawtext(MVP,VP,0,1,1,0,0,0,0,transt);
    else if(digit==2)
      drawtext(MVP,VP,1,1,0,1,1,0,1,transt);
    else if(digit==3)
      drawtext(MVP,VP,1,1,1,1,0,0,1,transt);
    else if(digit==4)
      drawtext(MVP,VP,0,1,1,0,0,1,1,transt);
    else if(digit==5)
      drawtext(MVP,VP,1,0,1,1,0,1,1,transt);
    else if(digit==6)
      drawtext(MVP,VP,1,0,1,1,1,1,1,transt);
    else if(digit==7)
      drawtext(MVP,VP,1,1,1,0,0,0,0,transt);
    else if(digit==8)
      drawtext(MVP,VP,1,1,1,1,1,1,1,transt);
    else if(digit==9)
      drawtext(MVP,VP,1,1,1,1,0,1,1,transt);
    transt+=0.5;
  }

  while(templife)
  {
    int digit=templife%10;
    templife/=10;
    if(digit==0)
      drawtext(MVP,VP,1,1,1,1,1,1,0,transt1);
    else if(digit==1)
      drawtext(MVP,VP,0,1,1,0,0,0,0,transt1);
    else if(digit==2)
      drawtext(MVP,VP,1,1,0,1,1,0,1,transt1);
    else if(digit==3)
      drawtext(MVP,VP,1,1,1,1,0,0,1,transt1);
    else if(digit==4)
      drawtext(MVP,VP,0,1,1,0,0,1,1,transt1);
    else if(digit==5)
      drawtext(MVP,VP,1,0,1,1,0,1,1,transt1);
    else if(digit==6)
      drawtext(MVP,VP,1,0,1,1,1,1,1,transt1);
    else if(digit==7)
      drawtext(MVP,VP,1,1,1,0,0,0,0,transt1);
    else if(digit==8)
      drawtext(MVP,VP,1,1,1,1,1,1,1,transt1);
    else if(digit==9)
      drawtext(MVP,VP,1,1,1,1,0,1,1,transt1);
    transt1+=0.5;
  }

  while(display_t)
  {
    int digit=display_t%10;
    display_t/=10;
    if(digit==0)
      drawtext(MVP,VP,1,1,1,1,1,1,0,transt2);
    else if(digit==1)
      drawtext(MVP,VP,0,1,1,0,0,0,0,transt2);
    else if(digit==2)
      drawtext(MVP,VP,1,1,0,1,1,0,1,transt2);
    else if(digit==3)
      drawtext(MVP,VP,1,1,1,1,0,0,1,transt2);
    else if(digit==4)
      drawtext(MVP,VP,0,1,1,0,0,1,1,transt2);
    else if(digit==5)
      drawtext(MVP,VP,1,0,1,1,0,1,1,transt2);
    else if(digit==6)
      drawtext(MVP,VP,1,0,1,1,1,1,1,transt2);
    else if(digit==7)
      drawtext(MVP,VP,1,1,1,0,0,0,0,transt2);
    else if(digit==8)
      drawtext(MVP,VP,1,1,1,1,1,1,1,transt2);
    else if(digit==9)
      drawtext(MVP,VP,1,1,1,1,0,1,1,transt2);
    transt2+=0.5;
  }
  if(life==0)
    flag2=1;
  if(flag2==1)
    drawover(MVP,VP);


  //camera_rotation_angle++; // Simulating camera rotation
  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
}
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Shoot out at IIIT", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, mousescroll);


    return window;
}


/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createTriangle2();
  createcircle();
  createcircle2();
  createcolorcircle();

  createRectangle();
  createRectangle2();
  createline();
  createRectanglebasket1();
  createRectanglebasket2();
  createRectangleblack ();
  createRectanglegreen();
  createRectanglemixed();
  createRectanglered();
  createsegment();
  createheartrect();
  createhearttriangle();
  createpoint();
	
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.15f, 0.15f, 0.15f, 0.3f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

   
}


void drawopening(GLFWwindow* window , glm::mat4 MVP,glm::mat4 VP)
{
   

  float transt2=-2.5;
  drawgameover(MVP,VP,0,1,1,1,0,1,1,transt2);transt2+=0.5;//y
  drawgameover(MVP,VP,1,1,1,0,1,1,1,transt2);transt2+=0.5;//a
  drawgameover(MVP,VP,0,0,0,1,1,1,0,transt2);transt2+=0.5;//l
  drawgameover(MVP,VP,1,1,0,0,1,1,1,transt2);transt2+=0.85;//p
  drawgameover(MVP,VP,1,1,0,0,1,1,1,transt2);transt2+=0.85;//p
  drawgameover(MVP,VP,1,0,1,1,0,1,1,transt2);transt2+=0.5;//s
  drawgameover(MVP,VP,1,0,1,1,0,1,1,transt2);transt2+=0.5;//s
  drawgameover(MVP,VP,1,0,0,1,1,1,1,transt2);transt2+=0.5;//e
  drawgameover(MVP,VP,0,0,0,0,1,0,1,transt2);transt2+=0.5;//r
  drawgameover(MVP,VP,1,1,0,0,1,1,1,transt2);transt2+=0.85;//p

}




int main (int argc, char** argv)
{

  pthread_t tid,tid1;
	int width = 800;
	int height = 800;

  rando *an[100000];
  for(int i=0;i<100000;i++)
  {
    an[i]=(rando*)malloc(sizeof(rando));
    an[i]->a=rand()%4 * ((rand()%2)==0?-1:1) + 0.75;
    an[i]->b=4+i;
    an[i]->c=rand()%3;
  }
  for(int i=50;i<100000;i+=50)
    an[i]->c=4;

  

  
    GLFWwindow* window = initGLFW(width, height);

	  initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time,display_time= glfwGetTime();

    system("vi Readme.txt &");
   

    /* Draw in loop */
    pthread_create(&tid, NULL, music, NULL);
    while (!glfwWindowShouldClose(window)) {
            if(startflag==1)
              display_t=glfwGetTime()-display_t;

        // OpenGL Draw commands
        draw(an,window);
        
        

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }
    pthread_join(tid, NULL);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
