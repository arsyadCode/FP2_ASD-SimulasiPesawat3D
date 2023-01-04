#include <iostream>
#include<math.h>


#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>

#define DEG2RAD (M_PI / 180.0) //degree to radian


#include "elinmath.h"

#define attribute_position 0
#define attribute_color    1
#define attribute_normal   2

using namespace std;

typedef struct _vertex_{
    float position[3];
    float color[4]; //R.G.B.A
    float normal[3];
}VERTEX;

typedef struct _structural_mesh_{
    GLuint VAO; //A Vertex Array Object (VAO) is an object which contains one or more Vertex Buffer Objects and is designed to store the information for a complete rendered object
    GLuint index_count; //to count object in index
}MESH;


typedef struct _structural_object_{
    MESH * messh;
    mat4x4 model_matrix; 
    _structural_object_ * next;         
    _structural_object_ * prev;         //========[ASD]===========//
    _structural_object_ * children;     //        Tree            //
    _structural_object_ * parent;       //========================//
}OBJECT;

typedef struct _list_{
    OBJECT * head;              //========[ASD]===========//
    OBJECT * tail;              //    Double Link List    //
    int counter;                //========================//
}LIST;


int InitLibraries() {
    //INITIALIZE SDL
    if (SDL_Init( SDL_INIT_EVERYTHING ) < 0) {
        fprintf(stderr, "Failed to initialize SDL. Error: %s\n",SDL_GetError());
        return -1;
    }
    fprintf( stderr, "SDL Initialized\n" );

    //INITIALIZE SDL_image
    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr,"Failed to initialize SDL_image. Error: %s\n", IMG_GetError());
        SDL_Quit();
        return -2;
    }
    fprintf( stderr, "SDL Image Initialized\n" );

    return 0;
}

SDL_Window *CreateWindow(int width,int height,SDL_GLContext context) {
    //INITIALIZE OPENGL SETTINGS
    // enable double buffering
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

     //set MSAA Buffer
    SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4); //4x MSAA

   //enable accelerated OpenGL rendering
    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );

    //set color depth
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );

    //set OpenGL version to use: OpenGL 3.3 core
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

    //SDL CREATE WINDOW
    SDL_Window * window = NULL;
    window = SDL_CreateWindow( "Tugas Akhir Kelompok 2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
    if (window == NULL) {
        fprintf(stderr, "Failed to create SDL window. Error: %s\n",SDL_GetError());
        SDL_Quit();
    }

    //CREATE OpenGL RENDERING CONTEXT
    context = SDL_GL_CreateContext( window );
    if (context == NULL) {
        fprintf(stderr, "Failed to create OpenGL context. Error: %s\n",SDL_GetError());
        SDL_DestroyWindow( window );
        SDL_Quit();
        return NULL;
    }

    //Initialize GLEW library
    glewExperimental = GL_TRUE; // Needed for core profile
    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW. Error: %s\n", glewGetErrorString(glew_err));
        SDL_GL_DeleteContext( context );
        SDL_DestroyWindow( window );
        SDL_Quit();
        window =  NULL;
    }


    //SET OpenGL operating Parameters
    // enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //set clear screen color
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    //set view port
    glViewport( 0, 0, width, height );

    //enable MSAA
    //glEnable(GL_MULTISAMPLE);

    //enable alpha blending
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    //enable vsync
    SDL_GL_SetSwapInterval(1);

    fprintf(stderr, "SDL2+OpenGL Window and Context Initialized.\n");

    return window;
}

