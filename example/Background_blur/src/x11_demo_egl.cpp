#define WINDOW_W 1920
#define WINDOW_H 1080
#include  <iostream>
#include  <cstdlib>
#include  <cstring>
#include  <cmath>
#include  <sys/time.h>
#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>
#include  <GLES2/gl2.h>
#include  <GLES2/gl2ext.h>
#include  <EGL/egl.h>
#include  <EGL/eglext.h>
#include  <cassert>

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
GLint position_loc, textureId;
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

static GLuint CreateSimpleTexture2D( )
{
   // Texture object handle
   GLuint textureId;
   int size_file = WINDOW_W * WINDOW_H * 4;
   GLubyte pixels[WINDOW_W * WINDOW_H * 4];

   // GLubyte pixelss[4 * 3] =
   // {
   //    255,   0,   0, // Red
   //      0, 255,   0, // Green
   //      0,   0, 255, // Blue
   //    255, 255,   0  // Yellow
   // };

	FILE *fp;
	fp = fopen("/home/linaro/bgra-1920-1080.bin","rt");
	//fp = fopen("/home/rockchip/bgra-250-250.bin","rt");
	//size_file = ftell(fp);
	printf("textures size:%d\n",size_file);
	fread(pixels, 1, size_file, fp);
	fclose(fp);

   // Use tightly packed data
   glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

   // Generate a texture object
   glGenTextures ( 1, &textureId );
   
   // Bind the texture object
   glBindTexture ( GL_TEXTURE_2D, textureId );

   // Load the texture
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_BGRA_EXT, WINDOW_W, WINDOW_H, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
   //glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelss);


   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   
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
	// draw
	// if ( !donesetup ) {
	// 	XWindowAttributes  gwa;
	// 	XGetWindowAttributes ( x_display , win , &gwa );
	// 	glViewport ( 0 , 0 , gwa.width , gwa.height );
	// 	glClearColor ( 0.08 , 0.06 , 0.07 , 1.);    // background color
	// 	donesetup = 1;
	// }
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
int  main(){
	///  the X11 part  //
	// in the first part the program opens a connection to the X11 window manager
	//
	GLint status;
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
	atom = XInternAtom ( x_display, "_NET_WM_STATE_FULLSCREEN", True );
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
	XStoreName ( x_display , win , "GL test" ); // give the window a name
	// get identifiers for the provided atom name strings
	Atom wm_state   = XInternAtom ( x_display, "_NET_WM_STATE", False );
	Atom fullscreen = XInternAtom ( x_display, "_NET_WM_STATE_FULLSCREEN", False );
	XEvent xev;
	memset ( &xev, 0, sizeof(xev) );
	xev.type                 = ClientMessage;
	xev.xclient.window       = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format       = 32;
	xev.xclient.data.l[0]    = 1;
	xev.xclient.data.l[1]    = fullscreen;
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
	
	//glLinkProgram ( shaderProgram );    // link the program
	//glUseProgram  ( shaderProgram );    // and select it for usage
	// now get the locations (kind of handle) of the shaders variables
	// phase_loc     = glGetUniformLocation ( shaderProgram , "phase"    );
	// offset_loc    = glGetUniformLocation ( shaderProgram , "offset"   );
	
	glLinkProgram(shaderProgram); 
	glUseProgram(shaderProgram);
	
	textureId 	  = CreateSimpleTexture2D();
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
		render();   // now we finally put something on the screen
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
	eglDestroyContext ( egl_display, egl_context );
	eglDestroySurface ( egl_display, egl_surface );
	eglTerminate      ( egl_display );
	XDestroyWindow    ( x_display, win );
	XCloseDisplay     ( x_display );
	return 0;
}
//# g++ demo_03.cpp -lX11 -lEGL -lGLESv2