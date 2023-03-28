/*
 * Computacao Grafica
 * Codigo Exemplo: Renderizacao de Modelo 3D Complexo e Animacoes
 * Autor: Pedro Felipe
 */

/* Inclui os headers do OpenGL, GLU, e GLUT */
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

/* Inclui o arquivo header com a textura */
#include "glm.h"
#include "glut_text.h"
#include <cstdio>
#include <cmath>
#include <map>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <windows.h>      //usada para reproduzir sons de fundo
#include <mmsystem.h>


#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/*
 * Declaracao de constantes e variaveis
 */
#define ESC 27 // ASCII para a tecla ESC
#define SPACE 32 // ASCII para a tecla de ESPACO
#define MAX_FPS 200 // Maximo de Frames Por Segundo (FPS) desejado
#define FPS 100 // FPS desejado atualmente
#define KEYFRAME_RATE 3 // taxa de repeticoes do keyframe da animacao nos quadros da video
// #define POSY -1.04  //posicao do modelo 3D no eixo y
#define POSY 0.25  //posicao do modelo 3D no eixo y
#define BPOSY -1.5  //posicao do modelo 3D no eixo y
#define POSYBOMB 0.0
// #define CAMZ 15.0    //posicao da camera no eixo z
#define CAMZ 15.0    //posicao da camera no eixo z
#define CAMY 15.0    //posicao da camera no eixo z

// Define a probabilidade de criar uma parede destrutível em uma posição vazia do mapa
const float DESTRUCTIBLE_WALL_PROBABILITY = 0.1;
const int MAP_WIDTH = 30;
const int MAP_HEIGHT = 30;

// Define o mapa do jogo
int gameMap[MAP_WIDTH][MAP_HEIGHT];
// Calcula o centro da matriz
int centralx = (float)MAP_WIDTH / 2.0f;
int centralz= (float)MAP_HEIGHT / 2.0f;

// Define uma estrutura para armazenar as informações sobre as paredes
struct Wall {
    float x, y , z, raio ;
    bool destrutivel;
    bool quebrado;
    int id;
};

std::vector<Wall> walls;


int fps_desejado = FPS/2; // variavel para alterar os frames por segundo desejado
int fps = 0; //contador de frames por segundo
float reshape_ratio = 1.0; // Manter proporcional a projecao
GLuint width = 640, height = 480; // Largura e altura da janela

//Enumeracoes com as direcoes do Objeto 3D em relacao a camera para realizar movimentacoes
enum direcao_acam{frente, tras, esquerda, direita};
int direcao = frente;

//Enumearacao com os identificadores das animacoes
enum animation_ids{paused, idle, bored, jumping, running, shooting};
int animation_id = idle; //Indica a animacao a ser desenhada atualmente
int current_animation = idle; // Salva qual e a animacao atual
bool is_jumping = false;
bool is_shooting = false;


enum bomb_ids{normal};
int bomb_id = normal;
int current = normal; // Salva qual e a animacao atual
bool is_bombing = false;

GLManimation * new_bomb = NULL;   //Ponteiro usado no armazenamento de uma bomba
map <int, GLManimation *> bombs; //Mapeamento dos identificadores com as bombas

GLManimation * animation = NULL;   //Ponteiro usado no armazenamento de uma animacao
map <int, GLManimation *> animations; //Mapeamento dos identificadores com as animacoes
int keyframe = 0; // numero do modelo 3D (keyframe) da animacao que sera desenhado no momento
int keyframe_rate = KEYFRAME_RATE; // alterar a taxa de repeticoes do keyframe na reproducao da animacao
int count_rate = -1; //Conta a quantidade de repeticoes do keyframe ate atingir a taxa keyframe_rate
int frames_playing = 0; //Conta a quantidade de quadros enquanto reproduz uma mesma animacao ate troca-la
int faces = 0, vertices = 0; //Numero de faces e vertices do objeto 3D

float graus = 15.0, deslocamento = 0.2; //incrementos do angulo de graus e do deslocamento
float scale = 1.0; // fator de escala proporcional em todos os eixos do modelo 3D
float rotx = 0.0, roty = 0.0, rotz = 0.0; // angulo de graus do modelo 3D no eixo x, y e z
float posx = 0.0, posy = POSY, posz = 0.0; // deslocamento do modelo 3D no eixo x, y e z
float praio = 0.25;
// float wraio = 0.25;
float posxBomb = 0.0, posyBomb = POSYBOMB, poszBomb = 0.0;  //deslocamento da bomba

float R = 1.0, G = 1.0, B = 1.0; //Cores do desenho controladas pelo glColor (inicial: cor branca)
float camx = 0.0, camy = CAMY, camz = CAMZ; // posicao da camera no eixo x, y e z
float cam_rotx = 0.0, cam_roty = 0.0; // angulo de graus da camera no eixo x e y
float cam_up = 1.0; //direcao do vetor para cima da camera