GLuint LoadShader(const char * fname,GLenum shaderType) {
    GLuint shader;

    //READ FILE TO STRING
    FILE *f = fopen(fname, "rb"); //rb = read binaries, typedef struct FILE udah ada darisananya 
    if (f == NULL) {
        fprintf(stderr,"FAILED TO OPEN SHADER FILE %s\n",fname);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    void *string = calloc(fsize + 1, 1);
    if (string == NULL) {
        fprintf(stderr,"FAILED TO ALLOCATE SOURCE BUFFER FOR %s\n",fname);
        fclose(f);
        return 0;
    }
    fread(string, fsize, 1, f);
    fclose(f);

    //CREATE SHADER SLOT
    shader = glCreateShader( shaderType );
    if (shader  == 0) {
        fprintf(stderr,"FAILED TO CREATE SHADER %s\n",fname);
        free(string);
        return 0;
    }

    //SET SHADER SOURCE CODE TO THE STRING READ FROM FILE AND COMPILE
    fprintf( stderr, "COMPILE SHADER %s\n",fname );
    glShaderSource( shader, 1, ( const GLchar ** )&string, NULL );
    glCompileShader( shader );

    free(string);

    //CHECK SHADER COMPILATION STATUS
    GLint status;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
    if( status == GL_FALSE )
    {
        GLsizei logLength;
        GLchar  log[1024];
        glGetShaderInfoLog(	shader, sizeof(log), &logLength, log);
        fprintf( stderr, "SHADER COMPILATION FAILED : %s\n",log );
        return 0;
    }

    fprintf( stderr, "COMPILE SHADER %s COMPLETE\n",fname );
    return shader;
}

GLuint LoadProgram(const char * vshadername, const char * fshadername){
    GLuint program;
    program = glCreateProgram();
    if (program  == 0) {
        fprintf(stderr,"FAILED TO CREATE PROGRAM");
        return 0;
    }

    fprintf( stderr, "LOAD SHADERS\n" );
    GLuint vs = LoadShader(vshadername,GL_VERTEX_SHADER );
    GLuint fs = LoadShader(fshadername,GL_FRAGMENT_SHADER  );

    fprintf( stderr, "ATTACH SHADERS\n" );
    glAttachShader( program, vs );
    glAttachShader( program, fs );

    glLinkProgram( program );

    return program;
}

void readfile_obj(MESH * objek, const char * fname){
    fprintf(stderr,"readfile_obj\n");
    VERTEX  * verteks = (VERTEX *) malloc(1000000 * sizeof(VERTEX));
    unsigned int * indexes = (unsigned int *) malloc(1000000 * sizeof(unsigned int));
    vec3 * normals = (vec3 *) malloc(1000000*sizeof(vec3));
    fprintf(stderr,"open file \n");
    FILE *f=fopen(fname,"r");
    char buffer[1000];
    objek->index_count=0;
    int vrtx_counter=0;
    int normal_counter=0;
    while (fgets(buffer,1000,f) != 0) {
        //fprintf(stderr,"isi baris : %s\n",buffer);
        if(strncmp(buffer,"v ",2)==0){
            float x,y,z;
            int num = sscanf(buffer,"v %f %f %f",&x,&y,&z);
            if (num == 3) {
                verteks[vrtx_counter].position[0]=x;
                verteks[vrtx_counter].position[1]=y;
                verteks[vrtx_counter].position[2]=z;
                verteks[vrtx_counter].color[0] = 0.5;
                verteks[vrtx_counter].color[1] = 0.5;
                verteks[vrtx_counter].color[2] = 0.5;
                verteks[vrtx_counter].color[3] = 1;
                vrtx_counter++;
            }
        }
        else if(strncmp(buffer,"f ",2)==0){
            unsigned int a,b,c,d,e,f,g,h,i;
            int num = sscanf(buffer,"f %u/%u/%u %u/%u/%u %u/%u/%u ",&a,&b,&c,&d,&e,&f,&g,&h,&i);
            if (num == 9) {
                indexes[objek->index_count] = a;
                objek->index_count++;
                indexes[objek->index_count] = d;
                objek->index_count++;
                indexes[objek->index_count] = g;
                objek->index_count++;
                verteks[a].normal[0]=normals[c][0];
                verteks[a].normal[1]=normals[c][1];
                verteks[a].normal[2]=normals[c][2];

                verteks[d].normal[0]=normals[f][0];
                verteks[d].normal[1]=normals[f][1];
                verteks[d].normal[2]=normals[f][2];
                
                verteks[g].normal[0]=normals[i][0];
                verteks[g].normal[1]=normals[i][1];
                verteks[g].normal[2]=normals[i][2];
            }
        }
        else if(strncmp(buffer,"vn ",3)==0){
            float x,y,z;
            int num = sscanf(buffer,"vn %f %f %f",&x,&y,&z);
            if (num == 3) {
                normals[normal_counter][0] = x;
                normals[normal_counter][1] = y;
                normals[normal_counter][2] = z;
                normal_counter++;
            }
        }
        
    }
    fclose(f);
    printf("vertex count : %d index count: %u normals count: %d \n",vrtx_counter,objek->index_count,normal_counter);
    GLuint vao,vbo,ibo;

    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    objek->VAO = vao;
    
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );

    glEnableVertexAttribArray( attribute_position );
    glEnableVertexAttribArray( attribute_color );
    glEnableVertexAttribArray( attribute_normal );

    glVertexAttribPointer( attribute_position, 3, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )0 );
    glVertexAttribPointer( attribute_color,  4, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )(3 * sizeof(float)));
    glVertexAttribPointer( attribute_normal,  3, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )(7 * sizeof(float)));

    glBufferData( GL_ARRAY_BUFFER, vrtx_counter * sizeof(VERTEX), verteks, GL_STATIC_DRAW );

    glGenBuffers( 1, &ibo );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, objek->index_count * sizeof(unsigned int), indexes, GL_STATIC_DRAW);

    free(verteks);
    free(indexes);
    free(normals);
}

