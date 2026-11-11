/*
 * ef52_pbtest: GPU work with a small pbuffer current, as HWUI and Chromium
 * (WebView) do on drivers without EGL_KHR_surfaceless_context.
 *   ef52_pbtest <pbuffer w> <pbuffer h> <mode> [iterations]
 *     mode fbo:   draw into a 512x512 FBO while the pbuffer is current
 *     mode clear: clear/draw the pbuffer's own (default) framebuffer
 * Run, then look for "GPU PAGE FAULT" / "gpu fault" in dmesg.
 */
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* VS = "attribute vec2 p; void main(){ gl_Position = vec4(p, 0.0, 1.0); }";
static const char* FS = "precision mediump float; uniform vec4 c; void main(){ gl_FragColor = c; }";

static GLuint sh(GLenum t, const char* s) {
    GLuint h = glCreateShader(t);
    glShaderSource(h, 1, &s, nullptr);
    glCompileShader(h);
    return h;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s <w> <h> fbo|clear [iterations]\n", argv[0]);
        return 2;
    }
    int w = atoi(argv[1]), h = atoi(argv[2]);
    bool fbo = !strcmp(argv[3], "fbo");
    int iters = argc > 4 ? atoi(argv[4]) : 300;
    // 6th arg: depth/stencil bits of the pbuffer config ("ds" = 24/8)
    bool ds = argc > 5 && !strcmp(argv[5], "ds");
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(dpy, nullptr, nullptr);
    const EGLint ca[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
                         EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                         EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                         EGL_DEPTH_SIZE, ds ? 24 : 0, EGL_STENCIL_SIZE, ds ? 8 : 0, EGL_NONE};
    EGLConfig cfg;
    EGLint n = 0;
    if (!eglChooseConfig(dpy, ca, &cfg, 1, &n) || n < 1) {
        printf("no pbuffer config\n");
        return 1;
    }
    const EGLint sa[] = {EGL_WIDTH, w, EGL_HEIGHT, h, EGL_NONE};
    EGLSurface pb = eglCreatePbufferSurface(dpy, cfg, sa);
    const EGLint xa[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, xa);
    if (pb == EGL_NO_SURFACE || ctx == EGL_NO_CONTEXT || !eglMakeCurrent(dpy, pb, pb, ctx)) {
        printf("egl setup failed 0x%x\n", eglGetError());
        return 1;
    }
    EGLint qw = 0, qh = 0;
    eglQuerySurface(dpy, pb, EGL_WIDTH, &qw);
    eglQuerySurface(dpy, pb, EGL_HEIGHT, &qh);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, sh(GL_VERTEX_SHADER, VS));
    glAttachShader(prog, sh(GL_FRAGMENT_SHADER, FS));
    glBindAttribLocation(prog, 0, "p");
    glLinkProgram(prog);
    glUseProgram(prog);
    GLint uc = glGetUniformLocation(prog, "c");
    const float tri[] = {-1, -1, 3, -1, -1, 3};
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, tri);
    glEnableVertexAttribArray(0);
    GLuint tex = 0, fb = 0;
    if (fbo) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glGenFramebuffers(1, &fb);
        glBindFramebuffer(GL_FRAMEBUFFER, fb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glViewport(0, 0, 512, 512);
    } else {
        glViewport(0, 0, w, h);
    }
    for (int i = 0; i < iters; i++) {
        glClearColor((i & 1) ? 1.f : 0.f, 0.5f, 0.f, 1.f);
        if (ds && !fbo) {
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        } else {
            glClear(GL_COLOR_BUFFER_BIT);
        }
        glUniform4f(uc, 0.f, (i % 7) / 7.f, 1.f, 1.f);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        if (i % 10 == 9) glFinish();
    }
    glFinish();
    GLenum err = glGetError();
    printf("pbuffer %dx%d (queried %dx%d) mode=%s iters=%d glError=0x%x eglError=0x%x\n", w, h, qw,
           qh, fbo ? "fbo" : "clear", iters, err, eglGetError());
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(dpy, pb);
    eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);
    return 0;
}