bool use_perspective = true; // Determina se usa projecao perspectiva ou ortografica
bool use_gouraud = true; // Determina o uso de Gouraud ou Flat shading
bool use_texture = true; // Determina o uso de texturas
bool use_color_material = false; // Determina o uso da glColor como parametro do material
bool use_depth_test = true; // Determina o uso ou nÃ£o do Z-buffering
bool use_light = true; // Determina se liga ou desliga a luz
bool render_solid = true; // Determina se renderiza o objeto solido ou em wireframe
bool is_paused = false; // Determina se a animacao esta tocando ou em pausa

// int delay_counter;
int bomb_delay = 3000;  // Tempo em milissegundos antes da explosão da bomba
int current_time = 0;   // Tempo atual em milissegundos
// raio da bomba em unidades do mundo
const float BOMB_RADIUS = 4.0f;

int new_bomb_id = 0;

GLint apply_texture = GL_DECAL; //Determina como a textura sera aplicada no objeto
std::string apply_texture_name = "Decal"; //Nome da fucao que sera usada para aplicar a textura no objeto

/*
 *  Definicao dos parametros do modelo de iluminacao
 */
GLfloat light_pos[] = {-2.0, 2.0, 2.0, 0.0};
GLfloat light_Ka[] = {0.4, 0.4, 0.4, 0.0};
GLfloat light_Kd[] = {1.0, 1.0, 1.0, 0.0};
GLfloat light_Ks[] = {1.0, 1.0, 1.0, 0.0};

/*
 *  Definicao dos parametros do material para o modelo de iluminacao
 *  Parametros para material amarelado, cor de latao (Brass)
 */
GLfloat material_Ka[] = {0.33, 0.22, 0.03, 1.00};
GLfloat material_Kd[] = {0.78, 0.57, 0.11, 1.00};
GLfloat material_Ks[] = {0.99, 0.94, 0.81, 1.00};
GLfloat material_Ke[] = {0.00, 0.00, 0.00, 0.00};
GLfloat material_Se = 28;

// Declaração de estrutura para informações do arquivo de som
typedef struct {
    const char* nome_arquivo;  // nome do arquivo de som
    const char* tipo;         // tipo de arquivo de som (wav, mp3, etc.)
} som_info;

// Declaração de variável para armazenar informações do arquivo de som
som_info som;


/*
 * Declaracoes antecipadas (forward) das funcoes (assinaturas)
 */
void init_glut(const char *window_name, int argc, char** argv);
void reshape(int w, int h);
void display(void);
void drawAnimation(int type, GLuint mode);
void drawFloor(GLuint mode);
void playAnimation(int id);
void computeFPS(int keyframe_rate);
void timer(int value);
void keyPlayAnimation(int id);
void keyboard(unsigned char key, int x, int y);
void keyboard_special(int key, int x, int y);
void menu(int value);

void drawSphere(float raio, int slices, int stacks);
void drawBomb(int id, GLuint mode);
void drawExplosion(GLfloat x, GLfloat y, GLfloat z);
// bool checkCollision(AABB box1, AABB box2);
bool checkCollision(float posx, float posy, float posz, float raio, const std::vector<Wall>& paredes);
void generateMap();
bool isBlockInBombRange(GLManimation* block, GLManimation* bomb);
void destroyBlocksInRange(GLManimation* bomb);
void destroyWall(Wall& wall);

/*
 * Funcao principal
 */
int main(int argc, char** argv){
//    setlocale(LC_ALL,"Portuguese");
   
    // inicia o GLUT e alguns parametros do OpenGL
    init_glut("Computacao Grafica: Renderizacao de Modelo 3D Complexo e Animacoes", argc, argv);

    // funcao de controle do GLUT
    glutMainLoop();

    return EXIT_SUCCESS;
}

/*
 * Funcao para inicializacao do GLUT e de alguns parametros do OpenGL
 */
