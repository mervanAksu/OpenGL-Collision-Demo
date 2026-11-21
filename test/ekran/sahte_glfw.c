// Ekran testi icin GLFW ve GLEW'in yerini tutan kucuk katman (Linux + X11 + GLX).
// Program gercek OpenGL ile cizer; pencere Xvfb uzerinde acilir, girdiler senaryo
// dosyasindan gelir. Her karede GL hatasi ve durum sizintisi denetlenir, istenince
// ekrandan piksel okunur. Zaman kare/60 olarak ilerler; "zaman_atla" ile takilma eklenir.
// PENCERE_BOYUTU=GxY ortam degiskeni programin istedigi pencere boyutunu degistirir.
#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GL/glxew.h>
#include <GLFW/glfw3.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct GLFWwindow {
    Display* ekran;
    Window pen;
    GLXContext baglam;
    int en, boy;
    int kapat;
    char baslik[256];
    GLFWkeyfun tus_geri;
    GLFWcursorposfun fare_geri;
    GLFWscrollfun tekerlek_geri;
    GLFWframebuffersizefun boyut_geri;
    GLFWmousebuttonfun dugme_geri;
};

static GLFWwindow* tek_pencere = NULL;
static Display* ekran = NULL;
static FILE* senaryo = NULL;
static int satir_no = 0;
static long kare = 0;
static double zaman_eki = 0.0;
static int bekle = 0;
static long cizim_sayisi = 0;
static int tus_durum[GLFW_KEY_LAST + 1];
static int dugme_durum[GLFW_MOUSE_BUTTON_LAST + 1];
static double fare_x = 0.0, fare_y = 0.0;

static int kontrol_sayisi = 0, hata_sayisi = 0;
static int gl_hata_kare = 0, durum_ihlal_kare = 0;
static int yakala_istegi = 0;
static unsigned char* goruntu[2] = { NULL, NULL };   // [1] son, [0] bir onceki
static int goruntu_en[2], goruntu_boy[2];

// glew.h'deki isaretciler; program yalnizca tampon fonksiyonlarini kullaniyor
PFNGLGENBUFFERSPROC __glewGenBuffers = NULL;
PFNGLBINDBUFFERPROC __glewBindBuffer = NULL;
PFNGLBUFFERDATAPROC __glewBufferData = NULL;
PFNGLDELETEBUFFERSPROC __glewDeleteBuffers = NULL;
GLboolean glewExperimental = GL_FALSE;

GLenum glewInit(void) {
    __glewGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddressARB((const GLubyte*)"glGenBuffers");
    __glewBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddressARB((const GLubyte*)"glBindBuffer");
    __glewBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddressARB((const GLubyte*)"glBufferData");
    __glewDeleteBuffers =
        (PFNGLDELETEBUFFERSPROC)glXGetProcAddressARB((const GLubyte*)"glDeleteBuffers");
    if (!__glewGenBuffers || !__glewBindBuffer || !__glewBufferData || !__glewDeleteBuffers)
        return GLEW_ERROR_NO_GL_VERSION;
    return GLEW_OK;
}

static void sonuc(int iyi, const char* ne) {
    kontrol_sayisi++;
    if (!iyi) hata_sayisi++;
    printf("[kare %4ld, satir %3d] %s: %s\n", kare, satir_no, iyi ? "tamam" : "HATA ", ne);
}

int glfwInit(void) {
    ekran = XOpenDisplay(NULL);
    if (ekran == NULL) {
        printf("X ekrani acilamadi (xvfb-run ile calistirin)\n");
        return GLFW_FALSE;
    }
    const char* yol = getenv("SENARYO");
    if (yol != NULL) {
        senaryo = fopen(yol, "r");
        if (senaryo == NULL) printf("senaryo acilamadi: %s\n", yol);
    }
    return GLFW_TRUE;
}

void glfwInitHint(int ipucu, int deger) { (void)ipucu; (void)deger; }
int glfwPlatformSupported(int platform) { return platform == GLFW_PLATFORM_X11; }