void InitFloor(MESH *object) {
    VERTEX g_vertex_buffer_data[] = {
        //{{x,y,z},   {r,g,b,a}}
        /* TOP */
        {{ 20.0,  -2.0,  20.0},  {1, 0, 1, 1}, {0,1,0}}, //FRONT RIGHT
        {{ 20.0,  -2.0, -20.0},  {1, 0, 1, 1}, {0,1,0}}, //REAR RIGHT
        {{-20.0,  -2.0, -20.0},  {1, 0, 1, 1}, {0,1,0}}, //REAR LEFT
        {{-20.0,  -2.0,  20.0},  {1, 0, 1, 1}, {0,1,0}}, //FRONT LEFT

    };

    const unsigned int g_index_buffer_data[] = {
        //TOP
        0,1,2,
        0,2,3,

    };

    GLuint vao,vbo,ibo;

    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    object->VAO = vao;
    
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );

    glEnableVertexAttribArray( attribute_position );
    glEnableVertexAttribArray( attribute_color );
    glEnableVertexAttribArray( attribute_normal );

    glVertexAttribPointer( attribute_position, 3, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )0 );
    glVertexAttribPointer( attribute_color,  4, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )(3 * sizeof(float)));
    glVertexAttribPointer( attribute_normal,  3, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )(7 * sizeof(float)));

    glBufferData( GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, GL_STATIC_DRAW );
    object->index_count = sizeof(g_index_buffer_data)/sizeof(unsigned int);

    glGenBuffers( 1, &ibo );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_index_buffer_data), g_index_buffer_data, GL_STATIC_DRAW);

    

    return;
}