void init_glut(const char *nome_janela, int argc, char** argv){
	
	// Gera o mapa aleatório
    generateMap();
    
    // inicia o GLUT
    glutInit(&argc,argv);

    // Inicia o display usando RGB, double-buffering e Z-buffering
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(width,height);
    glutInitWindowPosition(100,100);
    glutCreateWindow(nome_janela);
    
    // Define as funcoes de callback
    glutKeyboardFunc(keyboard);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(keyboard_special);
    glutTimerFunc(1000/fps_desejado, timer, 0); //(mseg, timer, value)

    // Define o menu
    glutCreateMenu(menu);
    glutAddMenuEntry("Desenhar a azul", 1);
    glutAddMenuEntry("Desenhar a verde", 2);
    glutAddMenuEntry("Desenhar a vermelho", 3);
    glutAddMenuEntry("Sair", 0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
	   
    /*
     *  Ativacao dos parametros do modelo de iluminacao para a Luz 0
     */
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_Ka);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_Kd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_Ks);

    /*
     *  Ativacao dos parametros do material para uso do modelo de iluminacao
     *  Para usar este material a iluminacao (GL_LIGHTING) deve estar ativa
     *  e a cor do material (GL_COLOR_MATERIAL) desativada
     */
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_Ka);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_Kd);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_Ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_Ke);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material_Se);

    glEnable(GL_LIGHTING); // Ativa o modelo de iluminacao
    glEnable(GL_LIGHT0); // Ativa a Luz 0. O OpenGL suporta pelo menos 8 pontos de luz.

    /*
     *  Configurando o OpenGL para o uso de Texturas
     */
    //Define como a textura sera aplicada ao objeto
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, apply_texture);

    //Ativa a visualizacao de texturas 2D (Texturizacao 2D)
    glEnable(GL_TEXTURE_2D);
    
    // Ativa o modelo de sombreamento de "Gouraud" (Smooth
    glShadeModel(GL_SMOOTH);
    
    // Ativa o Back-face Culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Ativa o z-buffering, de modo a remover as superficies escondidas
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
        
    //Define a cor de fundo (branco)
    glClearColor(1.0, 1.0, 1.0, 1.0);
    
    /*
     *  Carregando Modelos 3Ds, Texturas e Animacoes
     */
    printf("Loading 3D Models, Textures and Animations");
    animation = glmLoadAnimation("models/bomberman/bomPose.obj", paused, 1);
    animation->name = "paused";
    animations[paused] = animation;	
    faces = animation->models[0]->numtriangles;
    vertices = animation->models[0]->numvertices;
    printf(".");
    animation = glmLoadAnimation("models/bomberman/bomPose.obj", idle, 1);
    animations[idle] = animation;
    printf(".");
    animation = glmLoadAnimation("models/bomberman/animations/bobm", bored, 6);
    animations[bored] = animation;
    printf(".");
    // animation = glmLoadAnimation("models/ironman/bomb", jumping, 1);
    // animations[jumping] = animation;
    // printf(".");
    animation = glmLoadAnimation("models/bomberman/animations/bobm", running, 6);
    animations[running] = animation;
    printf(".");
	animation = glmLoadAnimation("models/bomberman/animations/bobm", shooting, 6);
    animations[shooting] = animation;
    printf(" done.\n");
 
	// srand(5);
	// Percorre a matriz do jogo e cria paredes destrutíveis com base na probabilidade definida
	for (int i = 0; i < MAP_WIDTH; i++) {
	    for (int j = 0; j < MAP_HEIGHT; j++) {
			Wall wall;
	        if (gameMap[i][j] == 1) {
   	 			wall.x = i - centralx;
    			wall.z = j - centralz;
    			wall.raio = 0.5;
	            wall.destrutivel = true;
	            wall.quebrado = false;
	        } else if (gameMap[i][j] == 0) { // Cria parede indestrutível
	            wall.destrutivel = false;
	            wall.quebrado = false;
	        }
    		walls.push_back(wall);
	    }
	}

}

/*
 * Processa o reshape da janela
 */
void reshape(int w, int h){
    // muda para o modo GL_PROJECTION e reinicia a projecao
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    
    // define a zona da janela onde se vai desenhar
    glViewport (0, 0, (GLsizei) w, (GLsizei) h);
    
    //Define a projecao perspectiva
    reshape_ratio = (GLfloat)w/(GLfloat)h;
    width = w;
    height = h;

    if(use_perspective){
        // gluPerspective(30, reshape_ratio, 1.0, 15.0);
        gluPerspective(60, reshape_ratio, 1.0, 90.0);
    }else{
        float ratioh = 1.34, ratiow = reshape_ratio * ratioh;
        glOrtho (-ratiow, ratiow, -ratioh, ratioh, 1.0 , 15.0);
    }
    
    // muda para o modo GL_MODELVIEW para preparar o desenho
    glMatrixMode (GL_MODELVIEW);
}

/*
 * Funcao para controlar o display
 */
