const int NESNE_SAYISI = 5;
const float HAREKET_HIZI = 3.0f;    // birim/saniye
const float DONME_HIZI = 90.0f;     // derece/saniye
const float KAMERA_TUS_HIZI = 250.0f;   // klavyeyle donuste saniyedeki "piksel" karsiligi

struct Nesne {
    const char* ad;
    FbxModel model;
    PhysObj sekil;
    glm::vec3 konum;      // modelin taban merkezi
    float aci;            // y ekseni etrafinda, derece
    glm::vec3 uzun_eksen; // yatay kapsul icin modelin uzun kenari (model uzayi)
};

Nesne nesneler[NESNE_SAYISI];
CollisionSystem carpisma;
Camera kamera;
Frustum frustum;

int secili = 0;
bool sekilleri_goster = true;
bool kirpma_acik = true;
bool frustum_kilitli = false;
int pencere_en = 1000, pencere_boy = 700;
double fare_x = 0.0, fare_y = 0.0;

// modelin kutusuna bakarak carpisma seklinin olculerini cikarir
void sekil_olustur(Nesne& n, SekilTipi tip, bool yatay) {
    glm::vec3 boyut = n.model.kutu_max - n.model.kutu_min;
    n.sekil.tip = tip;
    n.sekil.eksen = glm::vec3(0.0f, 1.0f, 0.0f);
    n.sekil.carpisti = false;
    n.uzun_eksen = glm::vec3(0.0f, 1.0f, 0.0f);

    if (tip == KURE) {
        // kutunun kosegenleri disarida kalsin diye yarim kosegenden biraz kucuk
        n.sekil.yaricap = glm::length(boyut) * 0.5f * 0.8f;
        n.sekil.boy = 0.0f;
    } else if (tip == SILINDIR) {
        n.sekil.yaricap = glm::max(boyut.x, boyut.z) * 0.5f;
        n.sekil.boy = boyut.y;
    } else if (yatay) {
        if (boyut.x > boyut.z) {
            n.uzun_eksen = glm::vec3(1.0f, 0.0f, 0.0f);
            n.sekil.boy = boyut.x;
            n.sekil.yaricap = glm::max(boyut.y, boyut.z) * 0.5f;
        } else {
            n.uzun_eksen = glm::vec3(0.0f, 0.0f, 1.0f);
            n.sekil.boy = boyut.z;
            n.sekil.yaricap = glm::max(boyut.y, boyut.x) * 0.5f;
        }
    } else {
        // insan gibi dik modeller: kollar acik olabilir, dar kenari al
        float dar = glm::min(boyut.x, boyut.z) * 0.5f;
        n.sekil.yaricap = glm::max(dar, boyut.y * 0.15f);
        n.sekil.boy = boyut.y;
    }
}

// nesne tasininca ya da donunce sekli de arkasindan gotur
void sekli_guncelle(Nesne& n) {
    float yukseklik = n.model.kutu_max.y - n.model.kutu_min.y;
    n.sekil.konum = n.konum + glm::vec3(0.0f, yukseklik * 0.5f, 0.0f);
    if (n.sekil.tip == KAPSUL && n.uzun_eksen.y == 0.0f) {
        float a = glm::radians(n.aci);
        glm::vec3 e = n.uzun_eksen;
        n.sekil.eksen = glm::vec3(e.x * cosf(a) + e.z * sinf(a), 0.0f,
                                  -e.x * sinf(a) + e.z * cosf(a));
    }
}

void baslangic_konumlari() {
    nesneler[0].konum = glm::vec3(-4.0f, 0.0f, 2.0f);
    nesneler[1].konum = glm::vec3(0.0f, 0.0f, 3.0f);
    nesneler[2].konum = glm::vec3(3.5f, 0.0f, -3.0f);
    nesneler[3].konum = glm::vec3(-2.0f, 0.0f, -2.5f);
    nesneler[4].konum = glm::vec3(3.0f, 0.0f, 2.0f);
    for (int i = 0; i < NESNE_SAYISI; i++) {
        nesneler[i].aci = 0.0f;
        sekli_guncelle(nesneler[i]);
    }
}

bool sahneyi_kur() {
    const char* dosya[NESNE_SAYISI] = { "models/araba.fbx", "models/insan.fbx", "models/agac.fbx",
                                        "models/kasa.fbx", "models/varil.fbx" };
    const char* ad[NESNE_SAYISI] = { "araba", "insan", "agac", "kasa", "varil" };
    float boy[NESNE_SAYISI] = { 3.2f, 1.8f, 4.0f, 1.2f, 1.1f };
    SekilTipi tip[NESNE_SAYISI] = { KAPSUL, KAPSUL, SILINDIR, KURE, SILINDIR };

    for (int i = 0; i < NESNE_SAYISI; i++) {
        nesneler[i].ad = ad[i];
        if (!nesneler[i].model.yukle(dosya[i], boy[i])) return false;
        sekil_olustur(nesneler[i], tip[i], i == 0);
        carpisma.nesneler.push_back(&nesneler[i].sekil);
    }
    baslangic_konumlari();
    return true;
}