void Initbaling(MESH *object) {
    VERTEX g_vertex_buffer_data[] = {
        //{{x,y,z},   {r,g,b,a}}
        /* TOP */
        {{ 0.0,  3.0,  -1.0},  {1, 0, 1, 1}, {1,0,0}}, //FRONT RIGHT
        {{ 0.0,  -3.0, -1.0},  {1, 0, 1, 1}, {1,0,0}}, //REAR RIGHT
        {{0.0,  -3.0, 1.0},  {1, 0, 1, 1}, {1,0,0}}, //REAR LEFT
        {{0.0,  3.0,  1.0},  {1, 0, 1, 1}, {1,0,0}}, //FRONT LEFT

    };

    const unsigned int g_index_buffer_data[] = {
        //TOP
        0,1,2,
        0,2,3,

    };

    GLuint vao,vbo,ibo;

    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    object->VAO = vao;
    
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );

    glEnableVertexAttribArray( attribute_position );
    glEnableVertexAttribArray( attribute_color );
    glEnableVertexAttribArray( attribute_normal );

    glVertexAttribPointer( attribute_position, 3, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )0 );
    glVertexAttribPointer( attribute_color,  4, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )(3 * sizeof(float)));
    glVertexAttribPointer( attribute_normal,  3, GL_FLOAT, GL_FALSE, sizeof( float ) * 10, ( void * )(7 * sizeof(float)));

    glBufferData( GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, GL_STATIC_DRAW );
    object->index_count = sizeof(g_index_buffer_data)/sizeof(unsigned int);

    glGenBuffers( 1, &ibo );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_index_buffer_data), g_index_buffer_data, GL_STATIC_DRAW);

    

    return;
}


void InitMiniCube(MESH *object) {
    VERTEX g_vertex_buffer_data[] = {
        //{{x,y,z},   {r,g,b,a}}
        /* TOP */
        {{ 0.25,  0.25,  0.25},  {1, 0, 1, 1}, {0,1,0}}, //FRONT RIGHT
        {{ 0.25,  0.25, -0.25},  {1, 0, 1, 1}, {0,1,0}}, //REAR RIGHT
        {{-0.25,  0.05, -0.25},  {1, 0, 1, 1}, {0,1,0}}, //REAR LEFT
        {{-0.5,  0.5,  0.5},  {1, 0, 1, 1}, {0,1,0}}, //FRONT LEFT

        /* BOTTOM */
        {{ 0.5, -0.5,  0.5},  {0, 0, 0, 1}, {0,1,0}}, //FRONT RIGHT
        {{ 0.5, -0.5, -0.5},  {0, 0, 0, 1}, {0,1,0}}, //REAR RIGHT
        {{-0.5, -0.5, -0.5},  {0, 0, 0, 1}, {0,1,0}}, //REAR LEFT
        {{-0.5, -0.5,  0.5},  {0, 0, 0, 1}, {0,1,0}}, //FRONT LEFT

        /* RIGHT WING BOTTOM */
        {{ 0.5, -0.5,  0.5},   {1, 0, 0, 1}, {0,1,0}}, //FRONT RIGHT
        {{ 0.5, -0.5, -0.5},   {1, 0, 0, 1}, {0,1,0}}, //REAR RIGHT

        /* LEFT WING BOTTOM */
        {{-0.5, -0.5, -0.5},   {1, 0, 0, 1}, {0,1,0}}, //REAR LEFT
        {{-0.5, -0.5,  0.5},   {1, 0, 0, 1}, {0,1,0}}, //FRONT LEFT

        /* REAR WING BOTTOM */
        {{ 0.5, -0.5, -0.5},   {1, 0, 0, 1}, {0,1,0}}, //REAR RIGHT
        {{-0.5, -0.5, -0.5},   {1, 0, 0, 1}, {0,1,0}}, //REAR LEFT
    };

    const unsigned int g_index_buffer_data[] = {
        //TOP
        0,1,2,
        0,2,3,

        //BOTTOM
        4,5,6,
        4,6,7,

        //FRONT
        0,4,7,
        0,7,3,

        //REAR
        1,2,13,
        1,13,12,

        //RIGHT
        0,1,9,
        0,9,8,

        //LEFT
        2,3,11,
        2,11,10
    };

    GLuint vao,vbo,ibo;

    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    object->VAO = vao;
    
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );

    glEnableVertexAttribArray( attribute_position );
    glEnableVertexAttribArray( attribute_color );

    glVertexAttribPointer( attribute_position, 3, GL_FLOAT, GL_FALSE, sizeof( float ) * 7, ( void * )0 );
    glVertexAttribPointer( attribute_color,  4, GL_FLOAT, GL_FALSE, sizeof( float ) * 7, ( void * )(3 * sizeof(float)));

    glBufferData( GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, GL_STATIC_DRAW );
    object->index_count = sizeof(g_index_buffer_data)/sizeof(unsigned int);

    glGenBuffers( 1, &ibo );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_index_buffer_data), g_index_buffer_data, GL_STATIC_DRAW);

    

    return;
}



