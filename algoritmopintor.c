/* painter.c — Algoritmo Pintor em OpenGL/GLUT
 * Requisitos de compilação (Windows + MSYS2/MinGW): 
 *   gcc painter.c -o painter -lfreeglut -lopengl32 -lglu32
 *
 * Teclas:
 *   P/p  -> liga/desliga Algoritmo Pintor
 *   Z/z  -> liga/desliga z-buffer (GL_DEPTH_TEST)
 *   T/t  -> liga/desliga transparência (GL_BLEND)
 *   C/c  -> liga/desliga back-face culling
 *   L/l  -> liga/desliga iluminação
 *   G/g  -> alterna textura no piso (checker procedural)
 *   +/-  -> avança/volta na animação da câmera (t da Bézier)
 *   R/r  -> reseta as opções padrão
 *   ESC  -> sair
 */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------- util ------------------------- */
typedef struct { float x,y,z; } vec3;
typedef struct { float r,g,b,a; } rgba;

static vec3 vadd(vec3 a, vec3 b){ vec3 r={a.x+b.x,a.y+b.y,a.z+b.z}; return r; }
static vec3 vsub(vec3 a, vec3 b){ vec3 r={a.x-b.x,a.y-b.y,a.z-b.z}; return r; }
static vec3 vscale(vec3 a, float s){ vec3 r={a.x*s,a.y*s,a.z*s}; return r; }
static float vdot(vec3 a, vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
static vec3 vcross(vec3 a, vec3 b){ vec3 r={a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; return r; }
static vec3 vnorm(vec3 a){ float m=sqrtf(vdot(a,a)); return (m>1e-8f)? vscale(a,1.f/m):a; }

/* -------------------- textura procedural -------------------- */
#define TXW 64
#define TXH 64
static GLuint texChecker=0;
static int    useChecker = 1;

static void makeChecker(void){
    GLubyte data[TXH][TXW][3];
    for(int i=0;i<TXH;i++){
        for(int j=0;j<TXW;j++){
            int c = (((i & 0x8)==0) ^ ((j & 0x8)==0)) ? 255:0;
            data[i][j][0] = (GLubyte)c;
            data[i][j][1] = (GLubyte)c;
            data[i][j][2] = (GLubyte)c;
        }
    }
    if(!texChecker) glGenTextures(1,&texChecker);
    glBindTexture(GL_TEXTURE_2D, texChecker);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,TXW,TXH,0,GL_RGB,GL_UNSIGNED_BYTE,data);
}

/* --------------------- câmera por Bézier --------------------- */
/* Curva cúbica: B(t) = Σ C(3,k) (1-t)^(3-k) t^k Pk, 0<=t<=1 */
static vec3 P0={-6,  2,  8};
static vec3 P1={-2,  4,  2};
static vec3 P2={ 4,  3,  4};
static vec3 P3={ 6,  1, 10};
static float tCam=0.0f;    /* 0..1 */
static int   animDir=0;

static vec3 bezier3(vec3 p0,vec3 p1,vec3 p2,vec3 p3,float t){
    float u=1.f-t;
    float b0=u*u*u, b1=3*u*u*t, b2=3*u*t*t, b3=t*t*t;
    vec3 r = vadd( vadd( vscale(p0,b0), vscale(p1,b1) ),
                   vadd( vscale(p2,b2), vscale(p3,b3) ) );
    return r;
}

/* --------------------- descritor de polígonos --------------------- */
typedef struct {
    int     n;           /* número de vértices */
    vec3    v[6];        /* até 6 vértices para simplicidade */
    rgba    color;
    int     textured;    /* 1 se usa textura checker */
    float   uv[6][2];    /* UV por vértice quando textured=1 */
    float   depth;       /* preenchido a cada frame (pintor) */
} Poly;

/* Cena: um “pátio” com piso texturizado, duas paredes opacas e
   uma “janela” semitransparente (para evidenciar a diferença
   entre z-buffer e pintor) + um cubo no meio. */
static Poly scene[16];
static int  nScene=0;

static void addQuad(vec3 a,vec3 b,vec3 c,vec3 d, rgba col, int textured,
                    float u0,float v0,float u1,float v1)
{
    Poly p; memset(&p,0,sizeof(p));
    p.n=4; p.v[0]=a; p.v[1]=b; p.v[2]=c; p.v[3]=d; p.color=col;
    p.textured=textured;
    if(textured){
        p.uv[0][0]=u0; p.uv[0][1]=v0;
        p.uv[1][0]=u0; p.uv[1][1]=v1;
        p.uv[2][0]=u1; p.uv[2][1]=v1;
        p.uv[3][0]=u1; p.uv[3][1]=v0;
    }
    scene[nScene++]=p;
}

static void addCube(vec3 c, float s, rgba col){
    float h=s*0.5f;
    vec3 A={c.x-h,c.y-h,c.z-h}, B={c.x+h,c.y-h,c.z-h}, C={c.x+h,c.y+h,c.z-h}, D={c.x-h,c.y+h,c.z-h};
    vec3 E={c.x-h,c.y-h,c.z+h}, F={c.x+h,c.y-h,c.z+h}, Gv={c.x+h,c.y+h,c.z+h}, H={c.x-h,c.y+h,c.z+h};
    addQuad(A,B,C,D,col,0,0,0,0,0); /* trás  */
    addQuad(E,F,Gv,H,col,0,0,0,0,0);/* frente*/
    addQuad(A,E,F,B,col,0,0,0,0,0); /* baixo */
    addQuad(D,H,Gv,C,col,0,0,0,0,0);/* cima  */
    addQuad(A,D,H,E,col,0,0,0,0,0); /* esquerda */
    addQuad(B,C,Gv,F,col,0,0,0,0,0);/* direita  */
}

static void buildScene(void){
    nScene=0;
    /* piso (texturizado repetindo) */
    addQuad((vec3){-8,-2,-2}, (vec3){-8,-2,12},
            (vec3){ 8,-2,12}, (vec3){ 8,-2,-2},
            (rgba){1,1,1,1}, 1, 0,0, 6,6);

    /* parede esquerda (opaca) */
    addQuad((vec3){-8,-2,12}, (vec3){-8, 6,12},
            (vec3){-8, 6,-2}, (vec3){-8,-2,-2},
            (rgba){0.7f,0.7f,0.9f,1}, 0,0,0,0,0);

    /* parede direita (opaca) */
    addQuad((vec3){ 8,-2,-2}, (vec3){ 8, 6,-2},
            (vec3){ 8, 6,12}, (vec3){ 8,-2,12},
            (rgba){0.9f,0.7f,0.7f,1}, 0,0,0,0,0);

    /* janela semitransparente à frente */
    addQuad((vec3){-3,0,6}, (vec3){-3,4,6}, (vec3){ 3,4,6}, (vec3){ 3,0,6},
            (rgba){0.2f,0.6f,1.0f,0.35f}, 0,0,0,0,0);

    /* cubo ao centro */
    addCube((vec3){0,-1,6}, 2.5f, (rgba){0.9f,0.9f,0.3f,1});
}

/* --------------------- flags/toggles --------------------- */
static int usePainter=1;      /* algoritmo pintor ligado */
static int useDepth=1;        /* z-buffer */
static int useBlend=1;        /* transparência */
static int useCull =1;        /* back-face culling */
static int useLight=1;        /* iluminação */
static int spin   =0;         /* rotação do cubo */
static int showTex=1;

/* ------------------ iluminação/material ------------------ */
static void setupLighting(void){
    if(useLight){
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        GLfloat La[]={0.2f,0.2f,0.2f,1}, Ld[]={1,1,1,1}, Ls[]={1,1,1,1};
        GLfloat Lpos[]={2,5,4,1};
        glLightfv(GL_LIGHT0,GL_AMBIENT,La);
        glLightfv(GL_LIGHT0,GL_DIFFUSE,Ld);
        glLightfv(GL_LIGHT0,GL_SPECULAR,Ls);
        glLightfv(GL_LIGHT0,GL_POSITION,Lpos);
        GLfloat mSpec[]={0.6f,0.6f,0.6f,1}, shin[]={32};
        glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mSpec);
        glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shin);
    }else{
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
    }
}

