// ef52_gputest: OpenGL ES 3.0 3D test for the VEGA Iron (Adreno 320).
// 1) correctness in an offscreen FBO (depth test, texture sampling, ES3
//    instancing) checked pixel by pixel, 2) a lit, instanced 3D scene drawn on
//    the screen for N seconds (fps), 3) blended full-screen fill rate.
// Usage: ef52_gputest [seconds per performance test, default 8]
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <WindowSurface.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace android;

static int gFail = 0;

static double now() {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static bool glOk(const char* what) {
    GLenum e = glGetError();
    if (e != GL_NO_ERROR) {
        printf("  GL ERROR 0x%04x after %s\n", e, what);
        gFail++;
        return false;
    }
    return true;
}

static GLuint shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("  shader compile FAILED: %s\n", log);
        gFail++;
    }
    return s;
}

static GLuint program(const char* vs, const char* fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, shader(GL_VERTEX_SHADER, vs));
    glAttachShader(p, shader(GL_FRAGMENT_SHADER, fs));
    glBindAttribLocation(p, 0, "aPos");
    glBindAttribLocation(p, 1, "aNrm");
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("  program link FAILED: %s\n", log);
        gFail++;
    }
    return p;
}

static void check(const char* name, int x, int y, unsigned er, unsigned eg, unsigned eb) {
    unsigned char px[4] = {0, 0, 0, 0};
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    bool ok = abs(px[0] - (int)er) <= 3 && abs(px[1] - (int)eg) <= 3 && abs(px[2] - (int)eb) <= 3;
    printf("  %-28s (%3d,%3d) got %3u %3u %3u want %3u %3u %3u  %s\n", name, x, y, px[0], px[1],
           px[2], er, eg, eb, ok ? "OK" : "FAIL");
    if (!ok) gFail++;
}

static const char* kFlatVs =
        "#version 300 es\n"
        "in vec3 aPos;\n"
        "uniform vec2 uOff; uniform float uScale;\n"
        "flat out int vInst;\n"
        "void main() {\n"
        "  vec2 o = uOff;\n"
        "  if (uScale > 0.0) o = vec2(float(gl_InstanceID % 2) - 0.5, float(gl_InstanceID / 2) - 0.5);\n"
        "  vInst = gl_InstanceID;\n"
        "  gl_Position = vec4(aPos.xy + o, aPos.z, 1.0);\n"
        "}\n";
static const char* kFlatFs =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform vec4 uColor; uniform float uScale;\n"
        "flat in int vInst;\n"
        "out vec4 oColor;\n"
        "void main() {\n"
        "  if (uScale > 0.0) {\n"
        "    vec4 c[4] = vec4[4](vec4(1,0,0,1), vec4(0,1,0,1), vec4(0,0,1,1), vec4(1,1,0,1));\n"
        "    oColor = c[vInst];\n"
        "  } else oColor = uColor;\n"
        "}\n";
static const char* kTexVs =
        "#version 300 es\n"
        "in vec3 aPos; out vec2 vUv;\n"
        "void main() { vUv = aPos.xy * 0.5 + 0.5; gl_Position = vec4(aPos, 1.0); }\n";
static const char* kTexFs =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform sampler2D uTex; in vec2 vUv; out vec4 oColor;\n"
        "void main() { oColor = texture(uTex, vUv); }\n";

// full-screen quad as two triangles at depth z, scaled to [-s,s]
static void quad(float* v, float z) {
    const float q[18] = {-1, -1, z, 1, -1, z, 1, 1, z, -1, -1, z, 1, 1, z, -1, 1, z};
    memcpy(v, q, sizeof(q));
}

