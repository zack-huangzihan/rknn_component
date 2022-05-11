#define WINDOW_W 1280
#define WINDOW_H 960
#include  <iostream>
#include  <cstdlib>
#include  <cstring>
#include  <cmath>
#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <cassert>
#include  <string.h>
#include  <sys/time.h>
#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>
#include  <GLES2/gl2.h>
#include  <GLES2/gl2ext.h>
#include  <EGL/egl.h>
#include  <EGL/eglext.h>
#include  <vector>
#include  "v4l2_camera.h"
#include <opencv2/imgproc.hpp>

#include "rockx.h"
#include "modules/bg_matting.h"

using namespace cv;
using namespace std;


const char vertex_src [] =
      "#version 300 es                            \n"
      "layout(location = 0) in vec4 a_position;   \n"
      "layout(location = 1) in vec2 a_texCoord;   \n"
      "out vec2 v_texCoord;                       \n"
      "void main()                                \n"
      "{                                          \n"
      "   gl_Position = a_position;               \n"
      "   v_texCoord = a_texCoord;                \n"
      "}                                          \n";
 
 
const char fragment_src [] =
      "#version 300 es                                     \n"
      "precision mediump float;                            \n"
      "in vec2 v_texCoord;                                 \n"
      "layout(location = 0) out vec4 outColor;             \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  outColor = texture( s_texture, v_texCoord );      \n"
      "}                                                   \n";
 
// handle to the shader
rockx_handle_t bg_matting_handle;
void print_shader_info_log (GLuint shader){
	GLint  length;
 
	glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &length );
 
	if ( length ) {
		char* buffer  =  new char [ length ];
		glGetShaderInfoLog ( shader , length , NULL , buffer );
		cout << "shader info: " <<  buffer << flush;
		delete [] buffer;
		GLint success;
		glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
		if ( success != GL_TRUE )   exit ( 1 );
	}
}
GLuint load_shader ( const char  *shader_source, GLenum type){
	GLuint  shader = glCreateShader( type );
	glShaderSource  ( shader , 1 , &shader_source , NULL );
	glCompileShader ( shader );
	print_shader_info_log ( shader );
	return shader;
}
Display    *x_display;
Window      win;
EGLDisplay  egl_display;
EGLContext  egl_context;
EGLSurface  egl_surface;
GLfloat
  	norm_x    =  0.0,
	norm_y    =  0.0,
	offset_x  =  0.0,
	offset_y  =  0.0,
	p1_pos_x  =  0.0,
	p1_pos_y  =  0.0;
GLint position_loc;
GLuint textureId;
bool update_pos = false;

static GLuint
create_shader(struct window *window, const char *source, GLenum shader_type)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(shader_type);
	assert(shader != 0);

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		fprintf(stderr, "Error: compiling %s: %.*s\n",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			len, log);
		exit(1);
	}

	return shader;
}

const int CAMERA_ID = 0;

static GLuint CreateSimpleTexture2D( )
{
   // Texture object handle
   int ret = 0;
   GLuint textureId;
   //int size_file = WINDOW_W * WINDOW_H * 4;
   char frame[WINDOW_W * WINDOW_H * 2];
   //GLubyte pixels[WINDOW_W * WINDOW_H * 4];
   v4l2_camera_get_frame(CAMERA_ID, (unsigned char*)frame, WINDOW_W*WINDOW_H*2);
   Mat camFrame(Size(WINDOW_W, WINDOW_H), CV_8UC2, frame, cv::Mat::AUTO_STEP);
   Mat pixels;
   cvtColor(camFrame, pixels, CV_YUV2RGB_YUYV);
   //数据是pixels.data

	rockx_image_t input_image;
	memset(&input_image, 0, sizeof(rockx_image_t));
    input_image.height = WINDOW_H;
    input_image.width = WINDOW_W;
    input_image.is_prealloc_buf = true;
    input_image.data = pixels.data;
    input_image.pixel_format = ROCKX_PIXEL_FORMAT_RGB888;

    rockx_bg_matting_array_t bg_mat_array;
    bg_mat_array.height = WINDOW_H;
    bg_mat_array.width = WINDOW_W;
    bg_mat_array.matting_img = (uint8_t *)malloc(WINDOW_H * WINDOW_W * 4);
    //TIME_BEGIN(rockx_bg_matting);
    ret = rockx_bg_matting(bg_matting_handle, &input_image, &bg_mat_array, nullptr);
    if (ret != ROCKX_RET_SUCCESS) {
        printf("rockx_bg_matting error %d\n", ret);
        return -1;
    }
    //TIME_END(rockx_bg_matting);

   // Use tightly packed data
   glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

   // Generate a texture object
   glGenTextures ( 1, &textureId );
   
   // Bind the texture object
   glBindTexture ( GL_TEXTURE_2D, textureId );

   // Load the texture
   //glTexImage2D ( GL_TEXTURE_2D, 0, GL_BGRA_EXT, WINDOW_W, WINDOW_H, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
   //glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, WINDOW_W, WINDOW_H, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data);
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_W, WINDOW_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, bg_mat_array.matting_img);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   free(bg_mat_array.matting_img);

   return textureId;
}