GLFWwindow* glfwCreateWindow(int en, int boy, const char* baslik, GLFWmonitor* m, GLFWwindow* p) {
    (void)m; (void)p;
    int ozellik[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
                      GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, None };
    XVisualInfo* gorsel = glXChooseVisual(ekran, DefaultScreen(ekran), ozellik);
    if (gorsel == NULL) return NULL;
    const char* zorla = getenv("PENCERE_BOYUTU");
    if (zorla != NULL) sscanf(zorla, "%dx%d", &en, &boy);
    GLFWwindow* w = (GLFWwindow*)calloc(1, sizeof(GLFWwindow));
    w->ekran = ekran;
    w->en = en;
    w->boy = boy;
    XSetWindowAttributes swa;
    memset(&swa, 0, sizeof(swa));
    swa.colormap = XCreateColormap(ekran, RootWindow(ekran, gorsel->screen), gorsel->visual,
                                   AllocNone);
    w->pen = XCreateWindow(ekran, RootWindow(ekran, gorsel->screen), 0, 0, (unsigned)en,
                           (unsigned)boy, 0, gorsel->depth, InputOutput, gorsel->visual,
                           CWColormap, &swa);
    XMapWindow(ekran, w->pen);
    XSync(ekran, False);
    w->baglam = glXCreateContext(ekran, gorsel, NULL, True);
    XFree(gorsel);
    snprintf(w->baslik, sizeof(w->baslik), "%s", baslik);
    tek_pencere = w;
    return w;
}

void glfwMakeContextCurrent(GLFWwindow* w) { glXMakeCurrent(w->ekran, w->pen, w->baglam); }
void glfwSwapInterval(int a) { (void)a; }
GLFWkeyfun glfwSetKeyCallback(GLFWwindow* w, GLFWkeyfun f) { w->tus_geri = f; return NULL; }
GLFWcursorposfun glfwSetCursorPosCallback(GLFWwindow* w, GLFWcursorposfun f) {
    w->fare_geri = f;
    return NULL;
}
GLFWscrollfun glfwSetScrollCallback(GLFWwindow* w, GLFWscrollfun f) {
    w->tekerlek_geri = f;
    return NULL;
}
GLFWframebuffersizefun glfwSetFramebufferSizeCallback(GLFWwindow* w, GLFWframebuffersizefun f) {
    w->boyut_geri = f;
    return NULL;
}
GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow* w, GLFWmousebuttonfun f) {
    w->dugme_geri = f;
    return NULL;
}
void glfwGetFramebufferSize(GLFWwindow* w, int* en, int* boy) { *en = w->en; *boy = w->boy; }
void glfwGetCursorPos(GLFWwindow* w, double* x, double* y) { (void)w; *x = fare_x; *y = fare_y; }
double glfwGetTime(void) { return (double)kare / 60.0 + zaman_eki; }
int glfwWindowShouldClose(GLFWwindow* w) { return w->kapat; }
void glfwSetWindowShouldClose(GLFWwindow* w, int deger) { w->kapat = deger; }
int glfwGetKey(GLFWwindow* w, int tus) { (void)w; return tus_durum[tus]; }
int glfwGetMouseButton(GLFWwindow* w, int d) { (void)w; return dugme_durum[d]; }
void glfwSetWindowTitle(GLFWwindow* w, const char* b) {
    snprintf(w->baslik, sizeof(w->baslik), "%s", b);
}

static int tus_kodu(const char* ad) {
    static const struct { const char* ad; int kod; } tablo[] = {
        { "SAG", GLFW_KEY_RIGHT }, { "SOL", GLFW_KEY_LEFT }, { "YUKARI", GLFW_KEY_UP },
        { "ASAGI", GLFW_KEY_DOWN }, { "PGUP", GLFW_KEY_PAGE_UP }, { "PGDN", GLFW_KEY_PAGE_DOWN },
        { "TAB", GLFW_KEY_TAB }, { "ESC", GLFW_KEY_ESCAPE },
    };
    for (size_t i = 0; i < sizeof(tablo) / sizeof(tablo[0]); i++)
        if (strcmp(ad, tablo[i].ad) == 0) return tablo[i].kod;
    if (strlen(ad) == 1 && ad[0] >= 'A' && ad[0] <= 'Z') return GLFW_KEY_A + (ad[0] - 'A');
    if (strlen(ad) == 1 && ad[0] >= '0' && ad[0] <= '9') return GLFW_KEY_0 + (ad[0] - '0');
    return -1;
}

