
#include "Angel.h"


//NOTES
//Why the hell does angel want to use degrees instead of radians? 
//Break into comment sections if we have to use a monster source file...
//passing MV and P to shader
//Lighting is gonna be done in view space, but no biggy (better for SSAO i think)
//Previous rotation system is a mess (and focus always 0,0,0), redoing using cam pos, rot (maybe camFocus, dist, rot)

#include "bitmap.h"
#include "bitmap.c"

#include <algorithm>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>

GLint windowHeight=640, windowWidth=960;

#include "gnatidread.h"


// IDs for the GLSL program and GLSL variables.
GLuint vPosition, vNormal, vTexCoord;
GLuint projectionU, modelViewU;
//--------------------------------------------

// Camera ---------------------------
vec3 camPosition = vec3(0, 0, 0);
vec3 camRotation = vec3(40, 0, 0); //pitch yaw roll

//-Z = up

// Camera locked to centre scene
static float viewDist = 1.5; // Distance from the camera to the centre of the scene


mat4 projection; // Projection matrix - set in the reshape function
mat4 view; // View matrix - set in the display function.
//------------------------------------------

// These are used to set the window title
char lab[] = "Project1";
char *programName = NULL; // Set in main 
int numDisplayCalls = 0; // Used to calculate the number of frames per second

//-------Time---------------------
int prevTime = 0;
float deltaTime = 0;


//----Shaders------------------------
GLuint programs[3];
GLuint shaderProgram;


// -----Meshes----------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                                            (numMeshes is defined in gnatidread.h)
aiMesh* meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays

const int GROUND_INDEX = 0;
const int AMBIENT_INDEX = 1;
const int LIGHT1_INDEX = 2;
const int LIGHT2_INDEX = 3;
const int LIGHT3_INDEX = 4;

const int MAX_LIGHTS = 3;

// -----Textures---------------------------------------------------------
//                                            (numTextures is defined in gnatidread.h)
texture* textures[numTextures]; // An array of texture pointers - see gnatidread.h
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures


// ------Scene Objects----------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
typedef struct {
    vec4 loc;
    float scale;
    vec3 angles; // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
	float phi, theta, falloff;
    vec3 rgb;
    float brightness; // Multiplies all colours
	
	int type; //light type
	
	int path; //path type
	float speed; //path speed
	float distance; //path distance
	
	//int shaderId;
    int meshId;
    int texId;
    float texScale;
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects = 0; // How many objects are currenly in the scene.
int currObject =-1; // The current object
int toolObj = -1;    // The object currently being modified


//------ Asset Loading ----------------------------------------------------
// Loads a texture by number, and binds it for later use.    
void loadTextureIfNotAlreadyLoaded(int i) {
    if(textures[i] != NULL) return; // The texture is already loaded.

    textures[i] = loadTextureNum(i);
    glActiveTexture(GL_TEXTURE0);

    // Based on: http://www.opengl.org/wiki/Common_Mistakes
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height,
                             0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    CheckError();
		
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
}


// The following uses the Open Asset Importer library via loadMesh in 
// gnatidread.h to load models in .x format, including vertex positions, 
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.
void loadMeshIfNotAlreadyLoaded(int meshNumber) {

    if(meshNumber>=numMeshes || meshNumber < 0) {
        printf("Error - no such model number");
        exit(1);
    }

    if(meshes[meshNumber] != NULL)
        return; // Already loaded

    aiMesh* mesh = loadMesh(meshNumber);
    meshes[meshNumber] = mesh;

    glBindVertexArray( vaoIDs[meshNumber] );

    // Create and initialize a buffer object for positions and texture coordinates, initially empty.
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
    GLuint buffer[1];
    glGenBuffers( 1, buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*(3+3+3)*mesh->mNumVertices, NULL, GL_STATIC_DRAW );

    int nVerts = mesh->mNumVertices;
    // Next, we load the position and texCoord data in parts.    
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(float)*3*nVerts, mesh->mVertices );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*3*nVerts, sizeof(float)*3*nVerts, mesh->mTextureCoords[0] );
	glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals);
    // Load the element index data
    GLuint elements[mesh->mNumFaces*3];
    for(GLuint i=0; i < mesh->mNumFaces; i++) {
        elements[i*3] = mesh->mFaces[i].mIndices[0];
        elements[i*3+1] = mesh->mFaces[i].mIndices[1];
        elements[i*3+2] = mesh->mFaces[i].mIndices[2];
    }

    GLuint elementBufferId[1];
    glGenBuffers(1, elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements, GL_STATIC_DRAW);

    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0                 
    glVertexAttribPointer( vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( vPosition );

    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
    glVertexAttribPointer( vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*3*mesh->mNumVertices) );
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*6*mesh->mNumVertices) );
    glEnableVertexAttribArray( vNormal );
    CheckError();
}

