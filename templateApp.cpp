/*

Book:      	Game and Graphics Programming for iOS and Android with OpenGL(R) ES 2.0
Author:    	Romain Marucchi-Foino
ISBN-10: 	1119975913
ISBN-13: 	978-1119975915
Publisher: 	John Wiley & Sons	

Copyright (C) 2011 Romain Marucchi-Foino

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of
this software. Permission is granted to anyone who either own or purchase a copy of
the book specified above, to use this software for any purpose, including commercial
applications subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that
you wrote the original software. If you use this software in a product, an acknowledgment
in the product would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be misrepresented
as being the original software.

3. This notice may not be removed or altered from any source distribution.

*/

#include "templateApp.h"

#include "chunk.hpp"


#include "glm.hpp"
#include "noise.hpp"
#include "matrix.hpp"
#include "matrix_transform.hpp"
#include "type_ptr.hpp"
#include "textures2.h"

/* The main structure of the template. This is a pure C struct, you initialize the structure
   as demonstrated below. Depending on the type of your type of app simply comment / uncomment
   which event callback you want to use. */

TEMPLATEAPP templateApp = {
							/* Will be called once when the program start. */
							templateAppInit,
							
							/* Will be called every frame. This is the best location to plug your drawing. */
							templateAppDraw,
							
							/* This function will be triggered when a new touche is recorded on screen. */
							templateAppToucheBegan,
							
							/* This function will be triggered when an existing touche is moved on screen. */
							templateAppToucheMoved,
							
							/* This function will be triggered when an existing touche is released from the the screen. */
							//templateAppToucheEnded,
							
							/* This function will be called everytime the accelerometer values are refreshed. Please take
							not that the accelerometer can only work on a real device, and not on the simulator. In addition
							you will have to turn ON the accelerometer functionality to be able to use it. This will be
							demonstrated in the book later on. */
							//templateAppAccelerometer // Turned off by default.
						  };


#define VERTEX_SHADER ( char * )"vertex.glsl"

#define FRAGMENT_SHADER ( char * )"fragment.glsl"

#define DEBUG_SHADERS 1

PROGRAM *program = NULL;

MEMORY *m = NULL;

chunk *ch = NULL;

superchunk *schunk = NULL;

vec2 touche = { 0.0f, 0.0f };

vec3 rot_angle = { 45.0f, -45.0f, 0.0f };



static int init_resources() {
	/* Create shaders */
    
//	program = create_program("glescraft.v.glsl", "glescraft.f.glsl");
    
//	if(program == 0)
//		return 0;
    
    
	attribute_coord = PROGRAM_get_vertex_attrib_location(program, ( char * )"coord");
	uniform_mvp = PROGRAM_get_uniform_location(program, ( char * )"mvp");
    uniform_texture = PROGRAM_get_uniform_location(program, ( char * )"texture");
    
	if(attribute_coord == -1 || uniform_mvp == -1)
		return 0;
    
	/* Create and upload the texture */
    
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures.width, textures.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textures.pixel_data);
	glGenerateMipmap(GL_TEXTURE_2D);
    
	/* Create the world */
    
	world = new superchunk;
    
	position = glm::vec3(0, CY + 1, 0);
	_angle = glm::vec3(0, -0.5, 0);
	update_vectors();
    
	/* Create a VBO for the cursor */
    
	glGenBuffers(1, &cursor_vbo);
    
	/* OpenGL settings that do not change while running this program */
    
	glUseProgram(program->pid);
	glUniform1i(uniform_texture, 0);
	glClearColor(0.6, 0.8, 1.0, 0.0);
	glEnable(GL_CULL_FACE);
    
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Use GL_NEAREST_MIPMAP_LINEAR if you want to use mipmaps
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
	glPolygonOffset(1, 1);
    
	glEnableVertexAttribArray(attribute_coord);
    
	return 1;
}