void display(void){
	
    // Computa a taxa  de desenho de frames por segundo
    computeFPS(keyframe_rate); // Incrementa o keyframe da animacao a ser desenhado
    // Apaga o video e o depth buffer, e reinicia a matriz
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Translada a camera no eixo Z se distanciando do objeto
    // gluLookAt(camx, camy, camz, 0.0, 0.0, 0.0, 0.0, cam_up, 0.0);
    gluLookAt(
				camx, camy, camz,  		// Posiï¿½ï¿½o da cï¿½mera
				posx, posy + 1.0, posz, // Ponto para onde a cï¿½mera estï¿½ olhando (o personagem)
				0.0, 1.0, 0.0);			// Vetor up

	    
    // Ativa/Desativa a Texturizacao 2D (Visualizacao de Texturas 2D)
    if(use_texture) glEnable(GL_TEXTURE_2D);
    else glDisable(GL_TEXTURE_2D);
    
    // Liga/Desliga a luz
    if(use_light) glEnable(GL_LIGHTING);
    else glDisable(GL_LIGHTING);
    
    //Define se desenha usando sombreamento Gouraud ou Flat
    GLuint mode;
    if(use_gouraud) mode = (GLM_SMOOTH | GLM_TEXTURE);
    else mode = (GLM_FLAT  |GLM_TEXTURE);
            
    //Define a cor dos objetos da animacao
    glColor3f(R, G, B);

    glPushMatrix();
        glScaled(scale, scale, scale);
        glTranslated (posx, posy, posz);
        glRotated (rotx, 1.0f, 0.0f, 0.0f);
        glRotated (roty, 0.0f, 1.0f, 0.0f);
        glRotated (rotz, 0.0f, 0.0f, 1.0f);
        drawAnimation(animation_id, mode);
        // glutSolidCube(0.5); // desenha a caixa com tamanho 1.0
        // GLfloat half_size = 0.5f * scale; // metade do tamanhoss do cubo

    glPopMatrix();
    
	if(is_bombing){
		
		new_bomb = glmLoadAnimation("models/bomberman/bomba/bomba.obj", normal, 1);
	    printf("\npassou\n");
	    new_bomb->name = "normal";
	    new_bomb->keyframes = 1;
	    new_bomb->position[0] = posx;
	    new_bomb->position[1] = BPOSY;
	    new_bomb->position[2] = posz;
	    new_bomb->creation_time = glutGet(GLUT_ELAPSED_TIME);  // Definindo o tempo de criação
	    bombs[new_bomb_id] = new_bomb;
	    is_bombing = false;
	    printf("%f %f %f \n", new_bomb->position[0], new_bomb->position[1], new_bomb->position[2]);
		new_bomb_id += 1;	    
	}
    
	if (!bombs.empty()) {
	    for (std::map<int, GLManimation*>::iterator it = bombs.begin(); it != bombs.end(); it++) {
	    	
	        GLfloat pos[3];  
			pos[0] = it->second->position[0];
			pos[1] = it->second->position[1];
			pos[2] = it->second->position[2];
	    	
	        glPushMatrix();
			glTranslated(2 * -pos[0], 2 * -pos[1], 2 * -pos[2]); // escala (origem)
			glTranslated(0.25, -0, -0.75); // posiciona a bomba nos pes do personagem
	        glScaled(3, 3, 3);
	        glTranslated(pos[0], BPOSY, pos[2]); // escala  (posicao original)
	        glRotated(0, 1.0f, 0.0f, 0.0f);
	        glRotated(0, 0.0f, 1.0f, 0.0f);
	        glRotated(0, 0.0f, 0.0f, 1.0f);
	        drawBomb(it->first, mode);
	        glPopMatrix();
	        
	    }
	}
	
    drawFloor(mode); //Desenha o chao
    
    if(is_jumping && keyframe == animations[jumping]->keyframes - 1) is_jumping = false;
    
	// int delay_counter = 300; // contador de delay de 3 segundos

	if (!bombs.empty()) {
	    int current_time = glutGet(GLUT_ELAPSED_TIME);
	    for (std::map<int, GLManimation*>::iterator it = bombs.begin(); it != bombs.end(); ) {
	        if (current_time - it->second->creation_time >= bomb_delay) {
	            drawExplosion(it->second->position[0], 0, it->second->position[2]);
	            destroyBlocksInRange(it->second);
	            bombs.erase(it++);
	            printf("booom\n");
	            som.nome_arquivo = "./audios/bombaExplodir.wav";
				som.tipo = "WAVE";
				//char som[] = "../audios/background.wav";
				PlaySound(som.nome_arquivo, NULL, SND_FILENAME | SND_ASYNC); //Inicia o áudio de fundo do jogo
				
	        } else {
	            ++it;
	        }
	    }
	}
    
    // Translada a camera no eixo Z se distanciando do objeto
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    
    // Desenhando textos na janela GLUT
    glDisable(GL_TEXTURE_2D); //Desativa a Texturizacao 2D para desenhar o texto
    glDisable(GL_LIGHTING); // Desliga a Luz para desenhar o texto
    glColor3f(0.0, 0.0, 0.0); // seleciona a cor preta para o texto
    
    std::string cor = (R && G && B)?"White":(B)?"Blue":(G)?"Green":"Red";
    std::string luz = (use_light)?"On":"Off";
    std::string dep = (use_depth_test)?"On":"Off";
    std::string sha = (use_gouraud)?"Smooth":"Flat";
    std::string mat = (use_color_material)?"On":"Off";
    std::string tex = (use_texture)?"On":"Off";
    std::string ren = (render_solid)?"Solid":"Wired";
    std::string pro = (use_perspective)?"Persp":"Ortho";
    float x = -reshape_ratio-0.45;

    // draw_text_stroke(x , 1.25, "(ESC)EXIT (</>)FPS: " + to_string(fps) + " FACE: " + to_string(faces) + " VERT: " + to_string(vertices) + " (P)PAUSE ANIM: " + animations[animation_id]->name);
    // draw_text_stroke(x , 1.15,"(R)RESET (W/S)ROTX (A/D)ROTY (Arrows)RUN (Space)JUMP (F1)PROJ: " + pro );
    // draw_text_stroke(x , 1.05, "(F2)RENDER: " + ren + " (F3)C.MAT: " + mat + " (F4)COLOR: " +cor+ " (F5)SHADE: " + sha);
    // draw_text_stroke(x , 0.95, "(F6)DEPTH: " + dep + " (F7)LIGHT: "+ luz + " (F8)TEX: " + tex + " (F9)APPLY: " + apply_texture_name);
	draw_text_stroke(x , 1.80, "PosX: " + to_string(posx) + " PosY: " + to_string(posy) + " PosZ: " + to_string(posz),0.0015);
	draw_text_stroke(x , 1.50, "CamX: " + to_string(camx) + " CamY: " + to_string(camy) + " CamZ: " + to_string(camz),0.0015);
    
    // Troca os buffers, mostrando o que acabou de ser desenhado
    glutSwapBuffers();
}