// --------------------------------------


//------------Tool Callbacks---------------------
inline float clamp(float v, float min, float max) { return v < min ? min : (v > max ? max : v); }

static mat2 camRotZ() {
    return rotZ(-camRotation.y) * mat2(10.0, 0, 0, -10.0);
}

static void adjustBrightnessY(vec2 by) {
    sceneObjs[toolObj].brightness = std::max(0.0f, sceneObjs[toolObj].brightness + by[0]);
    sceneObjs[toolObj].loc[1] += by[1];
}

static void adjustRedGreen(vec2 rg) {
    sceneObjs[toolObj].rgb[0] = std::max(0.0f, sceneObjs[toolObj].rgb[0] + rg[0]);
    sceneObjs[toolObj].rgb[1] = std::max(0.0f, sceneObjs[toolObj].rgb[1] + rg[1]);
}

static void adjustBlueBrightness(vec2 bl_br) {
    sceneObjs[toolObj].rgb[2] = std::max(0.0f, sceneObjs[toolObj].rgb[2] + bl_br[0]);
    sceneObjs[toolObj].brightness = std::max(0.0f, sceneObjs[toolObj].brightness + bl_br[1]);
}

static void adjustAmbientDiffuse(vec2 ad){
    sceneObjs[toolObj].ambient = std::max(0.0f, sceneObjs[toolObj].ambient + ad[0]);
    sceneObjs[toolObj].diffuse = std::max(0.0f, sceneObjs[toolObj].diffuse + ad[1]);
}

static void adjustSpecularShine(vec2 ss){
    sceneObjs[toolObj].specular = std::max(0.0f, sceneObjs[toolObj].specular + ss[0]);
    sceneObjs[toolObj].shine = std::max(0.0f, sceneObjs[toolObj].shine + ss[1]);
}

static void adjustCamrotsideViewdist(vec2 cv) {
	camRotation.y += cv[0];
    viewDist = std::max(0.0f, viewDist + cv[1]);
}

static void adjustCamYawPitch(vec2 su) {
    camRotation.y += su[0];     //cam yaw
	camRotation.x += su[1];     //cam pitch
}
    
static void adjustCamXY(vec2 xy) {
	camPosition += xy[0] * vec3(cos(camRotation.y * DegreesToRadians), 0, sin(camRotation.y * DegreesToRadians));
	
	float sinx = sin(camRotation.x * DegreesToRadians);
	camPosition += xy[1] * vec3(sin(camRotation.y * DegreesToRadians) * sinx,
									cos(camRotation.x * DegreesToRadians),
									-cos(camRotation.y * DegreesToRadians) * sinx);
}
	
static void adjustLocXZ(vec2 xz) {
	sceneObjs[toolObj].loc[0]+=xz[0];
    sceneObjs[toolObj].loc[2]+=xz[1];
}

static void adjustScaleY(vec2 sy) {
	sceneObjs[toolObj].scale+=sy[0];
    sceneObjs[toolObj].loc[1]+=sy[1];
}

static void adjustAngleYX(vec2 angle_yx) {
    sceneObjs[toolObj].angles[1] += angle_yx[0];
    sceneObjs[toolObj].angles[0] += angle_yx[1];
}

static void adjustAngleZTexscale(vec2 az_ts) {
    sceneObjs[toolObj].angles[2] += az_ts[0];
    sceneObjs[toolObj].texScale  += az_ts[1];
}

static void adjustSpeedDist(vec2 sd) {
	sceneObjs[toolObj].speed 	+= sd[0];
    sceneObjs[toolObj].distance += sd[1];
}

static void adjustPhiTheta(vec2 pt) {
	sceneObjs[toolObj].phi = std::max(sceneObjs[toolObj].theta, sceneObjs[toolObj].phi + pt[0]);
	sceneObjs[toolObj].theta = std::max(0.0f, std::min(sceneObjs[toolObj].phi, sceneObjs[toolObj].theta + pt[1]));
}

static void adjustFalloffBrightness(vec2 fb) {
	sceneObjs[toolObj].falloff = std::max(0.0f, sceneObjs[toolObj].falloff + fb[0]);
	sceneObjs[toolObj].brightness = std::max(0.0f, sceneObjs[toolObj].brightness + fb[1]);
}

//------Set the mouse buttons to rotate the camera around the centre of the scene. 
inline void doRotate() {
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400,0,0,-2), adjustCamYawPitch, mat2(400, 0, 0,-90) );
}

