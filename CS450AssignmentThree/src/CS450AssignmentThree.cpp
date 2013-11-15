// Simple 2D OpenGL Program

//Includes vec, mat, and other include files as well as macro defs
#define GL3_PROTOTYPES

// Include the vector and matrix utilities from the textbook, as well as some
// macro definitions.
#include "Angel.h"
#include <stdio.h>
#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#endif

// My includes
#include <sstream>
#include <iostream>
#include <fstream>
#include "Obj.h"
#include "SceneLoader.h"

using namespace std;
// globals
const string DATA_DIRECTORY_PATH = "./Data/";

Obj *GROUND_QUAD = new Obj();
Obj manips[3];
typedef Angel::vec4  color4;
typedef Angel::vec4  point4;


// global variables
GLuint  gModelViewLoc;  // model-view matrix uniform shader variable location
GLuint  gProjectionLoc; // projection matrix uniform shader variable location

vector<Obj*> obj_data;


// copied from example
//Selection variables
GLuint gSelectionColorR, gSelectionColorG, gSelectionColorB, gSelectionColorA;
int gPicked = -1;
GLint gFlag = 0;
GLuint gSelectFlagLoc;
GLuint gSelectColorRLoc, gSelectColorGLoc, gSelectColorBLoc, gSelectColorALoc;


GLuint gProgram;
GLint gVertLoc, gNormLoc, gColorLoc;

// which manipulator is being dragged
enum manip {
	NO_MANIP_HELD,
	X_HELD,
	Y_HELD,
	Z_HELD
};
manip held;
int last_x, last_y;

enum obj_mode {
	OBJ_TRANSLATE,
	OBJ_ROTATE,
	OBJ_SCALE
};
enum camera_mode {
	CAMERA_ROT_X,
	CAMERA_ROT_Y,
	CAMERA_ROT_Z,
	CAMERA_TRANSLATE,
	CAMERA_DOLLY
};
obj_mode gCurrentObjMode = OBJ_TRANSLATE;
camera_mode gCurrentCameraMode = CAMERA_TRANSLATE;

enum menu_val {
	ITEM_NEW_OBJ,
	ITEM_OBJ_TRANSLATION,
	ITEM_OBJ_ROTATION,
	ITEM_SCALE,
	ITEM_CAMERA_ROT_X,
	ITEM_CAMERA_ROT_Y,
	ITEM_CAMERA_ROT_Z,
	ITEM_CAMERA_TRANSLATION,
	ITEM_DOLLY
};

int load_scene_by_file(string filename, vector<string>& obj_filename_list)
{
	ifstream input_scene_file;
	string line;
	string filepath;
	int status = -1;

	filepath = DATA_DIRECTORY_PATH + filename;

	input_scene_file.open(filepath);
	if(input_scene_file.is_open())
	{
		getline(input_scene_file, line);
		while(!input_scene_file.eof())
		{
			getline(input_scene_file, line);
			obj_filename_list.push_back(line);
			cout << line << endl;
		}
		status = 0;
		input_scene_file.close();
	} else {
		status = -1;
	}
	return status;
}


// does all vao/vbo setup for obj_data[i]
void
setup_obj(int i) {
	// Create a vertex array object
	glGenVertexArrays(1, &obj_data[i]->vao);
	glBindVertexArray(obj_data[i]->vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);

	// set up colors for selection
	obj_data[i]->selectionR = i;
	obj_data[i]->selectionG = i;
	obj_data[i]->selectionB = i;
	obj_data[i]->selectionA = 255; // only seems to work at 255
		
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLsizei num_bytes_vert_data = sizeof(GLfloat) * obj_data[i]->data_soa.positions.size();
	GLsizei num_bytes_norm_data = sizeof(GLfloat) * obj_data[i]->data_soa.normals.size();
	GLvoid *vert_data = obj_data[i]->data_soa.positions.data();
	GLvoid *norm_data = obj_data[i]->data_soa.normals.data();
	glBufferData(GL_ARRAY_BUFFER, num_bytes_vert_data + num_bytes_vert_data, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_bytes_vert_data, vert_data);
	glBufferSubData(GL_ARRAY_BUFFER, num_bytes_vert_data, num_bytes_norm_data, norm_data);
		
	
	glEnableVertexAttribArray(gVertLoc);
	glVertexAttribPointer(gVertLoc, obj_data[i]->data_soa.positions_stride, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);
	glEnableVertexAttribArray(gNormLoc);
	glVertexAttribPointer(gNormLoc, obj_data[i]->data_soa.normals_stride, GL_FLOAT, GL_FALSE, 0, (GLvoid *) num_bytes_vert_data);
}