static void correctness() {
    printf("== correctness (offscreen 128x128 FBO)\n");
    const int W = 128, H = 128;
    GLuint fbo, color, depth;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, W, H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    printf("  FBO status 0x%04x %s\n", st, st == GL_FRAMEBUFFER_COMPLETE ? "complete" : "INCOMPLETE");
    if (st != GL_FRAMEBUFFER_COMPLETE) gFail++;
    glViewport(0, 0, W, H);

    GLuint flat = program(kFlatVs, kFlatFs);
    glUseProgram(flat);
    GLint uColor = glGetUniformLocation(flat, "uColor"), uOff = glGetUniformLocation(flat, "uOff"),
          uScale = glGetUniformLocation(flat, "uScale");
    float v[18];
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);

    // depth test: red quad z=0.0, green z=+0.6 (farther, left half), blue z=-0.6 (nearer, top half)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0, 0, 0, 1);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    auto draw = [&](float z, float sx, float sy, float ox, float oy, float r, float g, float b) {
        quad(v, z);
        for (int i = 0; i < 6; i++) { v[i * 3] *= sx; v[i * 3 + 1] *= sy; }
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glUniform2f(uOff, ox, oy);
        glUniform1f(uScale, -1.0f);
        glUniform4f(uColor, r, g, b, 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    };
    draw(0.0f, 1, 1, 0, 0, 1, 0, 0);
    draw(0.6f, 0.5f, 1, -0.5f, 0, 0, 1, 0);
    draw(-0.6f, 1, 0.5f, 0, 0.5f, 0, 0, 1);
    glOk("depth draws");
    check("depth: top-left (blue)", 32, 96, 0, 0, 255);
    check("depth: bottom-left (red)", 32, 32, 255, 0, 0);
    check("depth: bottom-right (red)", 96, 32, 255, 0, 0);

    // ES3 instancing: 4 instances, one colored quad per quadrant
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    quad(v, 0);
    for (int i = 0; i < 18; i++) if (i % 3 != 2) v[i] *= 0.5f;
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glUniform1f(uScale, 1.0f);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 4);
    glOk("instanced draw");
    check("instancing: id0 (red)", 32, 32, 255, 0, 0);
    check("instancing: id1 (green)", 96, 32, 0, 255, 0);
    check("instancing: id2 (blue)", 32, 96, 0, 0, 255);
    check("instancing: id3 (yellow)", 96, 96, 255, 255, 0);

    // texture: 2x2 RGBA texture, nearest filtering, sampled over the target
    GLuint prog = program(kTexVs, kTexFs);
    glUseProgram(prog);
    const unsigned char tex[16] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    GLuint t;
    glGenTextures(1, &t);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    quad(v, 0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glOk("texture draw");
    check("texture: texel(0,0) red", 32, 32, 255, 0, 0);
    check("texture: texel(1,0) green", 96, 32, 0, 255, 0);
    check("texture: texel(0,1) blue", 32, 96, 0, 0, 255);
    check("texture: texel(1,1) white", 96, 96, 255, 255, 255);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
    glDeleteTextures(1, &t);
    glDeleteRenderbuffers(1, &depth);
    glDeleteBuffers(1, &vbo);
}

// ---- 3D scene ----
static void mul(float* r, const float* a, const float* b) {  // column-major 4x4
    float t[16];
    for (int c = 0; c < 4; c++)
        for (int rr = 0; rr < 4; rr++) {
            float s = 0;
            for (int k = 0; k < 4; k++) s += a[k * 4 + rr] * b[c * 4 + k];
            t[c * 4 + rr] = s;
        }
    memcpy(r, t, sizeof(t));
}

static const char* kSceneVs =
        "#version 300 es\n"
        "in vec3 aPos; in vec3 aNrm;\n"
        "uniform mat4 uProj; uniform mat4 uView; uniform float uTime;\n"
        "out vec3 vN; out vec3 vP; out vec3 vC;\n"
        "void main() {\n"
        "  int i = gl_InstanceID;\n"
        "  vec3 cell = vec3(float(i % 10) - 4.5, float((i / 10) % 10) - 4.5, float(i / 100) * -2.5);\n"
        "  float a = uTime + float(i) * 0.13;\n"
        "  mat3 r = mat3(cos(a), 0.0, -sin(a), 0.0, 1.0, 0.0, sin(a), 0.0, cos(a));\n"
        "  r = r * mat3(1.0, 0.0, 0.0, 0.0, cos(a*0.7), sin(a*0.7), 0.0, -sin(a*0.7), cos(a*0.7));\n"
        "  vec3 p = r * (aPos * 0.35) + cell * 1.1;\n"
        "  vN = r * aNrm; vP = (uView * vec4(p, 1.0)).xyz;\n"
        "  vC = 0.5 + 0.5 * vec3(sin(float(i)), sin(float(i) * 1.7), sin(float(i) * 2.3));\n"
        "  gl_Position = uProj * uView * vec4(p, 1.0);\n"
        "}\n";