//------Add an object to the scene
static void addObject(int id) {

    vec2 currPos = currMouseXYworld(camRotation.y);
    sceneObjs[nObjects].loc[0] = currPos[0];
    sceneObjs[nObjects].loc[1] = 0.0;
    sceneObjs[nObjects].loc[2] = currPos[1];
    sceneObjs[nObjects].loc[3] = 1.0;

    if(id!=0 && id!=55)
        sceneObjs[nObjects].scale = 0.005;

    sceneObjs[nObjects].rgb[0] = 0.7; sceneObjs[nObjects].rgb[1] = 0.7;
    sceneObjs[nObjects].rgb[2] = 0.7; sceneObjs[nObjects].brightness = 1.0;

    sceneObjs[nObjects].diffuse = 1.0; sceneObjs[nObjects].specular = 0.5;
    sceneObjs[nObjects].ambient = 0.7; sceneObjs[nObjects].shine = 10.0;

    sceneObjs[nObjects].angles[0] = 0.0; sceneObjs[nObjects].angles[1] = 180.0;
    sceneObjs[nObjects].angles[2] = 0.0;

    sceneObjs[nObjects].meshId = id;
    sceneObjs[nObjects].texId = rand() % numTextures;
    sceneObjs[nObjects].texScale = 2.0;

    toolObj = currObject = nObjects++;
    setToolCallbacks(adjustLocXZ, camRotZ(), adjustScaleY, mat2(0.05, 0, 0, 10.0) );
    glutPostRedisplay();
}


//--------------Menus----------------------
static void shaderMenu(int id) {
	deactivateTool();
	shaderProgram = programs[id-1];
    
    vPosition = glGetAttribLocation( shaderProgram, "vPosition" );
    vNormal = glGetAttribLocation( shaderProgram, "vNormal" );
    vTexCoord = glGetAttribLocation( shaderProgram, "vTexCoord" );

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");
    
	//glUseProgram(shaderProgram); CheckError();
}

static void objectMenu(int id) {
    deactivateTool();
    addObject(id);
}

static void texMenu(int id) {
    deactivateTool();
    if(currObject>=0) {
        sceneObjs[currObject].texId = id;
        //glutPostRedisplay();
    }
}

static void groundMenu(int id) {
    deactivateTool();
    sceneObjs[GROUND_INDEX].texId = id;
    //glutPostRedisplay();
}

static void pathMenu(int id) {
	deactivateTool();
	if(id < 10) {
		sceneObjs[currObject].path = id;
		sceneObjs[currObject].speed = 1.0f;
		sceneObjs[currObject].distance = 2.0f;
	}
	if(id == 10) {
		toolObj = currObject;
		setToolCallbacks(adjustSpeedDist, mat2(1, 0, 0, 1), adjustSpeedDist, mat2(1, 0, 0, 1));
	}
}

static void lightMenu(int id) {
    deactivateTool();
	switch(id) {
		case 60: {
			toolObj = AMBIENT_INDEX;
			setToolCallbacks(adjustLocXZ, camRotZ(), adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
			break;
		}
		case 61: {
			toolObj = AMBIENT_INDEX;
			setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0), adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
			break;
		}
		case 70: {
			toolObj = LIGHT1_INDEX;
			setToolCallbacks(adjustLocXZ, camRotZ(), adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
			break;
		}
		case 71: {
			toolObj = LIGHT1_INDEX;
			setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0), adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
			break;
		}
		case 72: {
			toolObj = LIGHT1_INDEX;
			setToolCallbacks(adjustAngleYX, mat2(200, 0, 0, -200), adjustAngleYX, mat2(40, 0, 0, -40));
			break;
		}
		case 73: {
			toolObj = LIGHT1_INDEX;
			setToolCallbacks(adjustPhiTheta, mat2(1, 0, 0, 1), adjustFalloffBrightness, mat2(1, 0, 0, 1));
			break;
		}
		case 80: {
			toolObj = LIGHT2_INDEX;
			setToolCallbacks(adjustLocXZ, camRotZ(), adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
			break;
		}
		case 81: {
			toolObj = LIGHT2_INDEX;
			setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0), adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
			break;
		}
		case 82: {
			toolObj = LIGHT2_INDEX;
			setToolCallbacks(adjustAngleYX, mat2(200, 0, 0, -200), adjustAngleYX, mat2(40, 0, 0, -40));
			break;
		}
		case 83: {
			toolObj = LIGHT2_INDEX;
			setToolCallbacks(adjustPhiTheta, mat2(1, 0, 0, 1), adjustFalloffBrightness, mat2(1, 0, 0, 1));
			break;
		}
		case 90: {
			toolObj = LIGHT3_INDEX;
			setToolCallbacks(adjustLocXZ, camRotZ(), adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
			break;
		}
		case 91: {
			toolObj = LIGHT3_INDEX;
			setToolCallbacks(adjustRedGreen,  mat2(1.0, 0, 0, 1.0), adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
			break;
		}
		case 92: {
			toolObj = LIGHT3_INDEX;
			setToolCallbacks(adjustAngleYX, mat2(200, 0, 0, -200), adjustAngleYX, mat2(40, 0, 0, -40));
			break;
		}
		case 93: {
			toolObj = LIGHT3_INDEX;
			setToolCallbacks(adjustPhiTheta, mat2(1, 0, 0, 1), adjustFalloffBrightness, mat2(1, 0, 0, 1));
			break;
		}
		default: {
			printf("Error in lightMenu\n %i", id);
			exit(1);
		}
	}
}

static void lightTypeMenu(int id) {
	switch(id) {
		case 10: {
			sceneObjs[LIGHT1_INDEX].type = 0;
			break;
		}
		case 11: {
			sceneObjs[LIGHT1_INDEX].type = 1;
			break;
		}
		case 12: {
			sceneObjs[LIGHT1_INDEX].type = 2;
			break;
		}
		case 20: {
			sceneObjs[LIGHT2_INDEX].type = 0;
			break;
		}
		case 21: {
			sceneObjs[LIGHT2_INDEX].type = 1;
			break;
		}
		case 22: {
			sceneObjs[LIGHT2_INDEX].type = 2;
			break;
		}
		case 30: {
			sceneObjs[LIGHT3_INDEX].type = 0;
			break;
		}
		case 31: {
			sceneObjs[LIGHT3_INDEX].type = 1;
			break;
		}
		case 32: {
			sceneObjs[LIGHT3_INDEX].type = 2;
			break;
		}
	}
}

static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int)) {
    int nSubMenus = (size-1)/10 + 1;
    int subMenus[nSubMenus];

    for(int i=0; i<nSubMenus; i++) {
        subMenus[i] = glutCreateMenu(menuFn);
        for(int j = i*10+1; j<=std::min(i*10+10, size); j++)
            glutAddMenuEntry( menuEntries[j-1], j); CheckError();
    }
    int menuId = glutCreateMenu(menuFn);

    for(int i=0; i<nSubMenus; i++) {
        char num[6];
        sprintf(num, "%d-%d", i*10+1, std::min(i*10+10, size));
        glutAddSubMenu(num, subMenus[i]); CheckError();
    }
    return menuId;
}