/*
 * Coloca para reproduzir a animacao identificada
 */
void playAnimation(int id){
    keyframe = 0; // atribui o modelo 3D (keyframe) zero da animacao selecionada para ser desenhado
    frames_playing = 1; // reinicia a contagem de frames da reproducao da mesma animacao
    count_rate = 0; // Reinicia o contador de repeticoes do mesmo keyframe
    animation_id = id; // ativa a animacao com o identificador id
    current_animation = animation_id; // salva qual e a animacao atual
}

/*
 * Computa a quantidade de frames por segundo da animacao
 * Baseado no FPS e de acordo com o keyframe_rate, incrementa o keyframe da animacao a ser desenhado
 */
void computeFPS(int keyframe_rate){
    static GLuint frames = 0;       //Conta os frames em 1000 milissegundos, computando o FPS
    static GLuint clock;            // em milissegudos
    static GLuint next_clock = 0;   // em milissegudos

    count_rate++;
    frames_playing++;
    frames++;
    clock = glutGet(GLUT_ELAPSED_TIME); //NÃºmero de milissegundos desde a chamada a glutInit()

    if(!is_paused){ // Verifica se a animacao esta em pausa
        if(count_rate == keyframe_rate){ // Ao atigir a taxa de keyframes
            keyframe++; // incrementa o keyframe atual
            count_rate = 0; // reinicia o contador da taxa de keyframes
        }
                
        int size = animations[current_animation]->keyframes; // Quantidade de frames da animacao atual
        // int sizeBomb = bombs[current]->keyframes; // Quantidade de frames da animacao atual
        
        //Se animacao == idle, depois de reproduzir 5 vezes a animacao idle reproduz a animacao bored
        if(animation_id == idle && frames_playing >= (size * keyframe_rate * 5) + 1) playAnimation(bored);

        //Verifica se foi alterada a animacacao atual e reinicia os contadores para a nova animacao
        else if(current_animation != animation_id) playAnimation(animation_id);

        //Se uma animacao diferente de idle ou bored nao for mais reiniciada, reproduz a animacao idle
        else if(animation_id!= idle && animation_id!= bored && keyframe>= size && frames_playing>= size) playAnimation(idle);

        //reinicia a mesma animacao
        // else if(keyframe >= sizeBomb) keyframe = 0;
        
        //reinicia a mesma animacao
        else if(keyframe >= size) keyframe = 0;

        
//        printf("animation: %d, keyframe: %d ,size: %d, rate: %d, frames_playing: %d\n", animation_id, keyframe, size, count_rate, frames_playing);
    }
    
    if (clock < next_clock ) return;
    fps = frames; // guarda o nÃºmero de frames por segundo
    // Evita o reinicio da contagem dos frames na primeira iteracao
    if(next_clock != 0) frames = 0;//Reinicia a contagem dos frames a cada 1000 milissegundos
    next_clock = clock + 1000; //A cada 1000 milissegundos = 1 segundo
}


void drawAnimation(int id, GLuint mode){
    glmDrawAnimation(animations[id], keyframe, mode);
}

void drawBomb(int id, GLuint mode){
 	glmDrawAnimation(bombs[id], keyframe, mode);
}

void drawExplosion(GLfloat x, GLfloat y, GLfloat z) {
    GLfloat size = 0.5;
    while (size < BOMB_RADIUS) {
        glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(size, size, size);
        glColor3f(1.0, 0.0, 0.0);
        glutSolidSphere(1.0, 10, 10);
        glPopMatrix();
        size += 0.1;
    }
}

void drawFloor(GLuint mode) {
	
	   // Desenhar o ch?o
    if (mode & GLM_SMOOTH) glEnable(GL_SMOOTH);
    else glEnable(GL_FLAT);
    glDisable(GL_TEXTURE_2D);
    for (int x = -15; x < 15; x++) {
        for (int z = -15; z < 15; z++) {
            if ((x + z) % 2 == 0) glColor3f(0.7, 0.7, 0.7);
            else glColor3f(0.5, 0.5, 0.5);
            glPushMatrix();
            glTranslatef(x, 0, z);
            glBegin(GL_TRIANGLES);
            glNormal3f(0.0, 1.0, 0.0);
            glVertex3f(0.5, -1.0, -0.5);
            if (mode & GLM_SMOOTH) glNormal3f(0.0, -1.0, 0.0);
            glVertex3f(-0.5, -1.0, -0.5);
            if (mode & GLM_SMOOTH) glNormal3f(0.0, 1.0, 0.0);
            glVertex3f(0.5, -1.0, 0.5);

            glNormal3f(0.0, 1.0, 0.0);
            glVertex3f(-0.5, -1.0, 0.5);
            if (mode & GLM_SMOOTH)glNormal3f(0.0, 1.0, 0.0);
            glVertex3f(0.5, -1.0, 0.5);
            if (mode & GLM_SMOOTH) glNormal3f(0.0, -1.0, 0.0);
            glVertex3f(-0.5, -1.0, -0.5);
            glEnd();
            glPopMatrix();
        }
    }

	// printf("%f %f %f", walls.size(), centerX, centerY);
	// Desenha todas as paredes
	for (int i = 0; i < int(walls.size()); i++) {
        if (walls[i].quebrado == 0) {
        	
            // Desenha o muro na posição (i, j)
            glPushMatrix();
            glTranslatef(walls[i].x, 0, walls[i].z); // ajusta a posição do cubo de acordo com as coordenadas da matriz
            glScalef(1.0f, 1.5f, 1.0f);
            glutSolidCube(1.0f); // desenha o cubo
            glPopMatrix();
        }
	}
	glEnable(GL_TEXTURE_2D);
}