static void tus_gonder(int tus, int eylem) {
    tus_durum[tus] = eylem == GLFW_RELEASE ? GLFW_RELEASE : GLFW_PRESS;
    if (tek_pencere->tus_geri) tek_pencere->tus_geri(tek_pencere, tus, 0, eylem, 0);
}

// piksel siniflari: tel kafes renkleri tam degerle cizilir, modeller isikli oldugu icin tutmaz
static int sinifta(const unsigned char* p, const char* sinif) {
    int r = p[0], g = p[1], b = p[2];
    int kirmizi = r == 255 && g == 0 && b == 0;
    int yesil = r == 0 && g == 255 && b == 0;
    int sari = r == 255 && g == 255 && b == 0;
    if (strcmp(sinif, "kirmizi") == 0) return kirmizi;
    if (strcmp(sinif, "yesil") == 0) return yesil;
    if (strcmp(sinif, "sari") == 0) return sari;
    // cok koyu (bomba gibi siyah modeller); arka plan ve izgara acik oldugu icin karismaz
    if (strcmp(sinif, "koyu") == 0) return r < 40 && g < 40 && b < 40;
    if (strcmp(sinif, "renkli") == 0) {
        int en_buyuk = r > g ? (r > b ? r : b) : (g > b ? g : b);
        int en_kucuk = r < g ? (r < b ? r : b) : (g < b ? g : b);
        return en_buyuk - en_kucuk > 40 && !kirmizi && !yesil && !sari;
    }
    return 0;
}

// (x0,y0)-(x1,y1) ust-sol koseden olculur; goruntu GL'deki gibi alttan yukari saklanir
static long bolgede_say(int x0, int y0, int x1, int y1, const char* sinif) {
    long adet = 0;
    if (goruntu[1] == NULL) return -1;
    for (int y = y0; y <= y1; y++) {
        int satir = goruntu_boy[1] - 1 - y;
        if (satir < 0 || satir >= goruntu_boy[1]) continue;
        for (int x = x0; x <= x1; x++) {
            if (x < 0 || x >= goruntu_en[1]) continue;
            adet += sinifta(goruntu[1] + ((size_t)satir * goruntu_en[1] + x) * 3, sinif);
        }
    }
    return adet;
}

static unsigned char* ppm_oku(const char* yol, int* en, int* boy) {
    FILE* f = fopen(yol, "rb");
    if (f == NULL) return NULL;
    unsigned char* veri = NULL;
    int en_deger = 255;
    if (fscanf(f, "P6 %d %d %d", en, boy, &en_deger) == 3 && fgetc(f) != EOF) {
        veri = (unsigned char*)malloc((size_t)*en * *boy * 3);
        // dosya ustten alta yazildi, bellekte GL gibi alttan uste tutulur
        for (int y = *boy - 1; y >= 0; y--)
            if (fread(veri + (size_t)y * *en * 3, 3, (size_t)*en, f) != (size_t)*en) {
                free(veri);
                veri = NULL;
                break;
            }
    }
    fclose(f);
    return veri;
}

static int karsilastir(long deger, const char* op, long sinir) {
    if (strcmp(op, ">") == 0) return deger > sinir;
    if (strcmp(op, "<") == 0) return deger < sinir;
    if (strcmp(op, "=") == 0) return deger == sinir;
    return 0;
}

