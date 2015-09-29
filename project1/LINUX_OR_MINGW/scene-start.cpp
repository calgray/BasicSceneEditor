
#include "Angel.h"


//NOTES
//Why the hell does angel want to use degrees instead of radians? 
//Break into comment sections if we have to use a monster source file...
//passing MV and P to shader
//Lighting is gonna be done in view space, but no biggy (better for SSAO i think)
//Previous rotation system is a mess (and focus always 0,0,0), redoing using cam pos, rot (maybe camFocus, dist, rot)

#include "bitmap.h"
#include "bitmap.c"

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
vec3 camRotation = vec3(0, 0, 0); //yaw pitch roll

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

//----Shaders------------------------
GLuint programs[3];
GLuint shaderProgram;


// -----Meshes----------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                                            (numMeshes is defined in gnatidread.h)
aiMesh* meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays

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
    float angles[3]; // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
    vec3 rgb;
    float brightness; // Multiplies all colours
    int meshId;
    int texId;
    float texScale;
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects=0; // How many objects are currenly in the scene.
int currObject=-1; // The current object
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
	if(meshNumber != 1) {
		glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals);
	} else {
		glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals);
	}
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

mat2 camRotZ() {
    std::cout << "cam rot z" << std::endl;
    return rotZ(-camRotation.x) * mat2(10.0, 0, 0, -10.0);
}


//------------Tool Callbacks---------------------
static void adjustBrightnessY(vec2 by) {
#ifdef _DEBUG  
  std::cout << "adjusting brightness(x) position(y)" << std::endl;
#endif
    sceneObjs[toolObj].brightness+=by[0];
    sceneObjs[toolObj].loc[1]+=by[1];
}

static void adjustRedGreen(vec2 rg) {
#ifdef _DEBUG  
  std::cout << "adjusting Red(x) Green(y)" << std::endl;
#endif
    sceneObjs[toolObj].rgb[0]+=rg[0];
    sceneObjs[toolObj].rgb[1]+=rg[1];
}

static void adjustBlueBrightness(vec2 bl_br) {
#ifdef _DEBUG  
  std::cout << "adjusting blue(x) brightness(y)" << std::endl;
#endif
    sceneObjs[toolObj].rgb[2]+=bl_br[0];
    sceneObjs[toolObj].brightness+=bl_br[1];
}

static void adjustAmbientShine(vec2 am_sh){
//#ifdef _DEBUG  
  std::cout << "adjusting ambience(x) shine (y)" << std::endl;
//#endif
	sceneObjs[toolObj].ambient+=am_sh[0];
	sceneObjs[toolObj].shine+=am_sh[1];
}

static void adjustDiffuseSpecular(vec2 df_sp){
//#ifdef _DEBUG  
  std::cout << "adjusting diffusion(x) specularity(y)" << std::endl;
  std::cout << "[" << sceneObjs[toolObj].diffuse << "," << sceneObjs[toolObj].specular << std::endl;
//#endif
	sceneObjs[toolObj].diffuse+=df_sp[0];
	sceneObjs[toolObj].specular+=df_sp[1];
}

static void adjustCamrotsideViewdist(vec2 cv) {
#ifdef _DEBUG
	std::cout << "adjusting cam yaw(x) dist(y)" << std::endl;
	//std::cout << cv << std::endl;
#endif
	camRotation.x += cv[0];
    viewDist += cv[1];
    //camPosition += vec3(cv, 0) / 1000;
}

static void adjustcamSideUp(vec2 su) {
#ifdef _DEBUG
	std::cout << "adjusting cam yaw(x) pitch(y)" << std::endl;
#endif

	//camPosition += vec3(su.x ,0, su.y);
		//cam yaw
    camRotation.x += su[0]; 
	//cam pitch
	camRotation.y += su[1];
}
    
static void adjustLocXZ(vec2 xz) {
#ifdef _DEBUG
    std::cout << "object positionX(x) positionY(y)" << std::endl;
#endif
	sceneObjs[toolObj].loc[0]+=xz[0];
    sceneObjs[toolObj].loc[2]+=xz[1];
}

static void adjustScaleY(vec2 sy) {
#ifdef _DEBUG
    std::cout << "object scale (x) PositionY(y)" << std::endl;
#endif
	sceneObjs[toolObj].scale+=sy[0];
    sceneObjs[toolObj].loc[1]+=sy[1];
}

static void adjustAngleYX(vec2 angle_yx) {
#ifdef _DEBUG  
  std::cout << "object angleX(x) angleY(y)" << std::endl;
#endif
    sceneObjs[currObject].angles[1]+=angle_yx[0];
    sceneObjs[currObject].angles[0]+=angle_yx[1];
}

static void adjustAngleZTexscale(vec2 az_ts) {
#ifdef _DEBUG  
  std::cout << "adjusting angleZ(x) Texscale(y)" << std::endl;
#endif
    sceneObjs[currObject].angles[2] += az_ts[0];
    sceneObjs[currObject].texScale  += az_ts[1];
}