void templateAppInit( int width, int height )
{
	// Setup the exit callback function.
	atexit( templateAppExit );
	
	// Initialize GLES.
	GFX_start();
	
	// Setup a GLES viewport using the current width and height of the screen.
//	glViewport( 0, 0, width, height );
	
	/* Insert your initialization code here */
//    GFX_set_matrix_mode( PROJECTION_MATRIX );
//	{
//		GFX_load_identity();
//		
//		GFX_set_perspective( 45.0f,
//                            ( float )width / ( float )height,
//                            0.01f,
//                            1000.0f,
//                            0.0f );
//		
//		glDisable( GL_CULL_FACE );
//	}
	
    ww = height;
    wh = width;
    	glViewport( 0, 0, ww, wh );
    
	program = PROGRAM_init( ( char * )"default" );
	
	program->vertex_shader = SHADER_init( VERTEX_SHADER, GL_VERTEX_SHADER );
    
	program->fragment_shader = SHADER_init( FRAGMENT_SHADER, GL_FRAGMENT_SHADER );
	
	m = mopen( VERTEX_SHADER, 1 );
	
	if( m ) {
        
		if( !SHADER_compile( program->vertex_shader,
                            ( char * )m->buffer,
                            DEBUG_SHADERS ) ) exit( 1 );
	}
	m = mclose( m );
    
	m = mopen( FRAGMENT_SHADER, 1 );
	
	if( m ) {
        
		if( !SHADER_compile( program->fragment_shader,
                            ( char * )m->buffer,
                            DEBUG_SHADERS ) ) exit( 2 ); 
	}
    
	m = mclose( m );
    
    if( !PROGRAM_link( program, DEBUG_SHADERS ) ) exit( 3 );
    
    init_resources();
    
//    initializeChunk();
}

static void display() {
	glm::mat4 view = glm::lookAt(position, position + lookat, up);
	glm::mat4 projection = glm::perspective(45.0f, 1.0f*ww/wh, 0.01f, 1000.0f);
//    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), (float)M_PI/2, glm::vec3(0.0, 0.0, 1.0));
//    glm::mat4 rotation_y = glm::rotate(glm::mat4(1.0f), rot_angle.y, glm::vec3(1.0, 0.0, 0.0));
    
    glm::mat4 mvp = projection * view;
    
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
    
	/* Then draw chunks */
    static const float movespeed = 0.5;
    
    static unsigned int start = get_milli_time();
    unsigned int t = get_milli_time();
    float dt = (t - start) * 1.0e-3;
    start = t;
    position += forward * movespeed * dt;
    
    
	world->render(mvp);
}

void templateAppDraw( void )
{
    static unsigned int start = get_milli_time(),
    fps = 0;
    if( get_milli_time() - start >= 1000 ) {
        console_print("FPS: %d\n", fps );
        start = get_milli_time();
        fps = 0;
    }
    ++fps;
    
    
    display();
    
}


void templateAppToucheBegan( float x, float y, unsigned int tap_count ) {
    
	touche.x = x;
	touche.y = y;
}


void templateAppToucheMoved( float x, float y, unsigned int tap_count ) {
    
//	rot_angle.y += -( touche.x - x );
//	rot_angle.x += -( touche.y - y );
//    
//	touche.x = x;
//	touche.y = y;
    
    
    
    static const float mousespeed = 0.001;
    
//    _angle.x -= (x - ww / 2) * mousespeed;
//    _angle.y -= (y - wh / 2) * mousespeed;
    
    _angle.x += -( touche.x - x ) * mousespeed;
	_angle.y += -( touche.y - y ) * mousespeed;
    
    
    if(_angle.x < -M_PI)
        _angle.x += M_PI * 2;
    if(_angle.x > M_PI)
        _angle.x -= M_PI * 2;
    if(_angle.y < -M_PI / 2)
        _angle.y = -M_PI / 2;
    if(_angle.y > M_PI / 2)
        _angle.y = M_PI / 2;
    
    update_vectors();
    
	touche.x = x;
	touche.y = y;
}



void templateAppToucheEnded( float x, float y, unsigned int tap_count )
{
	/* Insert code to execute when a touche is removed from the screen. */
}


void templateAppAccelerometer( float x, float y, float z )
{
	/* Insert code to execute with the accelerometer values ( when available on the system ). */
}


void templateAppExit( void )
{
	/* Code to run when the application exit, perfect location to free everything. */
    if( program && program->vertex_shader )
		program->vertex_shader = SHADER_free( program->vertex_shader );
    
	if( program && program->fragment_shader )
		program->fragment_shader = SHADER_free( program->fragment_shader );
    
	if( program )
		program = PROGRAM_free( program );
    
    if (schunk) {
        delete schunk;
    }
}