static void komut_isle(char* satir) {
    char* yeni = strchr(satir, '\n');
    if (yeni) *yeni = 0;
    char* k = strtok(satir, " \t");
    if (k == NULL || k[0] == '#') return;
    char* kalan = strtok(NULL, "");
    while (kalan && (*kalan == ' ' || *kalan == '\t')) kalan++;
    char mesaj[512];

    if (strcmp(k, "kare") == 0) {
        bekle = atoi(kalan);
    } else if (strcmp(k, "bas") == 0 || strcmp(k, "birak") == 0 || strcmp(k, "tikla") == 0) {
        int t = tus_kodu(kalan);
        if (t < 0) { sonuc(0, "bilinmeyen tus"); return; }
        if (strcmp(k, "birak") != 0) tus_gonder(t, GLFW_PRESS);
        if (strcmp(k, "bas") != 0) tus_gonder(t, GLFW_RELEASE);
    } else if (strcmp(k, "fare") == 0) {
        sscanf(kalan, "%lf %lf", &fare_x, &fare_y);
        if (tek_pencere->fare_geri) tek_pencere->fare_geri(tek_pencere, fare_x, fare_y);
    } else if (strcmp(k, "imlec") == 0) {
        // imlec yeri degisir ama programa hareket olayi gitmez (pencereye disaridan girme gibi)
        sscanf(kalan, "%lf %lf", &fare_x, &fare_y);
    } else if (strcmp(k, "fare_bas") == 0 || strcmp(k, "fare_birak") == 0) {
        int d = strcmp(kalan, "SAG") == 0 ? GLFW_MOUSE_BUTTON_RIGHT : GLFW_MOUSE_BUTTON_LEFT;
        int eylem = strcmp(k, "fare_bas") == 0 ? GLFW_PRESS : GLFW_RELEASE;
        dugme_durum[d] = eylem;
        if (tek_pencere->dugme_geri) tek_pencere->dugme_geri(tek_pencere, d, eylem, 0);
    } else if (strcmp(k, "tekerlek") == 0) {
        if (tek_pencere->tekerlek_geri) tek_pencere->tekerlek_geri(tek_pencere, 0.0, atof(kalan));
    } else if (strcmp(k, "boyut") == 0) {
        int en = 0, boy = 0;
        sscanf(kalan, "%d %d", &en, &boy);
        // 0x0 simge durumudur: X penceresi oldugu gibi kalir, cerceve tamponu 0 bildirilir
        if (en > 0 && boy > 0) {
            XResizeWindow(ekran, tek_pencere->pen, (unsigned)en, (unsigned)boy);
            XSync(ekran, False);
        }
        tek_pencere->en = en;
        tek_pencere->boy = boy;
        if (tek_pencere->boyut_geri) tek_pencere->boyut_geri(tek_pencere, en, boy);
    } else if (strcmp(k, "yakala") == 0) {
        yakala_istegi = 1;
        bekle = 1;
    } else if (strcmp(k, "baslik_icerir") == 0 || strcmp(k, "baslik_icermez") == 0) {
        int var = strstr(tek_pencere->baslik, kalan) != NULL;
        int iyi = strcmp(k, "baslik_icerir") == 0 ? var : !var;
        snprintf(mesaj, sizeof(mesaj), "%s \"%s\"  (baslik: %s)", k, kalan, tek_pencere->baslik);
        sonuc(iyi, mesaj);
    } else if (strcmp(k, "piksel") == 0) {
        char sinif[32], op[4];
        long sinir = 0, adet = 0;
        sscanf(kalan, "%31s %3s %ld", sinif, op, &sinir);
        if (goruntu[1] != NULL) {
            long n = (long)goruntu_en[1] * goruntu_boy[1];
            for (long i = 0; i < n; i++) adet += sinifta(goruntu[1] + i * 3, sinif);
        }
        snprintf(mesaj, sizeof(mesaj), "%s piksel %s %ld  (bulunan %ld)", sinif, op, sinir, adet);
        sonuc(goruntu[1] != NULL && karsilastir(adet, op, sinir), mesaj);
    } else if (strcmp(k, "piksel_bolge") == 0) {
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        char sinif[32], op[4];
        long sinir = 0;
        sscanf(kalan, "%d %d %d %d %31s %3s %ld", &x0, &y0, &x1, &y1, sinif, op, &sinir);
        long adet = bolgede_say(x0, y0, x1, y1, sinif);
        snprintf(mesaj, sizeof(mesaj), "(%d,%d)-(%d,%d) bolgesinde %s piksel %s %ld  (bulunan %ld)",
                 x0, y0, x1, y1, sinif, op, sinir, adet);
        sonuc(adet >= 0 && karsilastir(adet, op, sinir), mesaj);
    } else if (strcmp(k, "karsilastir") == 0) {
        char yol[256], op[4];
        long sinir = 0, adet = -1;
        int en = 0, boy = 0;
        sscanf(kalan, "%255s %3s %ld", yol, op, &sinir);
        unsigned char* kayit = ppm_oku(yol, &en, &boy);
        if (kayit && goruntu[1] && en == goruntu_en[1] && boy == goruntu_boy[1]) {
            adet = 0;
            for (long i = 0; i < (long)en * boy * 3; i += 3)
                if (memcmp(kayit + i, goruntu[1] + i, 3) != 0) adet++;
        }
        free(kayit);
        snprintf(mesaj, sizeof(mesaj), "%s ile farkli piksel %s %ld  (bulunan %ld)", yol, op,
                 sinir, adet);
        sonuc(adet >= 0 && karsilastir(adet, op, sinir), mesaj);
    } else if (strcmp(k, "zaman_atla") == 0) {
        zaman_eki += atof(kalan);
    } else if (strcmp(k, "cizim_say") == 0) {
        cizim_sayisi = 0;
    } else if (strcmp(k, "cizim") == 0) {
        char op[4];
        long sinir = 0;
        sscanf(kalan, "%3s %ld", op, &sinir);
        snprintf(mesaj, sizeof(mesaj), "cizilen kare sayisi %s %ld  (bulunan %ld)", op, sinir,
                 cizim_sayisi);
        sonuc(karsilastir(cizim_sayisi, op, sinir), mesaj);
    } else if (strcmp(k, "degisim") == 0) {
        char op[4];
        long sinir = 0, adet = -1;
        sscanf(kalan, "%3s %ld", op, &sinir);
        if (goruntu[0] && goruntu[1] && goruntu_en[0] == goruntu_en[1]
            && goruntu_boy[0] == goruntu_boy[1]) {
            adet = 0;
            long n = (long)goruntu_en[1] * goruntu_boy[1] * 3;
            for (long i = 0; i < n; i += 3)
                if (memcmp(goruntu[0] + i, goruntu[1] + i, 3) != 0) adet++;
        }
        snprintf(mesaj, sizeof(mesaj), "son iki goruntude degisen piksel %s %ld  (bulunan %ld)",
                 op, sinir, adet);
        sonuc(adet >= 0 && karsilastir(adet, op, sinir), mesaj);
    } else if (strcmp(k, "kaydet") == 0) {
        FILE* f = goruntu[1] ? fopen(kalan, "wb") : NULL;
        if (f) {
            fprintf(f, "P6\n%d %d\n255\n", goruntu_en[1], goruntu_boy[1]);
            for (int y = goruntu_boy[1] - 1; y >= 0; y--)
                fwrite(goruntu[1] + (size_t)y * goruntu_en[1] * 3, 3, (size_t)goruntu_en[1], f);
            fclose(f);
        }
    } else if (strcmp(k, "kapat") == 0) {
        tek_pencere->kapat = 1;
    } else {
        snprintf(mesaj, sizeof(mesaj), "bilinmeyen komut: %s", k);
        sonuc(0, mesaj);
    }
}