//------Set the mouse buttons to rotate the camera around the centre of the scene. 
static void doRotate() {
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400,0,0,-2), adjustcamSideUp, mat2(400, 0, 0,-90) );
}
                                     
//------Add an object to the scene
static void addObject(int id) {

    vec2 currPos = currMouseXYworld(camRotation.x);
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


//----------- Initialization ---------------------------
void init() {
    srand ( time(NULL) ); /* initialize random seed - so the starting scene varies */
    aiInit();

    //        for(int i=0; i<numMeshes; i++)
    //                meshes[i] = NULL;

    glGenVertexArrays(numMeshes, vaoIDs); CheckError(); // Allocate vertex array objects for meshes
    glGenTextures(numTextures, textureIDs); CheckError(); // Allocate texture objects

    // Load shaders and use the resulting shader program
    programs[0] = InitShader( "vGouraud-Blinn.glsl", "fGouraud-Blinn.glsl" );
	programs[1] = InitShader( "vPhong.glsl", "fPhong-Blinn.glsl" );
	programs[2] = InitShader( "vPhong.glsl", "fPhong.glsl" );
	shaderProgram = programs[2];
	
    glUseProgram(shaderProgram);

    vPosition = glGetAttribLocation( shaderProgram, "vPosition" );
    vNormal = glGetAttribLocation( shaderProgram, "vNormal" );
    vTexCoord = glGetAttribLocation( shaderProgram, "vTexCoord" );

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");

    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[0].loc = vec4(0.0, 0.0, 0.0, 1.0);
    sceneObjs[0].scale = 10.0;
    sceneObjs[0].angles[0] = 90.0; // Rotate it.
    sceneObjs[0].texScale = 5.0; // Repeat the texture.

    addObject(55); // Sphere for the first light
    sceneObjs[1].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[1].scale = 0.1;
    sceneObjs[1].texId = 0; // Plain texture
    sceneObjs[1].brightness = 0.2; // The light's brightness is 5 times this (below).

    addObject(rand() % numMeshes); // A test mesh

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable( GL_DEPTH_TEST );
	//glEnable(GL_DEPTH_CLAMP);
    doRotate(); // Start in camera rotate mode.
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */
}


//--------------Menus----------------------
static void shaderMenu(int id) {
	deactivateTool();
	std::cout << shaderProgram << std::endl;
	std::cout << id << std::endl;
	//shaderProgram = programs[id];
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
        glutPostRedisplay();
    }
}

static void groundMenu(int id) {
    deactivateTool();
    sceneObjs[0].texId = id;
    glutPostRedisplay();
}

static void lightMenu(int id) {
    deactivateTool();
    if(id == 70) {
        toolObj = 1;
        setToolCallbacks(adjustLocXZ, camRotZ(), adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );

    } else if(id>=71 && id<=74) {
        toolObj = 1;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0), adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
    }

    else { printf("Error in lightMenu\n");
        exit(1);
    }
}

static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int)) {
    int nSubMenus = (size-1)/10 + 1;
    int subMenus[nSubMenus];

    for(int i=0; i<nSubMenus; i++) {
        subMenus[i] = glutCreateMenu(menuFn);
        for(int j = i*10+1; j<=std::min(i*10+10, size); j++)
            glutAddMenuEntry( menuEntries[j-1] , j); CheckError();
    }
    int menuId = glutCreateMenu(menuFn);

    for(int i=0; i<nSubMenus; i++) {
        char num[6];
        sprintf(num, "%d-%d", i*10+1, std::min(i*10+10, size));
        glutAddSubMenu(num,subMenus[i]); CheckError();
    }
    return menuId;
}

static void materialMenu(int id) {
    deactivateTool();
    if(currObject<0) return;
    if(id==10) {
        toolObj = currObject;
        setToolCallbacks(adjustRedGreen, mat2(1, 0, 0, 1),
                         adjustBlueBrightness, mat2(1, 0, 0, 1) );
    }
    // You'll need to fill in the remaining menu items here.                                                
    if(id==20){
		toolObj = currObject;
		setToolCallbacks(adjustDiffuseSpecular, mat2(1,0,0,1), adjustAmbientShine, mat2(1,0,0,1) );
	}
    else { printf("Error in materialMenu\n"); }
}

static void mainmenu(int id) {
    deactivateTool();
    if(id == 41 && currObject>=0) {
        toolObj=currObject;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustScaleY, mat2(0.05, 0, 0, 10) );
    }
    if(id == 50)
        doRotate();
    if(id == 55 && currObject>=0) {
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400),
                         adjustAngleZTexscale, mat2(400, 0, 0, 15) );
    }
    if(id == 99) exit(0);
}