static const char* kSceneFs =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec3 vN; in vec3 vP; in vec3 vC; out vec4 oColor;\n"
        "void main() {\n"
        "  vec3 n = normalize(vN); vec3 l = normalize(vec3(0.4, 0.7, 0.6));\n"
        "  vec3 v = normalize(-vP); vec3 h = normalize(l + v);\n"
        "  float d = max(dot(n, l), 0.0); float s = pow(max(dot(n, h), 0.0), 32.0);\n"
        "  oColor = vec4(vC * (0.15 + 0.85 * d) + vec3(0.6) * s, 1.0);\n"
        "}\n";

static double scene3d(EGLDisplay dpy, EGLSurface surf, int w, int h, double secs, int cubes) {
    static const float P[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                                  {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
    static const int F[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                                {3, 7, 6, 2}, {0, 4, 7, 3}, {1, 2, 6, 5}};
    static const float N[6][3] = {{0, 0, -1}, {0, 0, 1}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}};
    float vb[36 * 6];
    int k = 0;
    for (int f = 0; f < 6; f++) {
        const int idx[6] = {F[f][0], F[f][1], F[f][2], F[f][0], F[f][2], F[f][3]};
        for (int j = 0; j < 6; j++) {
            memcpy(&vb[k], P[idx[j]], 12);
            memcpy(&vb[k + 3], N[f], 12);
            k += 6;
        }
    }
    GLuint prog = program(kSceneVs, kSceneFs);
    glUseProgram(prog);
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vb), vb, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    float asp = (float)w / h, fov = 1.0f / tanf(0.5f * 1.0f), zn = 0.5f, zf = 60.0f;
    float proj[16] = {fov / asp, 0, 0, 0, 0, fov, 0, 0, 0, 0, (zf + zn) / (zn - zf), -1,
                      0, 0, 2 * zf * zn / (zn - zf), 0};
    glUniformMatrix4fv(glGetUniformLocation(prog, "uProj"), 1, GL_FALSE, proj);
    GLint uView = glGetUniformLocation(prog, "uView"), uTime = glGetUniformLocation(prog, "uTime");
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.05f, 0.05f, 0.1f, 1);
    int frames = 0;
    double t0 = now(), t = t0, worst = 0, last = t0;
    while (t - t0 < secs) {
        float tt = (float)(t - t0);
        float ry[16] = {cosf(tt * 0.3f), 0, -sinf(tt * 0.3f), 0, 0, 1, 0, 0,
                        sinf(tt * 0.3f), 0, cosf(tt * 0.3f), 0, 0, 0, 0, 1};
        float tr[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -16.0f, 1};
        float view[16];
        mul(view, tr, ry);
        glUniformMatrix4fv(uView, 1, GL_FALSE, view);
        glUniform1f(uTime, tt);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, cubes);
        eglSwapBuffers(dpy, surf);
        frames++;
        t = now();
        if (t - last > worst) worst = t - last;
        last = t;
    }
    glFinish();
    glOk("3D scene");
    double fps = frames / (now() - t0);
    printf("  %d cubes (%d triangles/frame), lit per pixel: %d frames, %.1f fps, worst frame %.1f ms\n",
           cubes, cubes * 12, frames, fps, worst * 1000);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDeleteBuffers(1, &vbo);
    glDisableVertexAttribArray(1);
    return fps;
}