void glfwWaitEvents(void) { glfwPollEvents(); }

void glfwPollEvents(void) {
    kare++;
    if (bekle > 0) {
        bekle--;
        if (bekle > 0) return;
    }
    char satir[512];
    while (bekle == 0 && !tek_pencere->kapat) {
        if (senaryo == NULL) {
            if (kare > 120) tek_pencere->kapat = 1;
            return;
        }
        if (fgets(satir, sizeof(satir), senaryo) == NULL) {
            tek_pencere->kapat = 1;
            return;
        }
        satir_no++;
        komut_isle(satir);
    }
}

// cizim sonunda programin GL durumunu geri birakip birakmadigini denetler
static void durum_denetle(void) {
    GLenum e;
    int gl_hata = 0;
    while ((e = glGetError()) != GL_NO_ERROR) {
        if (gl_hata_kare < 5) printf("[kare %4ld] GL hatasi 0x%04x\n", kare, e);
        gl_hata = 1;
    }
    gl_hata_kare += gl_hata;

    GLint ozellik_yigini = 0, istemci_yigini = 0, mv_yigini = 0, pr_yigini = 0, tampon = 0;
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &ozellik_yigini);
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH, &istemci_yigini);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &mv_yigini);
    glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &pr_yigini);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &tampon);
    GLint matris_kipi = 0;
    GLfloat cizgi = 0.0f;
    glGetIntegerv(GL_MATRIX_MODE, &matris_kipi);
    glGetFloatv(GL_LINE_WIDTH, &cizgi);
    // cizim fonksiyonlari actiklarini kapatmali: kare sonunda yalnizca derinlik testi acik
    int acik_kalan = glIsEnabled(GL_LIGHTING) || glIsEnabled(GL_LIGHT0)
        || glIsEnabled(GL_COLOR_MATERIAL) || glIsEnabled(GL_VERTEX_ARRAY)
        || glIsEnabled(GL_NORMAL_ARRAY) || glIsEnabled(GL_COLOR_ARRAY);
    int ihlal = ozellik_yigini != 0 || istemci_yigini != 0 || mv_yigini != 1 || pr_yigini != 1
        || tampon != 0 || !glIsEnabled(GL_DEPTH_TEST) || acik_kalan
        || matris_kipi != GL_MODELVIEW || cizgi != 1.0f;
    if (ihlal && durum_ihlal_kare < 5)
        printf("[kare %4ld] GL durumu sizdi: attrib=%d client=%d mv=%d pr=%d vbo=%d derinlik=%d "
               "acik_kalan=%d matris_kipi=0x%x cizgi=%.1f\n", kare, ozellik_yigini,
               istemci_yigini, mv_yigini, pr_yigini, tampon, glIsEnabled(GL_DEPTH_TEST),
               acik_kalan, matris_kipi, cizgi);
    durum_ihlal_kare += ihlal;
}

