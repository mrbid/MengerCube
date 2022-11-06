/*
    James William Fletcher (github.com/mrbid)
        November 2022
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/file.h>
#include <stdint.h>
#include <unistd.h>

#define uint GLushort
#define sint GLshort
#define f32 GLfloat

#include "inc/gl.h"
#define GLFW_INCLUDE_NONE
#include "inc/glfw3.h"

#ifndef __x86_64__
    #define NOSSE
#endif

// uncommenting this define will enable the MMX random when using fRandFloat (it's a marginally slower)
#define SEIR_RAND

#include "inc/esAux2.h"

#include "inc/res.h"
#include "ncube.h"

//*************************************
// globals
//*************************************
GLFWwindow* window;
uint winw = 1024;
uint winh = 768;
double t = 0;   // time
f32 dt = 0;     // delta time
double fc = 0;  // frame count
double lfct = 0;// last frame count time
f32 aspect;
double x,y,lx,ly;
double rww, ww, rwh, wh, ww2, wh2;
double uw, uh, uw2, uh2; // normalised pixel dpi

// render state id's
GLint projection_id;
GLint modelview_id;
GLint normalmat_id = -1;
GLint position_id;
GLint lightpos_id;
GLint solidcolor_id;
GLint color_id;
GLint opacity_id;
GLint normal_id; // 

// render state matrices
mat projection;
mat view;

// models
ESModel mdlMenger;

// camera vars
vec lightpos = {0.f, 0.f, 0.f};
#define FAR_DISTANCE 120.f
uint focus_cursor = 0;
double sens = 0.001f;
f32 xrot = 0.f;
f32 yrot = d2PI;
f32 zoom = -16.0f; // -6.0f / -26.0f

// sim vars
f32 r=0.f,g=0.f,b=0.f;

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
float urandf()
{
    static const float RECIP_FLOAT_UINT64_MAX = 1.f/(float)UINT64_MAX;
    int f = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    uint64_t s = 0;
    read(f, &s, sizeof(uint64_t));
    close(f);
    return ((float)s) * RECIP_FLOAT_UINT64_MAX;
}
float clamp(float f, float min, float max)
{
    if(f > max){return max;}
    else if(f < min){return min;}
    return f;
}
void stepTitle()
{
    static uint p = 0;
    static double lt = 0.0;
    if(t > lt)
    {
        if(p == 0)
        {
            glfwSetWindowTitle(window, "L3 Menger Cube");
            lt = t+6.0;
            p++;
            return;
        }
        else if(p == 1)
            glfwSetWindowTitle(window, "F");
        else if(p == 2)
            glfwSetWindowTitle(window, "Fa");
        else if(p == 3)
            glfwSetWindowTitle(window, "Fan");
        else if(p == 4)
            glfwSetWindowTitle(window, "Fanc");
        else if(p == 5)
            glfwSetWindowTitle(window, "Fancy");
        else if(p == 6)
            glfwSetWindowTitle(window, "Fancy a");
        else if(p == 7)
            glfwSetWindowTitle(window, "Fancy a s");
        else if(p == 8)
            glfwSetWindowTitle(window, "Fancy a sp");
        else if(p == 9)
            glfwSetWindowTitle(window, "Fancy a spi");
        else if(p == 10)
            glfwSetWindowTitle(window, "Fancy a spin");
        else if(p == 11)
        {
            glfwSetWindowTitle(window, "Fancy a spin?");
            lt = t+6.0;
            p++;
            return;
        }
        p++;
        if(p >= 12){p=0;}
        lt = t+0.09+(randf()*0.04);
    }
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// camera
//*************************************
    if(focus_cursor == 1)
    {
        glfwGetCursorPos(window, &x, &y);

        xrot += (ww2-x)*sens;
        yrot += (wh2-y)*sens;

        glfwSetCursorPos(window, ww2, wh2);
    }

    mIdent(&view);
    mTranslate(&view, 0.f, 0.f, zoom);
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 0.f, 1.f);
    
    if(focus_cursor == 0)
    {
        static f32 ss = 0.08f;
        static f32 tft = 0.f;
        tft += dt;
        yrot += sinf(tft*0.1f)*-ss;
        ss += 0.000001f;
        xrot += dt*0.1f;
        r += randfc()*dt*1.6f;
        g += randfc()*dt*1.6f;
        b += randfc()*dt*1.6f;
        r = clamp(r, -1.f, 1.f);
        g = clamp(g, -1.f, 1.f);
        b = clamp(b, -1.f, 1.f);
        glUniform3f(color_id, r, g, b);
        const f32 ft = tft*0.5f;
        glUniform3f(lightpos_id, sinf(ft) * 10.0f, cosf(ft) * 10.0f, sinf(ft) * 10.0f);
    }

    stepTitle();

//*************************************
// render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (GLfloat*) &view.m[0][0]);
    if(normalmat_id != -1)
    {
        mat inverted, normalmat;
        mIdent(&inverted);
        mIdent(&normalmat);
        mInvert(&inverted.m[0][0], &view.m[0][0]);
        mTranspose(&normalmat, &inverted);
        glUniformMatrix4fv(normalmat_id, 1, GL_FALSE, (GLfloat*) &normalmat.m[0][0]);
    }
    glDrawElements(GL_TRIANGLES, ncube_numind, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
}

//*************************************
// Input Handelling
//*************************************
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_F)
        {
            if(t-lfct > 2.0)
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] FPS: %g\n", strts, fc/(t-lfct));
                lfct = t;
                fc = 0;
            }
        }
        else if(key == GLFW_KEY_Z)
        {
            shadeLambert1(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
            glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
            glUniform1f(opacity_id, 1.0f);
            glUniform3f(color_id, r, g, b);
            normalmat_id = -1;
        }
        else if(key == GLFW_KEY_X)
        {
            shadePhong1(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
            glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
            glUniform1f(opacity_id, 1.0f);
            glUniform3f(color_id, r, g, b);
        }
        else if(key == GLFW_KEY_A)
            glDisable(GL_BLEND);
        else if(key == GLFW_KEY_S)
            glEnable(GL_BLEND);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(yoffset == -1)
        zoom += 0.06f * zoom;
    else if(yoffset == 1)
        zoom -= 0.06f * zoom;
    
    if(zoom > 0.f){zoom = 0.f;}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
        {
            focus_cursor = 1 - focus_cursor;
            if(focus_cursor == 0)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            glfwSetCursorPos(window, ww2, wh2);
            glfwGetCursorPos(window, &ww2, &wh2);
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            r = urandf(), g = urandf(), b = urandf();
            glUniform3f(color_id, r, g, b);
        }
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width;
    winh = height;

    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = winw;
    wh = winh;
    rww = 1/ww;
    rwh = 1/wh;
    ww2 = ww/2;
    wh2 = wh/2;
    uw = (double)aspect / ww;
    uh = 1 / wh;
    uw2 = (double)aspect / ww2;
    uh2 = 1 / wh2;

    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 0.01f, FAR_DISTANCE); 
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // allow framerate cap
    double maxfps = 144.0;
    if(argc >= 3){maxfps = atof(argv[2]);}

    // help
    printf("----\n");
    printf("L3 Menger Cube\n");
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("----\n");
    printf("Argv(2): msaa, maxfps\n");
    printf("e.g; ./uc 16 60\n");
    printf("----\n");
    printf("Left Click = Focus toggle camera control\n");
    printf("F = FPS to console.\n");
    printf("A = Opaque.\n");
    printf("D = Transparent.\n");
    printf("Z = Lambertian Shading.\n");
    printf("X = Phong Shading.\n");
    printf("----\n");

    // init glfw
    if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    window = glfwCreateWindow(winw, winh, "L3 Menger Cube", NULL, NULL);
    if(!window)
    {
        printf("glfwCreateWindow() failed.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(0); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});

//*************************************
// projection
//*************************************

    window_size_callback(window, winw, winh);

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND MENGER *****
    esBind(GL_ARRAY_BUFFER, &mdlMenger.vid, ncube_vertices, sizeof(ncube_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMenger.nid, ncube_normals, sizeof(ncube_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMenger.iid, ncube_indices, sizeof(ncube_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader programs
//*************************************

    //makeAllShaders();
    makeLambert1();
    makePhong1();

//*************************************
// configure render options
//*************************************

    // standard stuff
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.13f, 0.13f, 0.13f, 0.0f);

    // setup shader
    shadePhong1(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 0.5f);
    
    // bind menger to render
    r = urandf(), g = urandf(), b = urandf();
    glUniform3f(color_id, 1.0, 1.0, 1.0);

    glBindBuffer(GL_ARRAY_BUFFER, mdlMenger.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlMenger.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMenger.iid);

//*************************************
// execute update / render loop
//*************************************

    // init
    t = glfwGetTime();
    lfct = t;
    dt = 1.0 / (float)maxfps; // fixed timestep delta-time
    
    // fps accurate event loop
    const useconds_t wait_interval = 1000000 / maxfps; // fixed timestep
    useconds_t wait = wait_interval;
    while(!glfwWindowShouldClose(window))
    {
        usleep(wait);
        t = glfwGetTime();

        glfwPollEvents();
        main_loop();

        // accurate fps
        wait = wait_interval - (useconds_t)((glfwGetTime() - t) * 1000000.0);
        if(wait > wait_interval)
            wait = wait_interval;
        
        fc++;
    }

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