// menu callback
void menu(int num){
	switch(num) {
	case ITEM_NEW_OBJ:
		{
			char buffer[256];
			printf("Please enter .obj filename in data directory:\n");
			cin.getline(buffer, 256);
			printf("input was %s\n", buffer);

			auto attempt = new Obj(DATA_DIRECTORY_PATH + buffer);
			if(attempt->bad_file || !attempt->is_loaded) {
				printf("Bad file selected, check file path\n");
			}
			else {
				obj_data.push_back(attempt);

				int i = obj_data.size() - 1;
				
				// for some reason only part of this loop can be put into a function. WTF
				setup_obj(i);
			}
		}
		break;
	case ITEM_OBJ_TRANSLATION:
		gCurrentObjMode = OBJ_TRANSLATE;
		break;
	case ITEM_OBJ_ROTATION:
		gCurrentObjMode = OBJ_ROTATE;
		break;
	case ITEM_SCALE:
		gCurrentObjMode = OBJ_SCALE;
		break;
	case ITEM_CAMERA_ROT_X:
		gCurrentCameraMode = CAMERA_ROT_X;
		break;
	case ITEM_CAMERA_ROT_Y:
		gCurrentCameraMode = CAMERA_ROT_Y;
		break;
	case ITEM_CAMERA_ROT_Z:
		gCurrentCameraMode = CAMERA_ROT_Z;
		break;
	case ITEM_CAMERA_TRANSLATION:
		gCurrentCameraMode = CAMERA_TRANSLATE;
		break;
	case ITEM_DOLLY:
		gCurrentCameraMode = CAMERA_DOLLY;
		break;
	}

	glutPostRedisplay();
} 

// building menus
void build_menus(void) {
	int menu_id;
	int obj_submenu_id;
	int camera_submenu_id;
	int rot_submenu_id;


	obj_submenu_id = glutCreateMenu(menu);
	glutAddMenuEntry("Translation", ITEM_OBJ_TRANSLATION);
	glutAddMenuEntry("Rotation", ITEM_OBJ_ROTATION);
	glutAddMenuEntry("Scale", ITEM_SCALE);
	
	rot_submenu_id = glutCreateMenu(menu);
	glutAddMenuEntry("X", ITEM_CAMERA_ROT_X);
	glutAddMenuEntry("Y", ITEM_CAMERA_ROT_Y);
	glutAddMenuEntry("Z", ITEM_CAMERA_ROT_Z);

	camera_submenu_id = glutCreateMenu(menu);
	glutAddSubMenu("Rotation", rot_submenu_id);
	glutAddMenuEntry("Translation", ITEM_CAMERA_TRANSLATION);
	glutAddMenuEntry("Dolly", ITEM_DOLLY);
	
	menu_id = glutCreateMenu(menu);
	glutAddMenuEntry("Load .obj File", ITEM_NEW_OBJ);
	glutAddSubMenu("Object Transformation", obj_submenu_id);
	glutAddSubMenu("Camera Transformation", camera_submenu_id);
 
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}



void
init_manips(void) {
	for(int i = 0; i < 3; i++) {
		manips[i] = Obj(DATA_DIRECTORY_PATH + "axis.obj");

		
		// setup vao and two vbos for manipulators
		glGenVertexArrays(1, &manips[i].vao);
		glBindVertexArray(manips[i].vao);
		GLuint manips_buffer[2]; // 0 is vertices, 1 is colors
		glGenBuffers(2, manips_buffer);

		manips[i].data_soa.colors_stride = 4;
		for(int j = 0; j < manips[i].data_soa.num_vertices; j++) {
			manips[i].data_soa.colors.push_back(i == 0 ? 1.0 : 0.0);
			manips[i].data_soa.colors.push_back(i == 1 ? 1.0 : 0.0);
			manips[i].data_soa.colors.push_back(i == 2 ? 1.0 : 0.0);
			manips[i].data_soa.colors.push_back(1.0);
		}

		
		// vertices
		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * manips[i].data_soa.positions.size(), manips[i].data_soa.positions.data(), GL_STATIC_DRAW);
		// colors
		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * manips[i].data_soa.colors.size(), manips[i].data_soa.colors.data(), GL_STATIC_DRAW);

		// color added to shader. only displays with flag == 2
		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[0]);
		glEnableVertexAttribArray(gVertLoc);
		glVertexAttribPointer(gVertLoc, manips[i].data_soa.positions_stride, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[1]);
		glEnableVertexAttribArray(gColorLoc);
		glVertexAttribPointer(gColorLoc, manips[i].data_soa.colors_stride, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	}

}