void zemin_ciz() {
    glColor3f(0.35f, 0.35f, 0.38f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; i++) {
        glVertex3f((float)i, 0.0f, -10.0f); glVertex3f((float)i, 0.0f, 10.0f);
        glVertex3f(-10.0f, 0.0f, (float)i); glVertex3f(10.0f, 0.0f, (float)i);
    }
    glEnd();
}

void baslik_yaz(GLFWwindow* pencere, int cizilen) {
    static int onceki_cizilen = -1, onceki_cift = -1, onceki_secili = -1;
    int cift = carpisma.carpisan_cift;
    if (cizilen == onceki_cizilen && cift == onceki_cift && secili == onceki_secili) return;
    onceki_cizilen = cizilen;
    onceki_cift = cift;
    onceki_secili = secili;
    char yazi[160];
    snprintf(yazi, sizeof(yazi), "Carpisma Demo | cizilen: %d/%d | carpisan cift: %d | secili: %s",
             cizilen, NESNE_SAYISI, cift, nesneler[secili].ad);
    glfwSetWindowTitle(pencere, yazi);
}

void sahne_ciz(GLFWwindow* pencere) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float oran = (float)pencere_en / (float)pencere_boy;
    glm::mat4 projeksiyon = glm::perspective(glm::radians(55.0f), oran, 0.1f, 80.0f);
    glm::mat4 bakis = kamera.bakis();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projeksiyon));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(bakis));

    // isik konumu kamera matrisinden sonra verilirse dunyada sabit kalir
    float isik[4] = { 6.0f, 12.0f, 8.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, isik);

    // K ile kilitlenince frustum eski kamerada kalir, kirpilan nesneler disaridan gorulebilir
    if (!frustum_kilitli) frustum.guncelle(projeksiyon * bakis);

    zemin_ciz();

    int cizilen = 0;
    bool gorunur[NESNE_SAYISI];
    for (int i = 0; i < NESNE_SAYISI; i++) {
        Nesne& n = nesneler[i];
        float model_r = glm::length(n.model.kutu_max - n.model.kutu_min) * 0.5f;
        float r = glm::max(model_r, sinir_yaricapi(n.sekil));
        gorunur[i] = !kirpma_acik || frustum.kure_gorunur(n.sekil.konum, r);
        if (!gorunur[i]) continue;
        cizilen++;

        glPushMatrix();
        glTranslatef(n.konum.x, n.konum.y, n.konum.z);
        glRotatef(n.aci, 0.0f, 1.0f, 0.0f);
        n.model.ciz();
        glPopMatrix();
    }

    // tel kafesler en son ve derinlik testi kapaliyken: modelin icinde kalan cizgiler de gorunsun
    if (sekilleri_goster) {
        glDisable(GL_DEPTH_TEST);
        for (int i = 0; i < NESNE_SAYISI; i++) {
            if (!gorunur[i]) continue;
            glLineWidth(i == secili ? 2.5f : 1.0f);
            sekil_ciz(nesneler[i].sekil);
        }
        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
    }
    baslik_yaz(pencere, cizilen);
}

// basili tutulan tuslar: secili nesneyi tasir ve dondurur
void hareket_et(GLFWwindow* pencere, float dt) {
    Nesne& n = nesneler[secili];
    glm::vec3 yon(0.0f);
    if (glfwGetKey(pencere, GLFW_KEY_LEFT) == GLFW_PRESS) yon.x -= 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_RIGHT) == GLFW_PRESS) yon.x += 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_UP) == GLFW_PRESS) yon.z -= 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_DOWN) == GLFW_PRESS) yon.z += 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_PAGE_UP) == GLFW_PRESS) yon.y += 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) yon.y -= 1.0f;
    n.konum += yon * (HAREKET_HIZI * dt);
    if (n.konum.y < 0.0f) n.konum.y = 0.0f;

    if (glfwGetKey(pencere, GLFW_KEY_Q) == GLFW_PRESS) n.aci += DONME_HIZI * dt;
    if (glfwGetKey(pencere, GLFW_KEY_E) == GLFW_PRESS) n.aci -= DONME_HIZI * dt;
    sekli_guncelle(n);
}