static void materialMenu(int id) {
    deactivateTool();
    if(currObject<0) return;
    if(id==10) {
        toolObj = currObject;
        setToolCallbacks(adjustRedGreen, mat2(1, 0, 0, 1), adjustBlueBrightness, mat2(1, 0, 0, 1) );
    }                                             
    else if(id==20){
		toolObj = currObject;
		setToolCallbacks(adjustAmbientDiffuse, mat2(1,0,0,1), adjustSpecularShine, mat2(1, 0, 0, 50) );
	}
    else { printf("Error in materialMenu\n"); }
}

static void mainmenu(int id) {
    deactivateTool();
    if(id == 41 && currObject>=0) {
        toolObj=currObject;
        setToolCallbacks(adjustLocXZ, camRotZ(), adjustScaleY, mat2(0.05, 0, 0, 10) );
    }
    if(id == 50)
        doRotate();
	if(id == 51)
		setToolCallbacks(adjustCamXY, mat2(1, 0, 0,1), adjustCamYawPitch, mat2(400,0,0,-90));
    if(id == 55 && currObject>=0) {
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400), adjustAngleZTexscale, mat2(400, 0, 0, 15) );
    }
    if(id == 99) exit(0);
}

static void makeMenu() {
	
	char shaderMenuEntries[3][128] = { "blinn-gouruad", "blinn-phong", "phong" };
	int shaderMenuId = createArrayMenu(3, shaderMenuEntries, shaderMenu);
	
    int objectMenuId = createArrayMenu(numMeshes, objectMenuEntries, objectMenu);

    int materialMenuId = glutCreateMenu(materialMenu);
    glutAddMenuEntry("R/G/B/All",10);
    glutAddMenuEntry("Ambient/Diffuse/Specular/Shine",20);

    int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
    int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);
	
	int pathMenuId = glutCreateMenu(pathMenu);
	glutAddMenuEntry("Speed/Distance", 10);
	glutAddMenuEntry("Stop", 0);
	glutAddMenuEntry("Revolve", 1);
	glutAddMenuEntry("Bounce", 2);
	
	int lightTypeMenuId1 = glutCreateMenu(lightTypeMenu);
	glutAddMenuEntry("Directional", 10);
	glutAddMenuEntry("Point", 11);
	glutAddMenuEntry("Spotlight", 12);
	
	int lightTypeMenuId2 = glutCreateMenu(lightTypeMenu);
	glutAddMenuEntry("Directional", 20);
	glutAddMenuEntry("Point", 21);
	glutAddMenuEntry("Spotlight", 22);
	
	int lightTypeMenuId3 = glutCreateMenu(lightTypeMenu);
	glutAddMenuEntry("Directional", 30);
	glutAddMenuEntry("Point", 31);
	glutAddMenuEntry("Spotlight", 32);
	
    int lightMenuId = glutCreateMenu(lightMenu);
	glutAddMenuEntry("Move Light Ambient", 60);
	glutAddMenuEntry("R/G/B/All Light Ambient", 61);
	glutAddSubMenu("Type Light 1", lightTypeMenuId1);
    glutAddMenuEntry("Move Light 1",70);
    glutAddMenuEntry("R/G/B/All Light 1",71);
    glutAddMenuEntry("Yaw/Pitch Light 1",72);
	glutAddMenuEntry("Phi/Theta/Falloff/All Light 1", 73);
	glutAddSubMenu("Type Light 2", lightTypeMenuId2);
    glutAddMenuEntry("Move Light 2",80);
    glutAddMenuEntry("R/G/B/All Light 2",81);
	glutAddMenuEntry("Yaw/Pitch Light 2",82);
	glutAddMenuEntry("Phi/Theta/Falloff/All Light 2", 83);
	glutAddSubMenu("Type Light 3", lightTypeMenuId3);
    glutAddMenuEntry("Move Light 3",90);
    glutAddMenuEntry("R/G/B/All Light 3",91);
	glutAddMenuEntry("Yaw/Pitch Light 3",92);
	glutAddMenuEntry("Phi/Theta/Falloff/All Light 3", 93);

    glutCreateMenu(mainmenu);
    glutAddMenuEntry("Rotate/Move Camera",50);
	glutAddMenuEntry("Rotate/Pan Camera", 51);
    glutAddSubMenu("Add object", objectMenuId);
    glutAddMenuEntry("Position/Scale", 41);
    glutAddMenuEntry("Rotation/Texture Scale", 55);
	glutAddSubMenu("Path", pathMenuId);
	glutAddSubMenu("Shader", shaderMenuId);
    glutAddSubMenu("Material", materialMenuId);
    glutAddSubMenu("Texture",texMenuId);
    glutAddSubMenu("Ground Texture",groundMenuId);
    glutAddSubMenu("Lights",lightMenuId);
    glutAddMenuEntry("EXIT", 99);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}