// OpenGL initialization
void
init(mat4 projection)
{
    // Load shaders and use the resulting shader program
	// doing this ahead of time so we can use it for setup of special objects
    gProgram = InitShader( "./src/vshader.glsl", "./src/fshader.glsl" );
    glUseProgram(gProgram);
	gVertLoc = glGetAttribLocation(gProgram, "vPosition");
	gNormLoc = glGetAttribLocation(gProgram, "vNormal");
	gColorLoc = glGetAttribLocation(gProgram, "vColor");

	// build the special objects not loaded by user
	init_manips();	

	
	
	for( int i = 0; i < obj_data.size(); i++ )
	{
		// for some reason only part of this loop can be put into a function. WTF
		setup_obj(i);
	}

	
	int linked;
	glGetProgramiv(gProgram, GL_LINK_STATUS, &linked);
	if( linked != GL_TRUE )
	{
		int maxLength;
		glGetProgramiv(gProgram, GL_INFO_LOG_LENGTH, &maxLength);
		maxLength = maxLength + 1;
		GLchar *pLinkInfoLog = new GLchar[maxLength];
		glGetProgramInfoLog(gProgram, maxLength, &maxLength, pLinkInfoLog);
		cerr << *pLinkInfoLog << endl;
	}

    // Initialize shader lighting parameters
    // RAM: No need to change these...we'll learn about the details when we
    // cover Illumination and Shading
    point4 light_position(0., 1.25, 1., 1.0);
    color4 light_ambient(0.2, 0.2, 0.2, 1.0);
    color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
    color4 light_specular(1.0, 1.0, 1.0, 1.0);

    color4 material_ambient(1.0, 0.0, 1.0, 1.0);
    color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
    color4 material_specular(1.0, 0.8, 0.0, 1.0);
    float  material_shininess = 100.0;

    color4 ambient_product = light_ambient * material_ambient;
    color4 diffuse_product = light_diffuse * material_diffuse;
    color4 specular_product = light_specular * material_specular;

    glUniform4fv( glGetUniformLocation(gProgram, "AmbientProduct"),
		  1, ambient_product );
    glUniform4fv( glGetUniformLocation(gProgram, "DiffuseProduct"),
		  1, diffuse_product );
    glUniform4fv( glGetUniformLocation(gProgram, "SpecularProduct"),
		  1, specular_product );

    glUniform4fv( glGetUniformLocation(gProgram, "LightPosition"),
		  1, light_position );

    glUniform1f( glGetUniformLocation(gProgram, "Shininess"),
		 material_shininess );

	
	//Set up selection colors and a gFlag -- copied from example
	gSelectColorRLoc = glGetUniformLocation(gProgram,"selectionColorR");
	gSelectColorGLoc = glGetUniformLocation(gProgram,"selectionColorG");
	gSelectColorBLoc = glGetUniformLocation(gProgram,"selectionColorB");
	gSelectColorALoc = glGetUniformLocation(gProgram,"selectionColorA");
	glUniform1i(gSelectColorRLoc, gSelectionColorR);
	glUniform1i(gSelectColorGLoc, gSelectionColorG);
	glUniform1i(gSelectColorBLoc, gSelectionColorB);
	glUniform1i(gSelectColorALoc, gSelectionColorA);

	gSelectFlagLoc = glGetUniformLocation(gProgram, "flag");
	glUniform1i(gSelectFlagLoc, gFlag);


    gModelViewLoc = glGetUniformLocation(gProgram, "ModelView");
    gProjectionLoc = glGetUniformLocation(gProgram, "Projection");

    point4  eye(0., 0., 1., 1.);
    point4  at(0., 0., 0., 1.);
    vec4    up(0., 1., 0., 0.);

    mat4  mv = LookAt(eye, at, up);

	// objects should know their mv, right?
	manips[0].model_view = mv;
	manips[1].model_view = mv;
	manips[2].model_view = mv;
	for(auto obj : obj_data) {
		obj->model_view = mv;
	}
    //vec4 v = vec4(0.0, 0.0, 1.0, 1.0);

    glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, mv);
    glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, projection);


    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------