/*
 * Funcao utilizada para a animacao com temporizador
 */
void timer(int value){
    glutTimerFunc(1000/(fps_desejado), timer, 0);
    glutPostRedisplay(); // Manda redesenhar a tela em cada frame
}

/*
 * Reproduz uma animacao acionada por uma tecla
 */
void keyPlayAnimation(int id){
    frames_playing = 0;
    animation_id = id;
    if(id == jumping) is_jumping = true;
    if(id == shooting) is_shooting = true;
    if(id == normal) is_bombing = true;
}

/*
 * Controle das teclas comuns
 */
void keyboard(unsigned char key, int x, int y){
    switch(key){
        // Tecle - ou + para alterar o incremento do angulo de graus
//        case '-': case '_': if(graus > 1) graus -= 1; break;
//        case '+': case '=': if(graus < 360) graus += 1; break;
        
        case SPACE: if(!is_paused) keyPlayAnimation(normal); break; // Tecle SPACE para ativar a animacao de salto
        case 'p':case 'P':
            is_paused = !is_paused;
            animation_id = paused;
            keyframe = 0;
        break;
        case 'd': case 'D': //Rotaciona a camera em torno do eixo Y no sentido anti-horario
            float radianos;
            cam_roty += graus;
            roty += graus;
			if (roty >= 360.0) roty = 0.0;
            if (cam_roty >= 360.0) cam_roty = 0.0;
            radianos = (cam_roty * M_PI)/180.0;
            camx = 0.0; camz = CAMZ;
            camx =  camx*cosf(radianos) + camz*sinf(radianos);
            camz =  - camx*sinf(radianos) + camz*cosf(radianos) ;
        break;
        case 'a': case 'A': //Rotaciona a camera em torno do eixo Y no sentido horario
            cam_roty -= graus;
            roty -= graus;
			if (roty >= 360.0) roty = 0.0;
            if (cam_roty <= -360.0) cam_roty = 0.0;
            radianos = (cam_roty * M_PI)/180.0;
            camx = 0.0; camz = CAMZ;
            camx =    camx*cosf(radianos) + camz*sinf(radianos);
            camz =  - camx*sinf(radianos) + camz*cosf(radianos);
        break;
        case 'w': case 'W': //Rotaciona a camera em torno do eixo X no sentido anti-horatio
            if (cam_rotx >= 360.0) cam_rotx = 0.0;
            cam_rotx += graus;
            if (cam_rotx >= 52.0 && cam_rotx < 309.0) cam_up = -1.0;
            else cam_up = 1.0;
            radianos = (cam_rotx * M_PI)/180.0;
            camy = 0.0; camz = CAMZ;
            camy =  camy*cosf(radianos) - camz*sinf(radianos);
            camz =  camy*sinf(radianos) + camz*cosf(radianos);
        break;
        case 's': case 'S': //Rotaciona a camera em torno do eixo X no sentido horatio
            if (cam_rotx <= 0.0) cam_rotx = 360.0;
            cam_rotx -= graus;
            if (cam_rotx >= 52.0 && cam_rotx < 309.0) cam_up = -1.0;
            else cam_up = 1.0;
            radianos = (cam_rotx * M_PI)/180.0;
            camy = 0.0; camz = CAMZ;
            camy =  camy*cosf(radianos) - camz*sinf(radianos);
            camz =  camy*sinf(radianos) + camz*cosf(radianos);
        break;
        case 'r': case 'R': //Reseta as posicoes da camera e do objeto para os parametroz iniciais
            cam_rotx -= 0.0; cam_roty -= 0.0;
            camx = 0.0; camy =  CAMY; camz = CAMZ;
            posx = 0.0; posy = POSY; posz = 0.0;
            rotx = 0.0; roty =  0.0; rotz = 0.0;
        break; // Tecle f ou F para ativar a animacao de shooting
    	case 'f': case 'F': 
    		if(!is_paused) keyPlayAnimation(shooting); 
    		// shoot = 0;
		break; 

        // Tecle < ou > para alterar o FPS desejado
        case ',': case '<': if(fps_desejado > 1) fps_desejado -= 1; break;
        case '.': case '>': if(fps_desejado*2 < MAX_FPS) fps_desejado += 1; break;
        case ESC: exit(EXIT_SUCCESS); break; // Tecle ESC para sair do programa
    }
}