void  render(){
	static float  phase = 0;
	static int    donesetup = 0;
	static XWindowAttributes gwa;
   	//正常贴图
   	GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0 
                           -1.0f, -1.0f, 0.0f,  // Position 1
                            0.0f,  1.0f,        // TexCoord 1
                            1.0f, -1.0f, 0.0f,  // Position 2
                            1.0f,  1.0f,        // TexCoord 2
                            1.0f,  1.0f, 0.0f,  // Position 3
                            1.0f,  0.0f         // TexCoord 3
                         };
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	glViewport(0, 0, WINDOW_W, WINDOW_H);
	glClear ( GL_COLOR_BUFFER_BIT );
   	glVertexAttribPointer ( 0, 3, GL_FLOAT,
                           GL_FALSE, 5 * sizeof ( GLfloat ), vVertices );

   	glVertexAttribPointer ( 1, 2, GL_FLOAT,
                           GL_FALSE, 5 * sizeof ( GLfloat ), &vVertices[3] );
   	glEnableVertexAttribArray (0);
   	glEnableVertexAttribArray (1);
   	glActiveTexture ( GL_TEXTURE0 );
   	glBindTexture ( GL_TEXTURE_2D, textureId );
   	// Set the sampler texture unit to 0
   	glUniform1i (position_loc, phase);

   	glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
   	eglSwapBuffers ( egl_display, egl_surface );
}

static inline int readenv_atoi(char *env) {
  char *p;
  if (( p = getenv(env) ))
  	return (atoi(p));
  else
	return(0);
}