//======================[ ASD ]=====================================//
void initObject(OBJECT * objek,MESH * mess, mat4x4 transmormasi){
    objek->messh=mess;
    mat4x4_dup(objek->model_matrix,transmormasi);
    objek->next = NULL;
    objek->prev = NULL;
    objek->children=NULL;
    objek->parent=NULL;
}


//   initObject adalah fungsi untuk mengisi
//   node baru dengan informasi pembangun sebuah object,
//   yang diisi dari fungsi ini adalah mesh model, dan
//   parameter transformasi gunanya untuk mengkalikan
//   model matriks dari modelnya
//=================================================================//

 
void DrawObject(OBJECT *object,GLuint program, mat4x4 pv_matrix, mat4x4 parent_matrix){
    //ACTIVATE PROGRAM
    glUseProgram( program );

    //PASS THE PV MATRIX TO SHADER
    glUniformMatrix4fv( glGetUniformLocation( program, "u_pv_matrix" ), 1, GL_FALSE, (float *) pv_matrix );


    //PASS MODEL MATRIX TO SHADER
    mat4x4 final_object_matrix;
    mat4x4_mul(final_object_matrix,parent_matrix,object->model_matrix);
    glUniformMatrix4fv( glGetUniformLocation( program, "u_model_matrix" ), 1, GL_FALSE, (float *) final_object_matrix );

    //SET WHICH VAO TO DRAW
    glBindVertexArray( object->messh->VAO );

    //DRAW
    glDrawElements(
        GL_TRIANGLES,      // mode gambar satuan
        object->messh->index_count,  // count
        GL_UNSIGNED_INT,   // type
        (void*)0           // element array buffer offset
    );

    //====================[ ASD ]===============================//
    OBJECT * p=object->children;
    while(p!=NULL){
        DrawObject(p,program,pv_matrix,final_object_matrix);
        p = p->next;
    }
    //  implementasi tree = menggambar object yang ada di anak tree
    //  selama anak tidak null maka object di anak akan digambar  
    //==========================================================//
}


//====================[ ASD ]===========================//
void insert_last(LIST * listt, OBJECT * newobj){
    if(listt->tail!=NULL){
        OBJECT * bungkus = listt->tail;
        listt->tail=newobj;
        bungkus->next=newobj;
        newobj->prev=bungkus;
    }
    else{
        listt->tail=newobj;
        listt->head=newobj;
    }
    listt->counter++;
}

OBJECT * remove_first(LIST * listt){
    OBJECT * bungkus =listt->head;
    if(bungkus!=NULL){
        listt->head=bungkus->next;
        if(listt->head==NULL){
            listt->tail=NULL;
        }
        else{
            listt->head->prev=NULL;
        }
        listt->counter--;
    }
    return bungkus;
}


//  Queue, digunakan untuk peluru
//====================================================//


//====================[ ASD ]=============================//
void insert_children(OBJECT * parent, OBJECT * child){
    child->next=parent->children;
    if(parent->children!=NULL){
        parent->children->prev=child;
    }
    child->prev=NULL;
    parent->children=child;
    child->parent=parent;
}


//  insert object ke cabang tree
//========================================================//