/*
 * Atualiza a direcao do personagem
 */
void update_direcao(){
    if(roty ==  0.0 || roty == 360.0){ //360
        direcao = frente;
        roty = 0.0;
    }
    if(roty == 180.0 || roty == -180.0){ // 180
        direcao = tras;
        roty = 180.0;
    }
    if(roty ==  90.0) direcao = direita;
    if(roty == -90.0 || roty == 270.0) direcao = esquerda;
}


bool checkCollision(float posx, float posy, float posz, float raio, const std::vector<Wall>& paredes) {
    // printf("entrei\n");
	for (int i = 0; i < int(walls.size()); i++) {
        float distancia = std::sqrt(std::pow(paredes[i].x - posx, 2) + std::pow(paredes[i].z - posz, 2));
        float distancia_minima = raio + paredes[i].raio;
        if (distancia < distancia_minima) {
            // Houve uma colisão. Ajuste a posição do personagem para a borda da parede.
            float ajuste_x = (posx - paredes[i].x) * distancia_minima / distancia;
            float ajuste_z = (posz - paredes[i].z) * distancia_minima / distancia;
            posx = paredes[i].x + ajuste_x;
            posz = paredes[i].z + ajuste_z;
            return true;
		}
    }
	printf("nao colisao\n");
    return false; // Não ocorreu colisão
}


/*
 * Controle das teclas especiais (Cursores, F1 a F12, etc...)
 */
void keyboard_special(int key, int x, int y){
    switch(key){
        case GLUT_KEY_F1: //Seleciona a projecao perspectiva ou ortografica
            use_perspective = (use_perspective)?false:true;
            reshape(width, height);
        break;
        case GLUT_KEY_F2: //Seleciona rederizacao solida ou em wireframe
            if (render_solid) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            render_solid = !render_solid;
        break;
        case GLUT_KEY_F3: //Liga/desliga o color material
            if (use_color_material){
                glDisable(GL_COLOR_MATERIAL);
                
                //Redefine os parametros do material para os valores iniciais, que estavam rastreando o glColor
                glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_Ka);
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_Kd);
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_Ks);
                glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_Ke);
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material_Se);
            }else glEnable(GL_COLOR_MATERIAL);
            use_color_material = !use_color_material;
        break;

        case GLUT_KEY_F4: //Alterar a cor de desenho
            if (R && G && B) { R = 0.0; G = 0.0; B = 1.0;} //Muda a cor para azul
            else if(B){ R = 0.0; G = 1.0; B = 0.0;} //Muda a cor para verde
            else if (G){ R = 1.0; G = 0.0; B = 0.0;} //Muda a cor para vermelho
            else { R = 1.0; G = 1.0; B = 1.0;} //Muda a cor para branco
        break;

        case GLUT_KEY_F5: use_gouraud = !use_gouraud; break; //Liga/desliga o gouraud shading
            
        case GLUT_KEY_F6: //Liga/desliga o Z-buffering
            if (use_depth_test) glDisable(GL_DEPTH_TEST);
            else glEnable(GL_DEPTH_TEST);
            use_depth_test = !use_depth_test;
        break;
            
        case GLUT_KEY_F7: use_light = !use_light; break; //Liga/desliga a luz
        
        case GLUT_KEY_F8: use_texture = !use_texture; break; //Liga/desliga a visualizacao da textura
        
        case GLUT_KEY_F9: //Troca como a textura sera aplicada no objeto
            switch (apply_texture) {
                case GL_ADD:
                    apply_texture = GL_DECAL;
                    apply_texture_name = "Decal";
                break;
                case GL_MODULATE:
                    apply_texture = GL_BLEND;
                    apply_texture_name = "Blend";
                break;
                case GL_BLEND:
                    apply_texture = GL_ADD;
                    apply_texture_name = "Add";
                break;
                case GL_DECAL:
                    apply_texture = GL_MODULATE;
                    apply_texture_name = "Modulate";
                break;
                //Funcoes usadas para combinar mais de uma texturas
//                case GL_COMBINE: //Uso sem combinar tem o mesmo efeito do GL_MODULATE
//                break;
//                case GL_REPLACE: //Uso sem combinar tem o mesmo efeito do GL_DECAL
//                break;
            }
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, apply_texture);
        break;
            
        //Use as setas do teclado para movimentar o personagem
        case GLUT_KEY_RIGHT: // Move o personagem para direita
            if(!is_paused){
                if(is_jumping) keyPlayAnimation(jumping);
                else keyPlayAnimation(running);
                if(checkCollision(posx + deslocamento, posy,posz, praio, walls) == false){
	                 if(direcao==direita){
	                    if(roty == 90.0) posx += deslocamento;
	                    else if (roty < 90.0 ) roty -= graus;
	                    else roty += graus;
	                }else if (direcao == esquerda){
	                    if( roty >= 180.0) roty -= graus;
	                    else roty += graus;
	                }else if (direcao == frente) roty += graus;
	                else roty -= graus;
	                update_direcao();
				}else{printf("colisao\n");}
            }
        break;

        case GLUT_KEY_LEFT: // Move o personagem para esquerda
            if(!is_paused){
                if(is_jumping) keyPlayAnimation(jumping);
                else keyPlayAnimation(running);
                if(!checkCollision(posx - deslocamento, posy,posz, praio, walls)){
	                if(direcao==esquerda){
	                    if(roty == -90.0 || roty == 270.0) posx -= deslocamento;
	                    else if ( roty < -90.0 || roty < 270.0 ) roty += graus;
	                    else roty -= graus;
	                }else if (direcao == direita){
	                    if(roty >= 90.0) roty += graus;
	                    else roty -= graus;
	                }else if (direcao == frente) roty -= graus;
	                else roty += graus;
	                update_direcao();
				}
            }
        break;
        case GLUT_KEY_UP: // Move o personagem para tras
            if(!is_paused){
                if(is_jumping) keyPlayAnimation(jumping);
                else keyPlayAnimation(running);
                if(!checkCollision(posx, posy,posz - deslocamento , praio, walls)){
	                if(direcao == tras){
	                    if(roty == 180.0) posz -= deslocamento;
	                    else if(roty < 180.0) roty += graus;
	                    else if(roty > 180.0) roty -= graus;
	                }else if (direcao==frente){
	                    if (roty >= 0.0) roty += graus;
	                    else roty -= graus;
	                }else if (direcao == direita) roty += graus;
	                else roty -= graus;
	                update_direcao();
				}
            }
        break;
        case GLUT_KEY_DOWN: // Move o personagem para frente
                if(!checkCollision(posx, posy,posz + deslocamento, praio, walls)){
                    if(is_jumping) keyPlayAnimation(jumping);
                    else keyPlayAnimation(running);
                    if(posz <= 13){
	                    if(direcao == frente){
	                        // if (roty == 0.0) posz += deslocamento;
	                        if (roty == 0.0) posz += deslocamento;
	                        else if (roty < 0.0) roty += graus;
	                        else if (roty > 0.0) roty -= graus;
	                    }else if(direcao == tras){
	                        if (roty >= 180.0) roty += graus;
	                        else roty -= graus;
	                    }else if(direcao == direita) roty -= graus;
	                    else roty += graus;
	                    update_direcao();
					}
                }
        break;
    }
}