int  main(){

	int ret = 0;
	char *license_path = "/usr/lib/key.lic";
	int numprocs = readenv_atoi("BACKGROUND_BLUR_VIDEO");
    rockx_config_t *config = rockx_create_config();
    rockx_add_config(config, ROCKX_CONFIG_LICENCE_KEY_PATH, (char *)license_path, strlen(license_path));
    ret = rockx_create(&bg_matting_handle, ROCKX_MODULE_BG_MATTING, config, sizeof(rockx_config_t));
    if (ret != ROCKX_RET_SUCCESS) {
        printf("init rockx module ROCKX_MODULE_BG_MATTING error %d\n", ret);
        return -1;
    }
    rockx_release_config(config);

	v4l2_camera_open(numprocs, WINDOW_W, WINDOW_H);
	GLint status;
	//open_v4l2();
	x_display = XOpenDisplay ( NULL );   // open the standard display (the primary screen)
	if ( x_display == NULL ) {
		cerr << "cannot connect to X server" << endl;
		return 1;
	}
	Window root  =  DefaultRootWindow( x_display );   // get the root window (usually the whole screen)
	XSetWindowAttributes  swa;
	swa.event_mask  =  ExposureMask | PointerMotionMask | KeyPressMask;
	win  =  XCreateWindow (   // create a window with the provided parameters
												 x_display, root,
												 0, 0, WINDOW_W, WINDOW_H,   0,
												 CopyFromParent, InputOutput,
												 CopyFromParent, CWEventMask,
												 &swa );
	XSetWindowAttributes  xattr;
	Atom  atom;
	int   one = 1;
	xattr.override_redirect = False;
	XChangeWindowAttributes ( x_display, win, CWOverrideRedirect, &xattr );
	atom = XInternAtom ( x_display, "_NET_WM_STATE", True );
	XChangeProperty (
									 x_display, win,
									 XInternAtom ( x_display, "_NET_WM_STATE", True ),
									 XA_ATOM,  32,  PropModeReplace,
									 (unsigned char*) &atom,  1 );
	XChangeProperty (
									 x_display, win,
									 XInternAtom ( x_display, "_HILDON_NON_COMPOSITED_WINDOW", False ),
									 XA_INTEGER,  32,  PropModeReplace,
									 (unsigned char*) &one,  1);
	XWMHints hints;
	hints.input = True;
	hints.flags = InputHint;
	XSetWMHints(x_display, win, &hints);
	XMapWindow ( x_display , win );             // make the window visible on the screen
	XStoreName ( x_display , win , "Background blur test" ); // give the window a name
	// get identifiers for the provided atom name strings
	Atom wm_state   = XInternAtom ( x_display, "_NET_WM_STATE", False );
	//Atom fullscreen = XInternAtom ( x_display, "_NET_WM_STATE_FULLSCREEN", False );
	XEvent xev;
	memset ( &xev, 0, sizeof(xev) );
	xev.type                 = ClientMessage;
	xev.xclient.window       = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format       = 32;
	xev.xclient.data.l[0]    = 1;
	//xev.xclient.data.l[1]    = fullscreen;
	xev.xclient.data.l[1]    = 0;
	XSendEvent (                // send an event mask to the X-server
							x_display,
							DefaultRootWindow ( x_display ),
							False,
							SubstructureNotifyMask,
							&xev );
	///  the egl part  //
	//  egl provides an interface to connect the graphics related functionality of openGL ES
	//  with the windowing interface and functionality of the native operation system (X11
	//  in our case.
	egl_display  =  eglGetDisplay( (EGLNativeDisplayType) x_display );
	if ( egl_display == EGL_NO_DISPLAY ) {
		cerr << "Got no EGL display." << endl;
		return 1;
	}
	if ( !eglInitialize( egl_display, NULL, NULL ) ) {
		cerr << "Unable to initialize EGL" << endl;
		return 1;
	}
	EGLint attr[] = {       // some attributes to set up our egl-interface
		EGL_BUFFER_SIZE, 16,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLConfig  ecfg;
	EGLint     num_config;
	if ( !eglChooseConfig( egl_display, attr, &ecfg, 1, &num_config ) ) {
		cerr << "Failed to choose config (eglError: " << eglGetError() << ")" << endl;
		return 1;
	}
	if ( num_config != 1 ) {
		cerr << "Didn't get exactly one config, but " << num_config << endl;
		return 1;
	}
	egl_surface = eglCreateWindowSurface ( egl_display, ecfg, win, NULL );
	if ( egl_surface == EGL_NO_SURFACE ) {
		cerr << "Unable to create EGL surface (eglError: " << eglGetError() << ")" << endl;
		return 1;
	}
	// egl-contexts collect all state descriptions needed required for operation
	EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	egl_context = eglCreateContext ( egl_display, ecfg, EGL_NO_CONTEXT, ctxattr );
	if ( egl_context == EGL_NO_CONTEXT ) {
		cerr << "Unable to create EGL context (eglError: " << eglGetError() << ")" << endl;
		return 1;
	}
	// associate the egl-context with the egl-surface
	eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );
	///  the openGL part  ///
	GLuint vertexShader   = load_shader ( vertex_src ,GL_VERTEX_SHADER);     // load vertex shader
	GLuint fragmentShader = load_shader ( fragment_src ,GL_FRAGMENT_SHADER);  // load fragment shader
	
	GLuint shaderProgram  = glCreateProgram ();                 // create program object
	glAttachShader ( shaderProgram, vertexShader );             // and attach both...
	glAttachShader ( shaderProgram, fragmentShader );           // ... shaders to it	
	
	glLinkProgram(shaderProgram); 
	glUseProgram(shaderProgram);
	
	position_loc  = glGetUniformLocation  (shaderProgram, "s_texture" );
	printf("textureId= %d, position_loc= %d\n", textureId, position_loc);
	if ( position_loc < 0  || textureId < 0) {
		cerr << "Unable to get uniform location" << endl;
		return 1;
	}
	const float
		window_width  = WINDOW_W,
		window_height = WINDOW_H;
	// this is needed for time measuring  -->  frames per second
	struct  timezone  tz;
	timeval  t1, t2;
	gettimeofday ( &t1 , &tz );
	int  num_frames = 0;
 
	bool quit = false;
	while ( !quit ) {    // the main loop
 
		while ( XPending ( x_display ) ) {   // check for events from the x-server
 
			XEvent  xev;
			XNextEvent( x_display, &xev );
 
			if ( xev.type == MotionNotify ) {  // if mouse has moved
				//            cout << "move to: << xev.xmotion.x << "," << xev.xmotion.y << endl;
				GLfloat window_y  =  (window_height - xev.xmotion.y) - window_height / 2.0;
				norm_y            =  window_y / (window_height / 2.0);
				GLfloat window_x  =  xev.xmotion.x - window_width / 2.0;
				norm_x            =  window_x / (window_width / 2.0);
				update_pos = true;
			}
			if ( xev.type == KeyPress )   quit = true;
		}
		textureId = CreateSimpleTexture2D();
		render();   // now we finally put something on the screen
		glDeleteTextures(1, &textureId);
		if ( ++num_frames % 100 == 0 ) {
			gettimeofday( &t2, &tz );
			float dt  =  t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
			cout << "fps: " << num_frames / dt << endl;
			num_frames = 0;
			t1 = t2;
		}
		//      usleep( 1000*10 );
	}
	//  cleaning up...
	v4l2_camera_close (numprocs);
	eglDestroyContext ( egl_display, egl_context );
	eglDestroySurface ( egl_display, egl_surface );
	eglTerminate      ( egl_display );
	XDestroyWindow    ( x_display, win );
	XCloseDisplay     ( x_display );

	rockx_destroy(bg_matting_handle);
	return 0;
}