//------------Input------------------------------------------
static void mouseClickOrScroll(int button, int state, int x, int y) {
  
	//global defined in gnatidread.h ... (bad practice, should only declare extern variable in header and define variable in source)
	prevPos = vec2(currMouseXYscreen(x,y));
  
    if(button==GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    if(glutGetModifiers()!=GLUT_ACTIVE_SHIFT) activateTool(button);
    else activateTool(GLUT_MIDDLE_BUTTON);
    
    else if(button==GLUT_LEFT_BUTTON && state == GLUT_UP) deactivateTool();
    else if(button==GLUT_MIDDLE_BUTTON && state==GLUT_DOWN) activateTool(button);
    else if(button==GLUT_MIDDLE_BUTTON && state==GLUT_UP) deactivateTool();

     // scroll up
    else if (button == 3) viewDist = (viewDist < 0.01 ? viewDist : viewDist*0.8  - 0.05);
    
    // scroll down
    else if(button == 4) viewDist = viewDist*1.25  + 0.05;
}

static void mousePassiveMotion(int x, int y) {
    mouseX=x;
    mouseY=y;
}

void keyboard(unsigned char key, int x, int y ) {		
	switch ( key ) {
	case 033:
		exit( EXIT_SUCCESS );
		break;
	}
}


//----------- Initialization ---------------------------
void init() {
    srand ( time(NULL) ); /* initialize random seed - so the starting scene varies */
    aiInit();

    //        for(int i=0; i<numMeshes; i++)
    //                meshes[i] = NULL;

    glGenVertexArrays(numMeshes, vaoIDs); CheckError(); // Allocate vertex array objects for meshes
    glGenTextures(numTextures, textureIDs); CheckError(); // Allocate texture objects

    // Load shaders and use the resulting shader program
    programs[0] = InitShader( "vGouraud-Blinn.glsl", "fGouraud-Blinn.glsl" ); CheckError();
	programs[1] = InitShader( "vPhong.glsl", "fPhong-Blinn.glsl" ); CheckError();
	programs[2] = InitShader( "vPhong.glsl", "fPhong.glsl" ); CheckError();
	
    shaderMenu(2); CheckError();
    
    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[GROUND_INDEX].loc = vec4(0.0, 0.0, 0.0, 1.0);
    sceneObjs[GROUND_INDEX].scale = 10.0;
    sceneObjs[GROUND_INDEX].angles[0] = 90.0; // Rotate it.
    sceneObjs[GROUND_INDEX].specular = 0.3;
    sceneObjs[GROUND_INDEX].texScale = 5.0; // Repeat the texture.

	addObject(55); //Ambient sphere
	sceneObjs[AMBIENT_INDEX].loc = vec4(0.0, 1.0, 3.0, 1.0);
    sceneObjs[AMBIENT_INDEX].scale = 0.1;
    sceneObjs[AMBIENT_INDEX].texId = 0; // Plain texture
    sceneObjs[AMBIENT_INDEX].brightness = 0.2; // The light's brightness is 5 times this (below).
	
    addObject(55); // Sphere for the first light
    sceneObjs[LIGHT1_INDEX].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[LIGHT1_INDEX].angles = vec3(90, 0, 0);
	sceneObjs[LIGHT1_INDEX].scale = 0.1;
    sceneObjs[LIGHT1_INDEX].texId = 0; // Plain texture
    sceneObjs[LIGHT1_INDEX].brightness = 1.0; // The light's brightness is 5 times this (below).
	sceneObjs[LIGHT1_INDEX].type = 0;
	sceneObjs[LIGHT1_INDEX].phi = 0.5f;
	sceneObjs[LIGHT1_INDEX].theta = 0.2f;
	sceneObjs[LIGHT1_INDEX].falloff = 0.5f;
	
	addObject(55); // Sphere for the second light
    sceneObjs[LIGHT2_INDEX].loc = vec4(-2.0, 1.0, 1.0, 1.0);
	sceneObjs[LIGHT2_INDEX].angles = vec3(90, 0, 0);
    sceneObjs[LIGHT2_INDEX].scale = 0.1;
    sceneObjs[LIGHT2_INDEX].texId = 0; // Plain texture
    sceneObjs[LIGHT2_INDEX].brightness = 1.0; // The light's brightness is 5 times this (below).
	sceneObjs[LIGHT2_INDEX].type = 1;
	sceneObjs[LIGHT2_INDEX].phi = 0.5f;
	sceneObjs[LIGHT2_INDEX].theta = 0.2f;
	sceneObjs[LIGHT2_INDEX].falloff = 0.5f;
	
	addObject(55); // Sphere for the third light
    sceneObjs[LIGHT3_INDEX].loc = vec4(0.0, 1.0, 1.0, 1.0);
	sceneObjs[LIGHT3_INDEX].angles = vec3(90, 0, 0);
    sceneObjs[LIGHT3_INDEX].scale = 0.1;
    sceneObjs[LIGHT3_INDEX].texId = 0; // Plain texture
    sceneObjs[LIGHT3_INDEX].brightness = 1.0; // The light's brightness is 5 times this (below).
	sceneObjs[LIGHT3_INDEX].type = 2;
	sceneObjs[LIGHT3_INDEX].phi = 0.5f;
	sceneObjs[LIGHT3_INDEX].theta = 0.2f;
	sceneObjs[LIGHT3_INDEX].falloff = 0.5f;
	
    addObject(rand() % numMeshes); // A test mesh

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable( GL_DEPTH_TEST );
    
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    
	//glEnable(GL_DEPTH_CLAMP);
    doRotate(); // Start in camera rotate mode.
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */
}

void update()
{
	float time = prevTime/1000.0f;
		
	for (int i = 0; i < nObjects; i++)
	{
		if(sceneObjs[i].path == 1) {
			float angle = sceneObjs[i].speed * time;
			float dist = sceneObjs[i].distance;
			
			sceneObjs[i].loc = vec4(dist * -sin(angle), sceneObjs[currObject].loc.y, dist * -cos(angle), 1);
			sceneObjs[i].angles = vec3(0, angle / DegreesToRadians, 0);
		}
		if(sceneObjs[i].path == 2) {
			float angle = sceneObjs[i].speed * time;
			float dist = sceneObjs[i].distance;
			
			sceneObjs[i].loc = vec4(dist * -sin(angle), 0.5f * std::abs(sin(8 * angle)), dist * -cos(angle), 1);
			sceneObjs[i].angles = vec3(0, angle / DegreesToRadians, 0);
		}
	}
}

//-----------------Draw/Display Callbacks---------------------------------------------------------
void drawMesh(SceneObject sceneObj) {
    // Activate a texture, loading if needed.
    loadTextureIfNotAlreadyLoaded(sceneObj.texId); CheckError();
    glActiveTexture(GL_TEXTURE0 ); CheckError();
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]); CheckError();

    // Texture 0 is the only texture type in this program, and is for the rgb colour of the
    // surface but there could be separate types for, e.g., specularity and normals. 
    glUniform1i( glGetUniformLocation(shaderProgram, "texture"), 0 ); CheckError();
	
	//sceneObj.loc += 0.01f * vec3(1, 0, 0);
		
    // Calculate the model matrix - this should combine translation, rotation and scaling based on what's
    // in the sceneObj structure (see near the top of the program).
    mat4 model = Translate(sceneObj.loc) *
				RotateZ(sceneObj.angles.z) *  //roll pitch yaw
				RotateX(sceneObj.angles.x) *
				RotateY(sceneObj.angles.y) *
				Scale(sceneObj.scale);
    
    // Set the model-view matrix for the shaders
    glUniformMatrix4fv( modelViewU, 1, GL_TRUE, view * model);


    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId);
    glBindVertexArray( vaoIDs[sceneObj.meshId] );

    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3, GL_UNSIGNED_INT, NULL); CheckError();
}