int main(int argc, char * argv[]){

    LIST daftar_peluru;
    daftar_peluru.head=NULL;
    daftar_peluru.tail=NULL;

    //              step 1                       
    //----------[ init library ]-------------//    
    fprintf( stderr, "MULAI\n" );

    InitLibraries();
    if (InitLibraries() < 0) {
        return 1;
    }
    //===========================================//


    //                  step 2
    //============[ run UI/Window ]===========//
    static const int width = 800;
    static const int height = 600;

    SDL_GLContext context = NULL;
    SDL_Window * window = CreateWindow(width,height, context);
    if (window == NULL) {
        SDL_Quit();
        return 1;
    }
    if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0) {
        fprintf( stderr, "FAILED TO SET RELATIVE MOUSE MODE\n" );
        SDL_GL_DeleteContext( context );
        SDL_DestroyWindow( window );
        SDL_Quit();
        return 1;

    }
    printf("step 2\n");
    //============================================//

    //                  step 3
    //===============[ run shaders ]=============//
    GLuint program01 = LoadProgram("shader/shader_vertex_07.glsl","shader/shader_fragment_07.glsl");
    if (program01 < 1) {
        return 1;
    }
    printf("step3\n");
    //==============================================//

    //                  step 4
    //============[ run projection matrix ]===============//
    mat4x4 projection_matrix;
    mat4x4_perspective(projection_matrix,45.0f * DEG2RAD , 4.0f / 3.0f, 0.1f, 100.0f);
    printf("step4\n");
    //================================================//

    //                  step 5
    //=============[ set up camera ]===================//
   

    vec4 cam_rel_pos = {0,5,-18,1};
    vec4 cam_rel_target = {0,4,0,1};


    vec3 plane_front = {0,0,1};
    vec3 plane_up = {0,1,0};
    vec3 plane_right = {-1,0,0};

    vec4 front = {0,0,1,0};
    vec4 up = {0,1,0,0};
    vec4 right = {-1,0,0,0};

    vec3 plane_pos = {0,0,0};
    float plane_x_rot = 0.0f;
    float plane_y_rot = 0.0f;

    mat4x4 view_matrix;
    printf("step5\n");
    //=================================================//

    mat4x4 temp_matrix;
    MESH model_01;
    printf("jj\n" );
    readfile_obj(&model_01,"Intergalactic_Spaceship.obj");
    OBJECT pesawat;
    mat4x4_identity(temp_matrix);
    initObject(&pesawat,&model_01,temp_matrix);
    fprintf( stderr, "5.2\n" );

    MESH model_terrain;
    readfile_obj(&model_terrain,"MountainTerrain.obj");
    OBJECT terrain;
    initObject(&terrain,&model_terrain,temp_matrix);
    
    MESH model_02;
    printf("jj\n" );
    Initbaling(&model_02);
    OBJECT baling_1,baling_2,baling_3;
    
    mat4x4_identity(temp_matrix);
    initObject(&baling_1,&model_02,temp_matrix);
    initObject(&baling_2,&model_02,temp_matrix);
    initObject(&baling_3,&model_02,temp_matrix);
    mat4x4_translate(baling_2.model_matrix,-5,0,0);
    mat4x4_translate(baling_3.model_matrix,5,0,0);

    insert_children(&pesawat,&baling_1);
    insert_children(&pesawat,&baling_2);
    insert_children(&pesawat,&baling_3);

    MESH model_peluru;
    InitMiniCube(&model_peluru);
    fprintf( stderr, "5.3\n" );

    
    printf("step6\n");

    //======================================//
   

    //==============[ run program event ]================//

    unsigned int last_frame_time = SDL_GetTicks();
    unsigned int last_log_time = last_frame_time;

    
    unsigned int running = 1;
    fprintf( stderr, "MAIN LOOP\n" );
    while (running)
    {
        unsigned int current_frame_time = SDL_GetTicks();
        unsigned int delta_frame_time = current_frame_time - last_frame_time;
        //PROCESS SDL EVENTS
        

        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_KEYUP:
                    if( event.key.keysym.sym == SDLK_ESCAPE )
                        running = 0;
                    //==============[ ASD ]===============/
                    if(  event.key.keysym.sym == SDLK_SPACE ) {
                        OBJECT * peluru=(OBJECT *)malloc(sizeof(OBJECT));
                        initObject(peluru,&model_peluru,pesawat.model_matrix);  
                        if(daftar_peluru.counter>=10){                          
                            OBJECT * p = remove_first(&daftar_peluru);          
                            free(p);                                            
                        }                                                       
                        insert_last(&daftar_peluru,peluru);                     
                    }                                                           
                    break;
                    //
                    // jika pencet space, maka akan dibuat node object baru
                    // bernama peluru, kemudian di lakukan inisialisasi lalu dimasukan
                    // ke queue dengan metode insert last, namun peluru ini hanya dibatasi
                    // 10 biji, jika lebih maka akan dilakukan dequeue dengan metode remove first
                    //=====================================//
                case SDL_MOUSEMOTION:
                    //ROTATE CAMERA ON MOUSE MOVE
                     plane_x_rot +=  ((float)event.motion.yrel / 1000.0 );
                     if (plane_x_rot > M_PI_4) {
                         plane_x_rot = M_PI_4;
                     }
                     else if (plane_x_rot < -M_PI_4) {
                         plane_x_rot = -M_PI_4;
                     }

                    plane_y_rot +=  ((float)event.motion.xrel / 1000.0 );
                    plane_y_rot = (float) fmod(plane_y_rot, 2 * M_PI);

                    mat4x4 plane_rot;
                    mat4x4_identity(plane_rot);
                    mat4x4_rotate_X(temp_matrix,plane_rot,plane_x_rot);
                    mat4x4_rotate_Y(plane_rot,temp_matrix,plane_y_rot);
                    mat4x4_dup(pesawat.model_matrix,plane_rot);

                    vec4 resultant;

                    mat4x4_mul_vec4(resultant,plane_rot, front);
                    plane_front[0] = resultant[0];
                    plane_front[1] = resultant[1];
                    plane_front[2] = resultant[2];

                    mat4x4_mul_vec4(resultant,plane_rot, up);
                    plane_up[0] = resultant[0];
                    plane_up[1] = resultant[1];
                    plane_up[2] = resultant[2];

                    mat4x4_mul_vec4(resultant,plane_rot, right);
                    plane_right[0] = resultant[0];
                    plane_right[1] = resultant[1];
                    plane_right[2] = resultant[2];

                    break;

            }
            //printf("step7\n");
        }

        const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );

        //MOVE CAMERA ON KEYBOARD PRESSES
        if( currentKeyStates[ SDL_SCANCODE_W ] ) {
            vec3 resultant;
            vec3 delta;
            vec3_scale(delta,plane_front,(float)delta_frame_time * 10.0f / 1000);
            vec4_add(resultant,plane_pos,delta);
            vec4_dup(plane_pos,resultant);
        }

        if( currentKeyStates[ SDL_SCANCODE_S ] ) {
            vec3 resultant;
            vec3 delta;
            vec3_scale(delta,plane_front,(float) delta_frame_time * 10.0f / 1000);
            vec3_sub(resultant,plane_pos,delta);
            vec3_dup(plane_pos,resultant);
        }
        
        if( currentKeyStates[ SDL_SCANCODE_A ] ) {
            vec4 resultant;
            vec3 delta;
            vec3_scale(delta,plane_right,(float)delta_frame_time * 10.0f / 1000);
            vec3_sub(resultant,plane_pos,delta);
            vec3_dup(plane_pos,resultant);
        }

        if( currentKeyStates[ SDL_SCANCODE_D ] ) {
            vec3 resultant;
            vec3 delta;
            vec3_scale(delta,plane_right,(float)delta_frame_time * 10.0f / 1000);
            vec3_add(resultant,plane_pos,delta);
            vec3_dup(plane_pos,resultant);
        }

        

        if(  currentKeyStates[ SDL_SCANCODE_C ] ) {
            vec4 resultant;
            vec3 delta;
            vec3_scale(delta,plane_up,(float)delta_frame_time * 10.0f / 1000);
            vec3_sub(resultant,plane_pos,delta);
            vec3_dup(plane_pos,resultant);
        }


        if (plane_pos[1] < -2) {
            plane_pos[1] = -2;
        }

        for (int i = 0; i <3; i++) {
            if (plane_pos[i] > 500 ) {
                plane_pos[i] = 500;
            }
            else if (plane_pos[i] < -500 ) {
                plane_pos[i] = -500;
            }
            pesawat.model_matrix[3][i] = plane_pos[i];
        }

        
        //PREPARE PROJECTION * VIEW MATRIX
        vec4 cam_pos4;
        vec4 cam_target4;
        vec3 cam_position;
        vec3 cam_target;

        mat4x4_mul_vec4(cam_pos4,pesawat.model_matrix,cam_rel_pos);
        mat4x4_mul_vec4(cam_target4,pesawat.model_matrix,cam_rel_target);

        for (int i = 0; i <3; i++) {
            cam_position[i] = cam_pos4[i];
            cam_target[i] = cam_target4[i];
        }
        vec3 depan={0,0,1};
        vec3 atas={0,1,0};
        vec3 jarak={0,1,-20};
        vec3_add(cam_position,plane_pos,jarak);
        vec3_add(cam_target,cam_position,depan);
        mat4x4_look_at(view_matrix,
            cam_position, // Camera is at (4,3,-3), in World Space
            cam_target, // and looks at the origin
            atas  // Head is up (set to 0,-1,0 to look upside-down)
        );
        mat4x4 pv_matrix;
        mat4x4_mul(pv_matrix,projection_matrix,view_matrix);


        float delta_sudut_baling= fmod(delta_frame_time*60/1000,2*M_PI);
        mat4x4 id;
        mat4x4_identity(id);
        mat4x4 rot_baling;
        mat4x4_rotate_X(rot_baling,baling_1.model_matrix,delta_sudut_baling);
        mat4x4_dup(baling_1.model_matrix,rot_baling);

        mat4x4_rotate_X(rot_baling,baling_2.model_matrix,delta_sudut_baling);
        mat4x4_dup(baling_2.model_matrix,rot_baling);

        mat4x4_rotate_X(rot_baling,baling_3.model_matrix,delta_sudut_baling);
        mat4x4_dup(baling_3.model_matrix,rot_baling);
        //CLEAR BUFFERpv_matrix
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //DRAW
        
        DrawObject( &terrain,program01,pv_matrix,id);
        DrawObject( &pesawat,program01,pv_matrix,id);
        //==============================[ ASD ]===================================//
        OBJECT * kursorpeluru=daftar_peluru.head;
        while(kursorpeluru!=NULL){
            cout << "1"<<endl;
            vec4 peluru_front;
            cout << "2"<<endl;
            vec4 delta;
            cout << "3"<<endl;
            mat4x4_mul_vec4(peluru_front,kursorpeluru->model_matrix,front);
            cout << "4"<<endl;
            vec4_scale(delta,peluru_front,(float)delta_frame_time * 10.0f / 1000);
            cout << "5"<<endl;
            mat4x4_translate(kursorpeluru->model_matrix,delta[0],delta[1],delta[2]);
            cout << "6"<<endl;
            DrawObject( kursorpeluru,program01,pv_matrix,id);
            cout << "7"<<endl;
            kursorpeluru = kursorpeluru->next;
            cout << "8"<<endl;
        }
        //
        //  settingan program ini adalah selama object peluru di list tidak NULL
        //  maka program akan langsung menggambar peluru akan langsung bergerak kedepan,
        //  arah mengikuti 
        //==========================================================================//
        //SWAP BUFFER
        SDL_GL_SwapWindow( window );
        SDL_Delay( 1 );
        last_frame_time = current_frame_time;
        if (current_frame_time - last_log_time > 2000) {
            fprintf(stderr,"camera position x: %f y: %f z: %f\n",cam_position[0],cam_position[1],cam_position[2]);
            fprintf(stderr,"camera target x: %f y: %f z: %f\n",cam_target[0],cam_target[1],cam_target[2]);
            last_log_time = current_frame_time;
        }
    }

    SDL_GL_DeleteContext( context );
    SDL_DestroyWindow( window );
    SDL_Quit();

    return 0;
}