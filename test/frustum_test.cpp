#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cmath>

const float PI = 3.14159265f;

#include "../parcalar/kamera.inc"

int toplam = 0;
int hata = 0;

void kontrol(const char* ad, bool ok) {
    toplam++;
    if (!ok) {
        hata++;
        printf("HATA: %s\n", ad);
    } else {
        printf("tamam: %s\n", ad);
    }
}

bool yakin(float a, float b) {
    return fabsf(a - b) < 1e-3f;
}

bool yakin_vec(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(a - b) < 1e-3f;
}

unsigned int tohum = 12345;

float rastgele(float en_az, float en_cok) {
    tohum = tohum * 1664525u + 1013904223u;
    float t = (float)(tohum >> 8) / 16777216.0f;
    return en_az + t * (en_cok - en_az);
}

int main() {
    Camera k;
    k.sifirla();
    k.hedef = glm::vec3(0.0f);
    k.uzaklik = 10.0f;

    k.yatay = 0.0f; k.dikey = 0.0f;
    kontrol("goz yatay=0 dikey=0 -> (0,0,10)", yakin_vec(k.goz(), glm::vec3(0.0f, 0.0f, 10.0f)));
    k.yatay = PI / 2.0f;
    kontrol("goz yatay=90 -> (10,0,0)", yakin_vec(k.goz(), glm::vec3(10.0f, 0.0f, 0.0f)));
    k.yatay = 0.0f; k.dikey = PI / 6.0f;
    kontrol("goz dikey=30 -> (0,5,8.66)", yakin_vec(k.goz(), glm::vec3(0.0f, 5.0f, 8.660254f)));
    k.hedef = glm::vec3(1.0f, 2.0f, 3.0f); k.dikey = 0.0f;
    kontrol("goz hedefe gore oteleniyor", yakin_vec(k.goz(), glm::vec3(1.0f, 2.0f, 13.0f)));
    kontrol("goz hedefe uzaklik kadar uzak", yakin(glm::length(k.goz() - k.hedef), 10.0f));

    k.hedef = glm::vec3(0.0f); k.yatay = 0.0f; k.dikey = 0.0f;
    glm::vec4 p = k.bakis() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    kontrol("bakis: hedef gorus uzayinda (0,0,-10)",
            yakin_vec(glm::vec3(p), glm::vec3(0.0f, 0.0f, -10.0f)));

    k.dondur(100.0f, 0.0f);
    kontrol("dondur: sag fare farki yatayi azaltir", yakin(k.yatay, -0.5f));
    k.dondur(0.0f, 1000000.0f);
    kontrol("dikey ust sinir 85 derece", yakin(k.dikey, 85.0f * PI / 180.0f));
    k.dondur(0.0f, -2000000.0f);
    kontrol("dikey alt sinir -85 derece", yakin(k.dikey, -85.0f * PI / 180.0f));

    k.uzaklik = 10.0f;
    k.yaklas(1000.0f);
    kontrol("zoom alt sinir 1.5", yakin(k.uzaklik, 1.5f));
    k.yaklas(-1000.0f);
    kontrol("zoom ust sinir 60", yakin(k.uzaklik, 60.0f));
    k.uzaklik = 10.0f;
    k.yaklas(1.0f);
    kontrol("zoom tek adim yaklasir", k.uzaklik < 10.0f && k.uzaklik > 8.0f);

    k.sifirla();
    k.hedef = glm::vec3(0.0f); k.yatay = 0.0f; k.dikey = 0.0f; k.uzaklik = 10.0f;
    k.kaydir(100.0f, 0.0f);
    kontrol("kaydir: sag fare farki hedefi -x yonune tasir",
            k.hedef.x < 0.0f && yakin(k.hedef.y, 0.0f));
    float yakin_adim = fabsf(k.hedef.x);
    k.hedef = glm::vec3(0.0f); k.uzaklik = 20.0f;
    k.kaydir(100.0f, 0.0f);
    kontrol("kaydir uzaklikla orantili", yakin(fabsf(k.hedef.x), 2.0f * yakin_adim));

    k.hedef = glm::vec3(0.0f); k.uzaklik = 10.0f;
    glm::mat4 pv = glm::perspective(glm::radians(60.0f), 4.0f / 3.0f, 0.1f, 100.0f) * k.bakis();
    Frustum f;
    f.guncelle(pv);

    kontrol("hedefteki kure gorunur", f.kure_gorunur(glm::vec3(0.0f), 1.0f));
    kontrol("goz arkasindaki kure gorunmez", !f.kure_gorunur(glm::vec3(0.0f, 0.0f, 30.0f), 1.0f));
    kontrol("uzak duzlemin otesi gorunmez", !f.kure_gorunur(glm::vec3(0.0f, 0.0f, -95.0f), 1.0f));
    kontrol("cok solda gorunmez", !f.kure_gorunur(glm::vec3(-30.0f, 0.0f, 0.0f), 1.0f));
    kontrol("cok sagda gorunmez", !f.kure_gorunur(glm::vec3(30.0f, 0.0f, 0.0f), 1.0f));
    kontrol("cok ustte gorunmez", !f.kure_gorunur(glm::vec3(0.0f, 30.0f, 0.0f), 1.0f));
    kontrol("cok altta gorunmez", !f.kure_gorunur(glm::vec3(0.0f, -30.0f, 0.0f), 1.0f));
    // z=0'da yari genislik 10*tan(30)*4/3 = 7.70; merkez 8.5'te, yan duzleme uzaklik yaklasik -0.64
    kontrol("yan duzlemi kesen kure gorunur", f.kure_gorunur(glm::vec3(8.5f, 0.0f, 0.0f), 1.5f));
    kontrol("ayni merkez, kucuk yaricap gorunmez",
            !f.kure_gorunur(glm::vec3(8.5f, 0.0f, 0.0f), 0.3f));

    bool hepsi_birim = true;
    for (int i = 0; i < 6; i++) {
        if (!yakin(glm::length(glm::vec3(f.duzlem[i])), 1.0f)) hepsi_birim = false;
    }
    kontrol("6 duzlem normali birim uzunlukta", hepsi_birim);

    glm::vec3 ic(0.0f, 0.0f, 0.0f);
    bool hepsi_pozitif = true;
    for (int i = 0; i < 6; i++) {
        float mesafe = glm::dot(glm::vec3(f.duzlem[i]), ic) + f.duzlem[i].w;
        if (mesafe <= 0.0f) hepsi_pozitif = false;
    }
    kontrol("ic nokta tum duzlemlere pozitif uzaklikta", hepsi_pozitif);

    // sol duzlem normali +x, sag duzlem normali -x yonune bakmali
    kontrol("sol duzlem normali ice (+x)", f.duzlem[0].x > 0.0f);
    kontrol("sag duzlem normali ice (-x)", f.duzlem[1].x < 0.0f);

    int denenen = 0;
    int uyusmayan = 0;
    for (int i = 0; i < 2000; i++) {
        glm::vec3 nokta(rastgele(-60.0f, 60.0f), rastgele(-60.0f, 60.0f), rastgele(-120.0f, 40.0f));
        bool sinira_yakin = false;
        for (int j = 0; j < 6; j++) {
            float mesafe = glm::dot(glm::vec3(f.duzlem[j]), nokta) + f.duzlem[j].w;
            if (fabsf(mesafe) < 1e-3f) sinira_yakin = true;
        }
        if (sinira_yakin) continue;
        glm::vec4 c = pv * glm::vec4(nokta, 1.0f);
        bool beklenen = -c.w <= c.x && c.x <= c.w && -c.w <= c.y && c.y <= c.w &&
                        -c.w <= c.z && c.z <= c.w;
        denenen++;
        if (f.kure_gorunur(nokta, 0.0f) != beklenen) uyusmayan++;
    }
    printf("rastgele nokta: %d denendi, %d uyusmadi\n", denenen, uyusmayan);
    kontrol("rastgele noktalar clip uzayiyla uyusuyor", denenen > 1500 && uyusmayan == 0);

    printf("%d kontrol, %d hata\n", toplam, hata);
    return hata == 0 ? 0 : 1;
}