void display( void ) {
    numDisplayCalls++;

	//Update Time variables
	int currTime = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = (currTime - prevTime) / 1000.0f; //in seconds
	prevTime = currTime;
	 
	update();
	
	//view gets passed via modelview matrix
	//view = RotateX(camRotation.y) * RotateY(camRotation.x) * RotateZ(camRotation.z) * Translate(-camPosition);
	view = Translate(0.0, 0.0, -viewDist) * 
			RotateZ(camRotation.z) *
			RotateX(camRotation.x) * 
			RotateY(camRotation.y) *
			Translate(camPosition);
	
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glUseProgram(shaderProgram); CheckError();
	
	//Set the projection matrix for the shaders
    glUniformMatrix4fv( projectionU, 1, GL_TRUE, projection ); CheckError();
	
	glUniform4fv( glGetUniformLocation(shaderProgram, "Origin"), 1, view * vec4(0, 0, 0, 1));
	
	int lightTypes[MAX_LIGHTS] = {
		sceneObjs[LIGHT1_INDEX].type,
		sceneObjs[LIGHT2_INDEX].type,
		sceneObjs[LIGHT3_INDEX].type
	};
	glUniform1iv( glGetUniformLocation(shaderProgram, "LightType"), MAX_LIGHTS, lightTypes); CheckError();
	
	
    vec4 lightPositions[MAX_LIGHTS] = {
		view * sceneObjs[LIGHT1_INDEX].loc,
		view * sceneObjs[LIGHT2_INDEX].loc,
		view * sceneObjs[LIGHT3_INDEX].loc
    };
	glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition"), MAX_LIGHTS, *lightPositions); CheckError();

	
	
	vec4 lightDirections[MAX_LIGHTS] = {
		view * RotateX(sceneObjs[LIGHT1_INDEX].angles.x) * RotateY(sceneObjs[LIGHT1_INDEX].angles.y) * vec4(0, 0, -1, 0),
		view * RotateX(sceneObjs[LIGHT2_INDEX].angles.x) * RotateY(sceneObjs[LIGHT2_INDEX].angles.y) * vec4(0, 0, -1, 0),
		view * RotateX(sceneObjs[LIGHT3_INDEX].angles.x) * RotateY(sceneObjs[LIGHT3_INDEX].angles.y) * vec4(0, 0, -1, 0)
    };
	glUniform4fv( glGetUniformLocation(shaderProgram, "LightDirection"), MAX_LIGHTS, *lightDirections); CheckError();
	
	
	vec3 spotlightVars[MAX_LIGHTS] = {
		vec3(sceneObjs[LIGHT1_INDEX].phi, sceneObjs[LIGHT1_INDEX].theta, sceneObjs[LIGHT1_INDEX].falloff ),
		vec3(sceneObjs[LIGHT2_INDEX].phi, sceneObjs[LIGHT2_INDEX].theta, sceneObjs[LIGHT2_INDEX].falloff ),
		vec3(sceneObjs[LIGHT3_INDEX].phi, sceneObjs[LIGHT3_INDEX].theta, sceneObjs[LIGHT3_INDEX].falloff )
	};
	glUniform3fv( glGetUniformLocation(shaderProgram, "SpotLightVars"), MAX_LIGHTS, *spotlightVars); CheckError();
	
	//Ambient determined by the ambient light source
	vec3 ambient = sceneObjs[AMBIENT_INDEX].rgb * sceneObjs[AMBIENT_INDEX].brightness;
	
    vec3 color[MAX_LIGHTS] = {
		sceneObjs[LIGHT1_INDEX].rgb * sceneObjs[LIGHT1_INDEX].brightness,
		sceneObjs[LIGHT2_INDEX].rgb * sceneObjs[LIGHT2_INDEX].brightness,
		sceneObjs[LIGHT3_INDEX].rgb * sceneObjs[LIGHT3_INDEX].brightness
	};
	
	 vec3 zero[MAX_LIGHTS] = {
		vec3(0,0,0), 
		vec3(0,0,0),
		vec3(0,0,0)
	};
	
    for(int i=0; i<nObjects; i++) {
		
        SceneObject& so = sceneObjs[i];

		vec3 diffuse[MAX_LIGHTS] = {
          so.diffuse * so.rgb * so.brightness * color[0],
          so.diffuse * so.rgb * so.brightness * color[1],
          so.diffuse * so.rgb * so.brightness * color[2]
        };
		
		vec3 specular[MAX_LIGHTS] = { 
			so.specular * color[0], 
			so.specular * color[1],
			so.specular * color[2]
		};
        
		switch(i) {
		case AMBIENT_INDEX:
			glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, ambient);
            glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), MAX_LIGHTS, *zero );
            glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), MAX_LIGHTS, *zero );
			break;
		case LIGHT1_INDEX: 
			glUniform3fv( glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, color[0] );
            glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), MAX_LIGHTS, *zero );
            glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), MAX_LIGHTS, *zero );
			break;
		case LIGHT2_INDEX: 
			glUniform3fv( glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, color[1] );
            glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), MAX_LIGHTS, *zero );
            glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), MAX_LIGHTS, *zero );
			break;
		case LIGHT3_INDEX: 
			glUniform3fv( glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, color[2] );
            glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), MAX_LIGHTS, *zero );
            glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), MAX_LIGHTS, *zero );
			break;
		default:
			glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, so.ambient * ambient);
            glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), MAX_LIGHTS, *diffuse );
            glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), MAX_LIGHTS, *specular );
		}
		
		
        glUniform1f( glGetUniformLocation(shaderProgram, "Shininess"), so.shine );
		glUniform1f( glGetUniformLocation(shaderProgram, "texScale"), so.texScale );
		
        CheckError();
        
        drawMesh(sceneObjs[i]);
    }

    glUseProgram(GL_NONE); CheckError();
    
	CheckError();
    glutSwapBuffers();
}