void generateMap() {
	// Define a semente aleatória
    srand(time(NULL));
    // Preenche a matriz com paredes e blocos destrutíveis aleatórios
    for(int i = 0; i < MAP_HEIGHT; i++) {
        for(int j = 0; j < MAP_WIDTH; j++) {
            if(i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) {
                // Cria paredes ao redor do mapa
                gameMap[i][j] = 1;	
            } else if(i % 2 == 0 && j % 2 == 0) {
                // Cria paredes nos pontos pares
                gameMap[i][j] = 1;
            } else if (i == centralz && j == centralx) {
                // Define o ponto central como vazio
                gameMap[i][j] = 0;
            } else if ((i == centralz+1 && j == centralx) || (i == centralz-1 && j == centralx) || 
                       (i == centralz && j == centralx+1) || (i == centralz && j == centralx-1)) {
                // Não cria paredes ou blocos nos arredores do personagem
                gameMap[i][j] = 0;
            } else {
                // 50% de chance de criar um bloco destrutível
                gameMap[i][j] = rand() % 8;
            }
        }
    }
}


// função para destruir um bloco destrutível
void destroyBlock(GLManimation* block) {
    // remover o bloco da estrutura de dados
    // walls.erase(block);

    // desenhar uma animação de destruição em seu lugar
    // ...
}


// função para checar se um bloco está dentro do raio da bomba
bool isBlockInBombRange(Wall* wall, GLManimation* bomb) {
    float dx = wall->x - bomb->position[0];
    float dz = wall->z - bomb->position[2];
    float dist = sqrt(dx*dx + dz*dz);
    return dist <= BOMB_RADIUS;
}

void destroyWall(Wall& wall) {
    // Remover a parede do vetor de paredes
    for (std::vector<Wall>::iterator it = walls.begin(); it != walls.end(); ++it) {
        if (it->x == wall.x && it->z == wall.z) {
            //walls.erase(it);
            wall.quebrado = 1;
            break;
        }
    }
}

// função para destruir todos os blocos destrutíveis dentro do raio da bomba
void destroyBlocksInRange(GLManimation* bomb) {
    std::vector<GLManimation*> blocks_to_destroy;
    for (std::vector<Wall>::iterator it = walls.begin(); it != walls.end(); ++it) {
        if (it->destrutivel && isBlockInBombRange(&(*it), bomb)) {
            destroyWall(*it);
        }
    }
}

/*
 * Controle do menu pop-up
 */
void menu(int value){
    switch(value){
        case 1: R = 0.0; G = 0.0; B = 1.0; break;//Muda a cor para azul
        case 2: R = 0.0; G = 1.0; B = 0.0; break;//Muda a cor para verde
        case 3: R = 1.0; G = 0.0; B = 0.0; break;//Muda a cor para vermelho
        case 0: exit(EXIT_SUCCESS); break; //Sai do programa
    }
}