/* -------------- transformação: mundo->olho -------------- */
static void multMV(const float M[16], const vec3 *p, vec3 *r){
    /* M: coluna-major (OpenGL) */
    float x=p->x, y=p->y, z=p->z;
    r->x = M[0]*x + M[4]*y + M[8]*z  + M[12];
    r->y = M[1]*x + M[5]*y + M[9]*z  + M[13];
    r->z = M[2]*x + M[6]*y + M[10]*z + M[14];
}

/* calcula profundidade média em coordenadas de olho (z) */
static void computeDepths(void){
    float MV[16]; glGetFloatv(GL_MODELVIEW_MATRIX, MV);
    for(int i=0;i<nScene;i++){
        float zsum=0;
        for(int k=0;k<scene[i].n;k++){
            vec3 o; multMV(MV,&scene[i].v[k],&o);
            zsum += o.z;
        }
        scene[i].depth = zsum/(float)scene[i].n;
    }
}

/* ordena polígonos de trás->frente (maior z primeiro) */
static int cmpPoly(const void* a, const void* b){
    float da = ((const Poly*)a)->depth;
    float db = ((const Poly*)b)->depth;
    return (da<db)? 1 : (da>db? -1 : 0);
}

/* desenha um polígono (aplica cor/alpha/tex) */
static void drawPoly(const Poly* p){
    glColor4f(p->color.r,p->color.g,p->color.b,p->color.a);
    if(p->textured && showTex && useChecker){
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texChecker);
    }else{
        glDisable(GL_TEXTURE_2D);
    }
    glBegin(GL_POLYGON);
    for(int i=0;i<p->n;i++){
        if(p->textured && showTex){
            glTexCoord2f(p->uv[i][0], p->uv[i][1]);
        }
        glVertex3f(p->v[i].x,p->v[i].y,p->v[i].z);
    }
    glEnd();
}