void idle( void ) {
    glutPostRedisplay();
}

void reshape( int width, int height ) {

    windowWidth = width;
    windowHeight = height;

    glViewport(0, 0, width, height);
		
	float fov = 60.0;
	float aspect = (float)width/(float)height;
	float nearDist = 0.001f;
	float farDist = 100.0f;
	
	//projection = Perspective(fov, aspect, nearDist, farDist);
	
	GLfloat top;
	GLfloat right;
	
	if(aspect > 1.0f) {
		top = tan(fov*DegreesToRadians/2) * nearDist;
		right = top * aspect;
	}
	else {
		right = tan(fov*DegreesToRadians/2) * nearDist;
		top = right / aspect;
	}
	
	projection = mat4();
	projection[0][0] = nearDist/right;
	projection[1][1] = nearDist/top;
	projection[2][2] = -(farDist + nearDist)/(farDist - nearDist);
	projection[2][3] = -2.0f * farDist * nearDist/(farDist - nearDist);
	projection[3][2] = -1.0f;
	projection[3][3] = 0.0f;
}

void timer(int) {
	
    char title[256];
    sprintf(title, "%s %s: %d Frames Per Second @ %d x %d",
                    lab, programName, numDisplayCalls, windowWidth, windowHeight );

    glutSetWindowTitle(title);

    numDisplayCalls = 0;
    glutTimerFunc(1000, timer, 1);
}