static void makeMenu() {
	
	char shaderMenuEntries[3][128] = { "blinn-gouruad", "blinn-phong", "phong" };
	int shaderId = createArrayMenu(3, shaderMenuEntries, shaderMenu);
	
    int objectId = createArrayMenu(numMeshes, objectMenuEntries, objectMenu);

    int materialMenuId = glutCreateMenu(materialMenu);
    glutAddMenuEntry("R/G/B/All",10);
    glutAddMenuEntry("Ambient/Diffuse/Specular/Shine",20);

    int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
    int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);

    int lightMenuId = glutCreateMenu(lightMenu);
    glutAddMenuEntry("Move Light 1",70);
    glutAddMenuEntry("R/G/B/All Light 1",71);
    glutAddMenuEntry("Move Light 2",80);
    glutAddMenuEntry("R/G/B/All Light 2",81);

    glutCreateMenu(mainmenu);	
    glutAddMenuEntry("Rotate/Move Camera",50);
    glutAddSubMenu("Add object", objectId);
    glutAddMenuEntry("Position/Scale", 41);
    glutAddMenuEntry("Rotation/Texture Scale", 55);
		glutAddSubMenu("Shader", shaderId);
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
    else if (button == 3) viewDist = (viewDist < 0.0 ? viewDist : viewDist*0.8) - 0.05;
    
    // scroll down
    else if(button == 4) viewDist = (viewDist < 0.0 ? viewDist : viewDist*1.25) + 0.05;
}

static void mousePassiveMotion(int x, int y) {
    mouseX=x;
    mouseY=y;
}

void keyboard( unsigned char key, int x, int y ) {
	float speed = 0.1f;
  switch ( key ) {
  case 033:
    exit( EXIT_SUCCESS );
    break;
	case 'd':
		camPosition += speed * vec3(cos(camRotation.y * DegreesToRadians),
									0,
									sin(camRotation.y * DegreesToRadians));
		break;
	case 'a':
		camPosition -= speed * vec3(cos(camRotation.y * DegreesToRadians),
									0,
									sin(camRotation.y * DegreesToRadians));
		break;
	case 'w':
		//make scroll wheel do this
		camPosition += speed * vec3(cos(camRotation.x * DegreesToRadians) * sin(camRotation.y * DegreesToRadians),
									-sin(camRotation.x * DegreesToRadians),
									cos(camRotation.x * DegreesToRadians) * -cos(camRotation.y * DegreesToRadians));
		break;
	case 's':
		camPosition -= speed * vec3(cos(camRotation.x * DegreesToRadians) * sin(camRotation.y * DegreesToRadians),
									-sin(camRotation.x * DegreesToRadians),
									cos(camRotation.x * DegreesToRadians) * -cos(camRotation.y * DegreesToRadians));
		break;
	case 'j':
		camRotation.y += 1.0f;
		break;
	case 'l':
		camRotation.y -= 1.0f;
		break;
	case 'i':
		camRotation.x += 1.0f;
		break;
	case 'k':
		camRotation.x -= 1.0f;
		break;
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
    mat4 model = Translate(sceneObj.loc) * RotateX(sceneObj.angles[0]) * RotateY(sceneObj.angles[1]) * RotateZ(sceneObj.angles[2]) * Scale(sceneObj.scale);
    
    // Set the model-view matrix for the shaders
    glUniformMatrix4fv( modelViewU, 1, GL_TRUE, view * model);


    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId);
    glBindVertexArray( vaoIDs[sceneObj.meshId] );

    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3, GL_UNSIGNED_INT, NULL); CheckError();
}

void display( void ) {
    numDisplayCalls++;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//Set the projection matrix for the shaders
    glUniformMatrix4fv( projectionU, 1, GL_TRUE, projection ); CheckError();
		
	//view gets passed via modelview matrix
	//view = RotateX(camRotation.x) * RotateY(camRotation.y) * RotateZ(camRotation.z) * Translate(-camPosition);
	view = Translate(0.0, 0.0, -viewDist) * 
			RotateX(camRotation.y) * 
			RotateY(camRotation.x)*
			Translate(camPosition);
    
	//only uses single point light, bound as object 1
    SceneObject lightObj1 = sceneObjs[1]; 
    vec4 lightPosition = view * lightObj1.loc;
    glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition"), 1, lightPosition); CheckError();

	
    for(int i=0; i<nObjects; i++) {
        SceneObject so = sceneObjs[i];

        vec3 rgb = so.rgb * lightObj1.rgb * so.brightness * lightObj1.brightness * 2.0;
        glUniform3fv( glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, so.ambient * rgb );
        glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, so.diffuse * rgb );
        glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, so.specular * rgb );
		
        glUniform1f( glGetUniformLocation(shaderProgram, "Shininess"), so.shine );
		
		glUniform1f( glGetUniformLocation(shaderProgram, "texScale"), so.texScale );
		
        drawMesh(sceneObjs[i]);
    }

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
		
	float fov = 120.0;
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

    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutIdleFunc( idle );

    glutMouseFunc( mouseClickOrScroll );
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc( doToolUpdateXY );
 
    glutReshapeFunc( reshape );
    glutTimerFunc(1000, timer, 1);

    makeMenu();

    glutMainLoop();
    return 0;
}