/* --------------------- display principal --------------------- */
static void drawScene(void){
    /* cubo gira um pouco pra dar vida */
    spin = (spin+1)%360;

    /* piso/parede/janela/cubo já estão em scene[].
       Para animar o cubo, aplicamos uma rotação local. */
    glPushMatrix();
    glTranslatef(0,-1,6);
    glRotatef((float)spin,0,1,0);
    glTranslatef(0,+1,-6);
    /* computa profundidades no sistema atual da câmera */
    computeDepths();

    /* Se pintor ligado: ordena e desenha “pintando” de trás pra frente.
       Caso contrário, desenha na ordem do array (ilustrando artefatos quando
       z-buffer/blend/culling mudam). */
    if(usePainter){
        Poly sorted[64];
        memcpy(sorted, scene, sizeof(Poly)*nScene);
        qsort(sorted, nScene, sizeof(Poly), cmpPoly);
        for(int i=0;i<nScene;i++) drawPoly(&sorted[i]);
    }else{
        for(int i=0;i<nScene;i++) drawPoly(&scene[i]);
    }
    glPopMatrix();
}

static void display(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* flags globais */
    if(useDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if(useCull)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
    if(useBlend){ glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
    else         { glDisable(GL_BLEND); }
    setupLighting();

    /* câmera (animação por Bézier) */
    vec3 eye = bezier3(P0,P1,P2,P3,tCam);
    vec3 ctr = (vec3){0, 0, 6};             /* olha para o centro da cena */
    vec3 up  = (vec3){0, 1, 0};
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eye.x,eye.y,eye.z,  ctr.x,ctr.y,ctr.z,  up.x,up.y,up.z);

    drawScene();
    glutSwapBuffers();
}

/* --------------------- reshape & init --------------------- */
static void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w/(float)h, 0.5, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

static void init(void){
    glClearColor(0,0,0,1);
    makeChecker();
    buildScene();
    glShadeModel(GL_SMOOTH);
}

/* ---------------------- teclado & idle ---------------------- */
static void keyboard(unsigned char key,int x,int y){
    switch(key){
        case 27: exit(0); break;
        case 'p': case 'P': usePainter=!usePainter; break;
        case 'z': case 'Z': useDepth  =!useDepth;   break;
        case 't': case 'T': useBlend  =!useBlend;   break;
        case 'c': case 'C': useCull   =!useCull;    break;
        case 'l': case 'L': useLight  =!useLight;   break;
        case 'g': case 'G': useChecker=!useChecker; break;
        case '+': tCam += 0.02f; if(tCam>1.f) tCam=1.f; break;
        case '-': tCam -= 0.02f; if(tCam<0.f) tCam=0.f; break;
        case 'r': case 'R':
            usePainter=1; useDepth=1; useBlend=1; useCull=1; useLight=1;
            tCam=0.0f; break;
        default: break;
    }
    glutPostRedisplay();
}

static void timer(int v){
    if(animDir){ tCam += 0.005f; if(tCam>1.f) tCam=0.f; }
    glutPostRedisplay();
    glutTimerFunc(16,timer,0);
}

int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900,600);
    glutCreateWindow("Algoritmo Pintor - CG");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16,timer,0);
    glutMainLoop();
    return 0;
}