// kamera fare olmadan da kullanilabilsin: WASD cevirir, Z/X yaklastirir
void kamera_klavye(GLFWwindow* pencere, float dt) {
    float adim = KAMERA_TUS_HIZI * dt;
    if (glfwGetKey(pencere, GLFW_KEY_A) == GLFW_PRESS) kamera.dondur(-adim, 0.0f);
    if (glfwGetKey(pencere, GLFW_KEY_D) == GLFW_PRESS) kamera.dondur(adim, 0.0f);
    if (glfwGetKey(pencere, GLFW_KEY_W) == GLFW_PRESS) kamera.dondur(0.0f, adim);
    if (glfwGetKey(pencere, GLFW_KEY_S) == GLFW_PRESS) kamera.dondur(0.0f, -adim);
    if (glfwGetKey(pencere, GLFW_KEY_Z) == GLFW_PRESS) kamera.yaklas(5.0f * dt);
    if (glfwGetKey(pencere, GLFW_KEY_X) == GLFW_PRESS) kamera.yaklas(-5.0f * dt);
}

void tus_olayi(GLFWwindow* pencere, int tus, int, int eylem, int) {
    if (eylem != GLFW_PRESS) return;
    if (tus == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(pencere, GLFW_TRUE);
    if (tus >= GLFW_KEY_1 && tus <= GLFW_KEY_5) secili = tus - GLFW_KEY_1;
    if (tus == GLFW_KEY_TAB) secili = (secili + 1) % NESNE_SAYISI;
    if (tus == GLFW_KEY_G) sekilleri_goster = !sekilleri_goster;
    if (tus == GLFW_KEY_C) kirpma_acik = !kirpma_acik;
    if (tus == GLFW_KEY_K) frustum_kilitli = !frustum_kilitli;
    if (tus == GLFW_KEY_R) {
        baslangic_konumlari();
        kamera.sifirla();
    }
}

void fare_hareketi(GLFWwindow* pencere, double x, double y) {
    float dx = (float)(x - fare_x);
    float dy = (float)(y - fare_y);
    fare_x = x;
    fare_y = y;
    if (glfwGetMouseButton(pencere, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        kamera.dondur(dx, dy);
    else if (glfwGetMouseButton(pencere, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        kamera.kaydir(dx, dy);
}

void tekerlek_olayi(GLFWwindow*, double, double y) {
    kamera.yaklas((float)y);
}

void boyut_degisti(GLFWwindow*, int en, int boy) {
    if (en == 0 || boy == 0) return;   // pencere simge durumuna kuculdu
    pencere_en = en;
    pencere_boy = boy;
    glViewport(0, 0, en, boy);
}

void ayarlar() {
    glClearColor(0.12f, 0.13f, 0.16f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    float ortam[4] = { 0.35f, 0.35f, 0.35f, 1.0f };
    float yayilan[4] = { 0.85f, 0.85f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ortam);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, yayilan);
}

void tuslari_yazdir() {
    printf("Kamera  : sol fare dondur, sag fare kaydir, tekerlek yaklas\n");
    printf("          W/A/S/D dondur, Z/X yaklas-uzaklas\n");
    printf("Nesne   : 1-5 ya da TAB sec, ok tuslari tasi, PgUp/PgDn yukselt, Q/E dondur\n");
    printf("Diger   : G carpisma sekilleri, C frustum kirpma, K frustumu kilitle\n");
    printf("          R sifirla, ESC cikis\n");
}

int main() {
    if (!glfwInit()) {
        printf("GLFW baslatilamadi\n");
        return 1;
    }
    GLFWwindow* pencere = glfwCreateWindow(pencere_en, pencere_boy, "Carpisma Demo", NULL, NULL);
    if (pencere == NULL) {
        printf("Pencere acilamadi\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(pencere);
    glfwSwapInterval(1);
    if (glewInit() != GLEW_OK) {
        printf("GLEW baslatilamadi\n");
        glfwTerminate();
        return 1;
    }

    glfwSetKeyCallback(pencere, tus_olayi);
    glfwSetCursorPosCallback(pencere, fare_hareketi);
    glfwSetScrollCallback(pencere, tekerlek_olayi);
    glfwSetFramebufferSizeCallback(pencere, boyut_degisti);
    glfwGetFramebufferSize(pencere, &pencere_en, &pencere_boy);
    glViewport(0, 0, pencere_en, pencere_boy);

    ayarlar();
    kamera.sifirla();
    if (!sahneyi_kur()) {
        glfwTerminate();
        return 1;
    }
    tuslari_yazdir();

    double onceki = glfwGetTime();
    while (!glfwWindowShouldClose(pencere)) {
        double simdi = glfwGetTime();
        float dt = (float)(simdi - onceki);
        onceki = simdi;
        // pencere suruklenirken kare cok uzar, nesne bir anda ziplamasin
        if (dt > 0.05f) dt = 0.05f;

        glfwPollEvents();
        hareket_et(pencere, dt);
        kamera_klavye(pencere, dt);
        carpisma.denetle();
        sahne_ciz(pencere);
        glfwSwapBuffers(pencere);
    }

    for (int i = 0; i < NESNE_SAYISI; i++) nesneler[i].model.bosalt();
    glfwTerminate();
    return 0;
}