//----------Constants--------------------------------
char dirDefault1[] = "models-textures";
char dirDefault3[] = "./res/models-textures";

char dirDefault4[] = "/d/models-textures";
char dirDefault2[] = "/cslinux/examples/CITS3003/project-files/models-textures";

void fileErr(char* fileName) {
    printf("Error reading file: %s\n", fileName);
    printf("When not in the CSSE labs, you will need to include the directory containing\n");
    printf("the models on the command line, or put it in the same folder as the exectutable.");
    exit(1);
}

//----------------------------------------------------------------------------

int main( int argc, char* argv[] )
{
    // Get the program name, excluding the directory, for the window title
    programName = argv[0];
    for(char *cpointer = argv[0]; *cpointer != 0; cpointer++)
        if(*cpointer == '/' || *cpointer == '\\') programName = cpointer+1;

    // Set the models-textures directory, via the first argument or some handy defaults.
    if(argc > 1) strcpy(dataDir, argv[1]);
    else if(opendir(dirDefault1)) strcpy(dataDir, dirDefault1);
    else if(opendir(dirDefault2)) strcpy(dataDir, dirDefault2);
    else if(opendir(dirDefault3)) strcpy(dataDir, dirDefault3);
    else if(opendir(dirDefault4)) strcpy(dataDir, dirDefault4);
    else fileErr(dirDefault1);

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowSize( windowWidth, windowHeight );

    glutInitContextVersion( 3, 1);
    glutInitContextProfile( GLUT_CORE_PROFILE );
    //glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE );
	
    glutCreateWindow( "Initialising..." );

    glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental.
    CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library

    init(); 

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);

    glutMouseFunc( mouseClickOrScroll );
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc( doToolUpdateXY );
 
    glutReshapeFunc( reshape );
    glutTimerFunc(1000, timer, 1);

    makeMenu();

    glutMainLoop();
    return 0;
}