void glfwSwapBuffers(GLFWwindow* w) {
    cizim_sayisi++;
    durum_denetle();
    if (yakala_istegi) {
        yakala_istegi = 0;
        free(goruntu[0]);
        goruntu[0] = goruntu[1];
        goruntu_en[0] = goruntu_en[1];
        goruntu_boy[0] = goruntu_boy[1];
        goruntu[1] = (unsigned char*)malloc((size_t)w->en * w->boy * 3);
        goruntu_en[1] = w->en;
        goruntu_boy[1] = w->boy;
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, w->en, w->boy, GL_RGB, GL_UNSIGNED_BYTE, goruntu[1]);
    }
    glXSwapBuffers(w->ekran, w->pen);
}

void glfwTerminate(void) {
    sonuc(gl_hata_kare == 0, "hicbir karede GL hatasi yok");
    sonuc(durum_ihlal_kare == 0, "hicbir karede GL durumu sizmadi");
    printf("EKRAN TESTI: %ld kare, %d kontrol, %d hata\n", kare, kontrol_sayisi, hata_sayisi);
    if (tek_pencere) {
        glXMakeCurrent(ekran, None, NULL);
        glXDestroyContext(ekran, tek_pencere->baglam);
        XDestroyWindow(ekran, tek_pencere->pen);
        free(tek_pencere);
        tek_pencere = NULL;
    }
    free(goruntu[0]);
    free(goruntu[1]);
    if (senaryo) fclose(senaryo);
    if (ekran) XCloseDisplay(ekran);
    fflush(stdout);
    if (hata_sayisi > 0) exit(1);
}