static void fillrate(EGLDisplay dpy, EGLSurface surf, int w, int h, double secs, int layers) {
    GLuint p = program(kFlatVs, kFlatFs);
    glUseProgram(p);
    GLint uColor = glGetUniformLocation(p, "uColor");
    glUniform2f(glGetUniformLocation(p, "uOff"), 0, 0);
    glUniform1f(glGetUniformLocation(p, "uScale"), -1.0f);
    float v[18];
    quad(v, 0);
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, w, h);
    int frames = 0;
    double t0 = now();
    while (now() - t0 < secs) {
        glClear(GL_COLOR_BUFFER_BIT);
        for (int i = 0; i < layers; i++) {
            glUniform4f(uColor, (i % 3) / 2.0f, ((i + 1) % 3) / 2.0f, ((i + 2) % 3) / 2.0f, 0.25f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        eglSwapBuffers(dpy, surf);
        frames++;
    }
    glFinish();
    double dt = now() - t0;
    glOk("fill rate");
    printf("  %d blended full-screen layers: %.1f fps, %.0f Mpixel/s\n", layers, frames / dt,
           frames * (double)layers * w * h / dt / 1e6);
    glDisable(GL_BLEND);
    glDeleteBuffers(1, &vbo);
}

int main(int argc, char** argv) {
    double secs = argc > 1 ? atof(argv[1]) : 8.0;
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint maj, min;
    if (!eglInitialize(dpy, &maj, &min)) { printf("eglInitialize failed\n"); return 1; }
    printf("== EGL %d.%d  vendor: %s  version: %s\n", maj, min, eglQueryString(dpy, EGL_VENDOR),
           eglQueryString(dpy, EGL_VERSION));
    const EGLint cfgAttr[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, 0x40 /*ES3*/,
                              EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                              EGL_DEPTH_SIZE, 16, EGL_NONE};
    EGLConfig cfg;
    EGLint n = 0;
    int es = 3;
    if (!eglChooseConfig(dpy, cfgAttr, &cfg, 1, &n) || n < 1) {
        printf("  no ES3 config; trying ES2\n");
        EGLint a2[sizeof(cfgAttr) / sizeof(EGLint)];
        memcpy(a2, cfgAttr, sizeof(cfgAttr));
        a2[3] = EGL_OPENGL_ES2_BIT;
        eglChooseConfig(dpy, a2, &cfg, 1, &n);
        es = 2;
        gFail++;
    }
    WindowSurface ws;
    EGLSurface surf = eglCreateWindowSurface(dpy, cfg, ws.getSurface(), nullptr);
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, es, EGL_NONE};
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttr);
    if (ctx == EGL_NO_CONTEXT || !eglMakeCurrent(dpy, surf, surf, ctx)) {
        printf("context/makeCurrent failed: 0x%x\n", eglGetError());
        return 1;
    }
    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    eglSwapInterval(dpy, 0);
    GLint maxTex = 0, maxRb = 0, nExt = 0, maxVa = 0, maxDraw = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRb);
    glGetIntegerv(GL_NUM_EXTENSIONS, &nExt);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVa);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDraw);
    printf("== GL_VENDOR: %s\n   GL_RENDERER: %s\n   GL_VERSION: %s\n   GLSL: %s\n",
           glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION),
           glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("   context ES%d, surface %dx%d, max texture %d, max renderbuffer %d, extensions %d, "
           "vertex attribs %d, draw buffers %d\n",
           es, w, h, maxTex, maxRb, nExt, maxVa, maxDraw);
    glOk("init");
    correctness();
    printf("== 3D scene (window %dx%d, swap interval 0, %.0f s each)\n", w, h, secs);
    scene3d(dpy, surf, w, h, secs, 100);
    scene3d(dpy, surf, w, h, secs, 400);
    printf("== fill rate\n");
    fillrate(dpy, surf, w, h, secs / 2, 4);
    fillrate(dpy, surf, w, h, secs / 2, 8);
    printf("== RESULT: %s (%d failures)\n", gFail ? "FAIL" : "PASS", gFail);
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);
    eglDestroySurface(dpy, surf);
    eglTerminate(dpy);
    return gFail ? 2 : 0;
}