void
mouse( int button, int state, int x, int y )
{
	held = NO_MANIP_HELD;
	if(button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) {
		return;
	}

	last_x = x;
	last_y = y;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//render each object, setting the selection RGBA to the objects selection color (RGBA)
	for(int i = 0; i < obj_data.size(); i++) {
		//should store numVerts with vao and possibly the index in the array of objects, instead of storing only ints as I currently am
		//which represent the vaos
		gFlag = 1;

		glBindVertexArray(obj_data[i]->vao);

		gSelectionColorR = obj_data[i]->selectionR;
		gSelectionColorG = obj_data[i]->selectionG;
		gSelectionColorB = obj_data[i]->selectionB;
		gSelectionColorA = obj_data[i]->selectionA;

		//sync with shader
		glUniform1i(gSelectColorRLoc,gSelectionColorR);
		glUniform1i(gSelectColorGLoc,gSelectionColorG);
		glUniform1i(gSelectColorBLoc,gSelectionColorB);
		glUniform1i(gSelectColorALoc,gSelectionColorA);
		glUniform1i(gSelectFlagLoc, gFlag);

		
		//glUniformMatrix4fv(gModelViewLoc, 1, false, obj_data[i]->model_view);

		//Draw the scene.  The gFlag will force shader to not use shading, but instead use a constant color
		glDrawArrays( GL_TRIANGLES, 0, obj_data[i]->data_soa.positions.size() / obj_data[i]->data_soa.positions_stride );
	}

	// check if a manip was clicked
	gFlag = 2;	// change flag to 2, for absolute coloring
	glUniform1i(gSelectFlagLoc, gFlag);
	for(int i = 0; i < 3; i++) {
		mat4 rot = manips[i].model_view;

		// default obj points up y axis
		if(i == 0) {
			rot = Angel::RotateZ(90.0f) * rot;
		}
		else if(i == 2) {
			rot = Angel::RotateX(90.0f) * rot;
		}
				
		// put rot in as model view matrix

		// try uncommenting this line. pretty sure this logic just needs to go elsewhere, not totally sure how uniforms work.
		//glUniformMatrix4fv(gModelViewLoc, 1, false, rot);
		// TODO hey Padraic, I'm trying here to rotate two of the axis objects.
		// I think something like the above line would do it.
		// But I'm also not sure it should be done here rather than in init.
		// this certainly blows things up


		glBindVertexArray(manips[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, manips[i].data_soa.num_vertices);
	}

	// this didn't need to be in the loop
	glutPostRedisplay();  //MUST REMEMBER TO CALL POST REDISPLAY OR IT WON'T RENDER!

	//Now check the pixel location to see what color is found!
	GLubyte pixel[4];
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	//Read as unsigned byte.
	glReadPixels(x, viewport[3] - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	gPicked = -1;

	// TODO: check first if a manipulator was selected. color of (255,0,0) or (0,255,0) or (0,0,255)
	if(ceil(pixel[0]) == 255 && ceil(pixel[1]) == 0 && ceil(pixel[2]) == 0) {
		held = X_HELD;
		return;
	}
	else if(ceil(pixel[0]) == 0 && ceil(pixel[1]) == 255 && ceil(pixel[2]) == 0) {
		held = Y_HELD;
		return;
	}
	else if(ceil(pixel[0]) == 0 && ceil(pixel[1]) == 0 && ceil(pixel[2]) == 255) {
		held = Z_HELD;
		return;
	}


	for(int i=0; i < obj_data.size(); i++) {
		if(obj_data[i]->selectionR == ceil(pixel[0]) && obj_data[i]->selectionG == pixel[1]
			&& obj_data[i]->selectionB == pixel[2]&& obj_data[i]->selectionA == pixel[3]) {
			gPicked = i;
			obj_data[i]->selected = true;
		}
		else {
			obj_data[i]->selected = false;
		}
	}

	//printf("Picked  == %d\n", gPicked);
	// uncomment below to see the color render
	// Swap buffers makes the back buffer actually show...in this case, we don't want it to show so we comment out.
	// For debugging, you can uncomment it to see the render of the back buffer which will hold your 'fake color render'
	//glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
display( void )
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	for( auto obj : obj_data )
	{
		//Normal render so set selection Flag to 0
		gFlag = 0;
		glUniform1i(gSelectFlagLoc, gFlag);

		if(obj->selected == true) {
			// draw manipulators here
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			gFlag = 2;	// change flag to 2, for absolute coloring
			glUniform1i(gSelectFlagLoc, gFlag);
			for(int i = 0; i < 3; i++) {
				mat4 rot = manips[i].model_view;

				// default obj points up y axis
				if(i == 0) {
					rot = Angel::RotateZ(90.0f) * rot;
				}
				else if(i == 2) {
					rot = Angel::RotateX(90.0f) * rot;
				}
				
				// put rot in as model view matrix

				// try uncommenting this line. pretty sure this logic just needs to go elsewhere, not totally sure how uniforms work.
				//glUniformMatrix4fv(gModelViewLoc, 1, false, rot);
				// TODO hey Padraic, I'm trying here to rotate two of the axis objects.
				// I think something like the above line would do it.
				// But I'm also not sure it should be done here rather than in init.
				// this certainly blows things up


				glBindVertexArray(manips[i].vao);
				glDrawArrays(GL_TRIANGLES, 0, manips[i].data_soa.num_vertices);
			}
			
			
			//glUniformMatrix4fv(gModelViewLoc, 1, false, obj->model_view);
			// back to normal rendering
			gFlag = 0;
			glUniform1i(gSelectFlagLoc, gFlag);

			// wireframe
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glPolygonOffset(1.0, 2); //Try 1.0 and 2 for factor and units

		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		glBindVertexArray(obj->vao);
		glDrawArrays(GL_TRIANGLES, 0, obj->data_soa.num_vertices);
	}
    glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
motion(int x, int y) {
	int delta_x = last_x - x;
	int delta_y = last_y - y;
	last_x = x;
	last_y = y;

	// so to translate XY mouse vector into an in-universe vector, we need to do two things
	// 1. apply camera transform (for instance, if we're looking down -y, the transform from viewing xy play to xz plane)
	// 2. translate the units of the vector from pixels into our arbitrary model units 
	//		(think of doing this with camera in default location. on-screen xy must be turned into in-universe xy in a unit-consistent way. it's possible that this involves the view volume.)

	vec3 world_transform = /* TODO some awesome function instead */ vec3(delta_x, delta_y, 0.0f);

	// Padraic: check my assumptions about the perpendicular vs parallel components of the mouse vector needed for transformations
	// these are just a rough idea of what's necessary and may not line up 100% with the actual math
	GLfloat dx, dy, dz, currx, curry, currz, dtheta, theta, dscalex, dscaley, dscalez, scalex, scaley, scalez;
	dx = dy = dz = dtheta = dscalex = dscaley = dscalez = .01;
	mat4 translate_xyz = Angel::identity();
	mat4 rotate_xyz = Angel::identity();
	mat4 scale_xyz = Angel::identity();
	Obj *selected_obj = obj_data[0];
	currx = selected_obj->model_view[3][0];
	curry = selected_obj->model_view[3][1];
	currz = selected_obj->model_view[3][2];
	theta = 0.;
	auto Rotate = RotateX;
	
	switch(held) {
	case NO_MANIP_HELD:
		printf("scene dragged %d px in x, %d px in y\n", delta_x, delta_y);
		switch(gCurrentCameraMode) {
		case CAMERA_ROT_X:
			// take the world_transform component perpendicular to the x axis, and turn that into degrees-to-rotate
			break;
		case CAMERA_ROT_Y:
			// take the world_transform component perpendicular to the y axis, and turn that into degrees-to-rotate
			break;
		case CAMERA_ROT_Z:
			// take the world_transform component perpendicular to the z axis, and turn that into degrees-to-rotate
			break;
		case CAMERA_TRANSLATE:
			// take the world_transform component parallel to the viewing plane, move the camera by that amount
			break;
		case CAMERA_DOLLY:
			// take the world_Transform component perpendicular to the viewing plane, move the camera in/out that much
			break;
		}

		break; // BREAK NO_MANIP_HELD
	case X_HELD:
		printf("manipulator %d dragged %d px in x, %d px in y\n", held, delta_x, delta_y);
		Rotate = RotateX;
		dy = 0.;
		dz = 0.;
		break;
	case Y_HELD:
		printf("manipulator %d dragged %d px in x, %d px in y\n", held, delta_x, delta_y);
		Rotate = RotateY;
		dx = 0.;
		dz = 0.;
		break;
	case Z_HELD:
		printf("manipulator %d dragged %d px in x, %d px in y\n", held, delta_x, delta_y);
		Rotate = RotateZ;
		dx = 0.;
		dy = 0.;
		break; // BREAK Z_HELD
	}
	// at this point the value of 'held'is 0, 1 or 2 - specifying x/y/z axis.
	// so it should be possible to write axis-agnostic code using 'held' as the index for which dimension
	// right????
	switch(gCurrentObjMode) {
	case OBJ_TRANSLATE:
		// take the world_transform component parallel to x/y/z, move object that far
		translate_xyz = Translate( dx, dy, dz );
		break;
	case OBJ_ROTATE:
		// take the world_transform component perpendicular to x/y/z, turn that into degrees-to-rotate
		rotate_xyz = Rotate( theta + dtheta );
		break;
	case OBJ_SCALE:
		// like translate, but scale instead of moving
		scale_xyz = Scale( scalex, scaley, scalez );
		break;
	}
	/*mat4 delta_model_view = translate_xyz * 
		Translate(  selected_obj->model_view[3][0], selected_obj->model_view[3][1], selected_obj->model_view[3][2] ) * 
		rotate_xyz * scale_xyz * Translate( -selected_obj->model_view[3][0], -selected_obj->model_view[3][1], -selected_obj->model_view[3][2] ) * 
		selected_obj->model_view;
	
	selected_obj->model_view = delta_model_view;*/
}

//----------------------------------------------------------------------------

void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
	case 033:  // Escape key
	case 'q': case 'Q':
	    exit( EXIT_SUCCESS );
	    break;
    }
}

//----------------------------------------------------------------------------

int main(int argc, char** argv)
{
	string application_info = "CS450AssignmentThree";
	string *window_title = new string;

	string usage = "Usage:\nCS450AssignmentThree O LEFT RIGHT BOTTOM TOP NEAR FAR\nwhere LEFT RIGHT BOTTOM TOP NEAR and FAR are floating point values that specify the \
orthographic view volume.\nor\nCS450AssignmentThree P FOV NEAR FAR\nwhere FOV is the field of view in degrees, and NEAR and FAR are floating point values that specify the view volume in perspective.\n";
	bool bad_input = false;

	if(argc < 5) {
		bad_input = true;
		cerr << "Not enough arguments.\n";
	}
	
	bool o = false;
	bool p = false;
	o = !string("O").compare(argv[1]);
	p = !string("P").compare(argv[1]);
	
	if(!o && !p) {
		bad_input = true;
		cerr << "1st parameter was neither 'O' nor 'P'.\n";
	}
	else if(o && argc != 8 || p && argc != 5) {
		bad_input = true;
		cerr << "Wrong number of arguments for 'O' or 'P' viewing.\n";
	}
	
	mat4 projection;
	if(o) {
		projection = Ortho(atof(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]), atof(argv[7]));
	}
	else if(p) {
		projection = Perspective(atof(argv[2]), 1.0, atof(argv[3]), atof(argv[4]));
	}


	if(bad_input) {
		cerr << usage;
		cin.get();
		return -1;
	}

	obj_data.push_back(new Obj(DATA_DIRECTORY_PATH + "bunnyS.obj"));

	glutInit(&argc, argv);
#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DEPTH_TEST);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitContextVersion (3, 2);
    glutInitContextFlags (GLUT_FORWARD_COMPATIBLE);
#endif
	window_title->append(application_info);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(500, 300);
    glutCreateWindow(window_title->c_str());
    printf("%s\n%s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    glewInit();
#endif

	init(projection);

    //NOTE:  callbacks must go after window is created!!!
	build_menus();
    glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
    glutDisplayFunc(display);
    glutMainLoop();

    return(0);
}
