// GL olmadan derlenir: g++ -std=c++11 -DTEST_DERLEME -I<glm> test/carpisma_test.cpp
#include <glm/glm.hpp>
#include <cstdio>
#include <cmath>
#include <vector>

const float PI = 3.14159265f;

#include "../parcalar/carpisma.inc"

const float EPS = 1e-3f;
const float PAY = 1e-2f;   // rastgele karsilastirmada sinira bu kadar yakin durumlar atlanir

int toplam_kontrol = 0;
int hata_sayisi = 0;

void bekle(bool kosul, const char* ad) {
    toplam_kontrol++;
    if (!kosul) {
        hata_sayisi++;
        printf("HATA: %s\n", ad);
    }
}

// her cift iki sirayla da denenir
void cift(const PhysObj& a, const PhysObj& b, bool beklenen, const char* ad) {
    bekle(carpisiyor_mu(a, b) == beklenen, ad);
    bekle(carpisiyor_mu(b, a) == beklenen, ad);
}

PhysObj kure(float x, float y, float z, float r) {
    PhysObj n;
    n.tip = KURE;
    n.konum = glm::vec3(x, y, z);
    n.yaricap = r;
    n.boy = 0.0f;
    n.eksen = glm::vec3(0.0f, 1.0f, 0.0f);
    n.carpisti = false;
    return n;
}

PhysObj silindir(float x, float y, float z, float r, float boy) {
    PhysObj n = kure(x, y, z, r);
    n.tip = SILINDIR;
    n.boy = boy;
    return n;
}

PhysObj kapsul(glm::vec3 merkez, glm::vec3 eksen, float boy, float r) {
    PhysObj n = kure(merkez.x, merkez.y, merkez.z, r);
    n.tip = KAPSUL;
    n.eksen = glm::normalize(eksen);
    n.boy = boy;
    return n;
}

void kure_kure_testleri() {
    cift(kure(0, 0, 0, 1), kure(3, 0, 0, 1), false, "kure-kure ayrik");
    cift(kure(0, 0, 0, 1), kure(1.5f, 0, 0, 1), true, "kure-kure ic ice");
    cift(kure(0, 0, 0, 1), kure(2 - EPS, 0, 0, 1), true, "kure-kure degmeden hemen once");
    cift(kure(0, 0, 0, 1), kure(2 + EPS, 0, 0, 1), false, "kure-kure degmeden hemen sonra");
    cift(kure(0, 0, 0, 2), kure(0.3f, 0, 0, 0.5f), true, "kure-kure icerme");
}

void kure_kapsul_testleri() {
    glm::vec3 x_yon(1, 0, 0);
    PhysObj kap = kapsul(glm::vec3(0, 0, 0), x_yon, 4, 0.5f);   // ic parca x in [-1.5, 1.5]
    cift(kure(0, 2, 0, 1), kap, false, "kure-kapsul yan ayrik");
    cift(kure(0, 1.4f, 0, 1), kap, true, "kure-kapsul yan ic ice");
    cift(kure(2.5f - EPS, 0, 0, 0.5f), kap, true, "kure-kapsul uc degme-eps");
    cift(kure(2.5f + EPS, 0, 0, 0.5f), kap, false, "kure-kapsul uc degme+eps");
    // uctan capraz: uzaklik 0.6*sqrt(2) = 0.8485
    cift(kure(2.1f, 0.6f, 0, 0.3f), kap, false, "kure-kapsul uc capraz ayrik");
    cift(kure(2.1f, 0.6f, 0, 0.4f), kap, true, "kure-kapsul uc capraz carpisma");
    cift(kure(1, 0.1f, 0, 0.1f), kap, true, "kure-kapsul icerme");
    PhysObj nokta_kap = kapsul(glm::vec3(0, 0, 0), x_yon, 1, 0.5f);   // ic parca sifir boylu
    cift(kure(1, 0, 0, 0.49f), nokta_kap, false, "kure-kapsul sifir parca ayrik");
    cift(kure(1, 0, 0, 0.51f), nokta_kap, true, "kure-kapsul sifir parca carpisma");
}

void kapsul_kapsul_testleri() {
    glm::vec3 x_yon(1, 0, 0), z_yon(0, 0, 1);
    PhysObj a = kapsul(glm::vec3(0, 0, 0), x_yon, 4, 0.5f);
    cift(a, kapsul(glm::vec3(0, 1.2f, 0), x_yon, 4, 0.5f), false, "kapsul paralel ayrik");
    cift(a, kapsul(glm::vec3(0, 0.9f, 0), x_yon, 4, 0.5f), true, "kapsul paralel carpisma");
    // paralel ama kaymis: uclar arasi sqrt(0.5^2+0.9^2)=1.0296 ve sqrt(0.3^2+0.9^2)=0.9487
    cift(a, kapsul(glm::vec3(3.5f, 0.9f, 0), x_yon, 4, 0.5f), false, "kapsul paralel kayik ayrik");
    cift(a, kapsul(glm::vec3(3.3f, 0.9f, 0), x_yon, 4, 0.5f), true, "kapsul paralel kayik carp");
    cift(a, kapsul(glm::vec3(0, 1 + EPS, 0), z_yon, 4, 0.5f), false, "kapsul capraz +eps");
    cift(a, kapsul(glm::vec3(0, 1 - EPS, 0), z_yon, 4, 0.5f), true, "kapsul capraz -eps");
    cift(a, kapsul(glm::vec3(2, 0, 0), z_yon, 4, 0.5f), true, "kapsul capraz uca yakin");
    cift(a, kapsul(glm::vec3(2.6f, 0, 0), z_yon, 4, 0.5f), false, "kapsul capraz uctan ayrik");
    cift(a, kapsul(glm::vec3(4.1f, 0, 0), x_yon, 4, 0.5f), false, "kapsul ayni dogru ayrik");
    cift(a, kapsul(glm::vec3(3.9f, 0, 0), x_yon, 4, 0.5f), true, "kapsul ayni dogru carpisma");
    cift(a, kapsul(glm::vec3(1, 0, 0), x_yon, 4, 0.5f), true, "kapsul ayni dogru ust uste");
    // egik: iki eksen orijinde kesisir, z yonunde 1.1 / 0.9 ayrilmis
    PhysObj egik = kapsul(glm::vec3(0, 0, 0), glm::vec3(1, 1, 0), 4, 0.5f);
    glm::vec3 ters_kosegen(1, -1, 0);
    cift(egik, kapsul(glm::vec3(0, 0, 1.1f), ters_kosegen, 4, 0.5f), false, "kapsul egik ayrik");
    cift(egik, kapsul(glm::vec3(0, 0, 0.9f), ters_kosegen, 4, 0.5f), true, "kapsul egik carp");
    // ikisi de sifir boylu: aslinda iki kure
    PhysObj nokta1 = kapsul(glm::vec3(0, 0, 0), x_yon, 1, 0.5f);
    cift(nokta1, kapsul(glm::vec3(0.79f, 0, 0), z_yon, 0.6f, 0.3f), true, "kapsul sifir carp");
    cift(nokta1, kapsul(glm::vec3(0.81f, 0, 0), z_yon, 0.6f, 0.3f), false, "kapsul sifir ayrik");
    cift(kapsul(glm::vec3(0, 0.95f, 0), x_yon, 1, 0.5f), a, true, "kapsul sifir-uzun carp");
    cift(kapsul(glm::vec3(0, 1.05f, 0), x_yon, 1, 0.5f), a, false, "kapsul sifir-uzun ayrik");
    cift(a, kapsul(glm::vec3(0.5f, 0.1f, 0), z_yon, 0.8f, 0.1f), true, "kapsul icerme");
}

void kure_silindir_testleri() {
    PhysObj sil = silindir(0, 0, 0, 1, 2);   // y in [-1, 1]
    cift(kure(1.5f, 0, 0, 0.49f), sil, false, "kure-silindir yan ayrik");
    cift(kure(1.5f, 0, 0, 0.51f), sil, true, "kure-silindir yan carpisma");
    cift(kure(0, 1.5f, 0, 0.5f - EPS), sil, false, "kure-silindir ust -eps");
    cift(kure(0, 1.5f, 0, 0.5f + EPS), sil, true, "kure-silindir ust +eps");
    // kose: en yakin nokta kenar cemberi (1,1,0), uzaklik sqrt(0.09+0.16)=0.5
    cift(kure(1.3f, 1.4f, 0, 0.49f), sil, false, "kure-silindir kenar cemberi ayrik");
    cift(kure(1.3f, 1.4f, 0, 0.51f), sil, true, "kure-silindir kenar cemberi carp");
    // xz'de capraz: radyal sqrt(2) - 1 = 0.4142
    cift(kure(1, 0, 1, 0.40f), sil, false, "kure-silindir xz capraz ayrik");
    cift(kure(1, 0, 1, 0.43f), sil, true, "kure-silindir xz capraz carp");
    cift(kure(0.2f, 0.3f, 0, 0.1f), sil, true, "kure silindirin icinde");
    cift(kure(0, 0, 0, 5), sil, true, "silindir kurenin icinde");
}

void silindir_silindir_testleri() {
    PhysObj sil = silindir(0, 0, 0, 1, 2);
    cift(sil, silindir(2.5f, 0, 0, 1, 2), false, "silindir yan ayrik");
    cift(sil, silindir(1.9f, 0, 0, 1, 2), true, "silindir yan carpisma");
    cift(sil, silindir(0, 2.1f, 0, 1, 2), false, "silindir ust ayrik");
    cift(sil, silindir(0, 1.9f, 0, 1, 2), true, "silindir ust carpisma");
    cift(sil, silindir(1.5f, 1.49f, 0, 0.6f, 1), true, "silindir kose carpisma");
    cift(sil, silindir(1.5f, 1.51f, 0, 0.6f, 1), false, "silindir kose ayrik");
    cift(sil, silindir(2 + EPS, 0, 0, 1, 2), false, "silindir yan +eps");
    cift(sil, silindir(2 - EPS, 0, 0, 1, 2), true, "silindir yan -eps");
    cift(sil, silindir(1.5f, 0, 1.5f, 0.1f, 2), false, "silindir xz capraz ayrik");
    cift(sil, silindir(0.2f, 0.1f, 0, 0.3f, 0.5f), true, "silindir icerme");
}

void kapsul_silindir_testleri() {
    glm::vec3 x_yon(1, 0, 0), y_yon(0, 1, 0), z_yon(0, 0, 1);
    PhysObj sil = silindir(0, 0, 0, 1, 2);
    // yatay kapsul yandan yaklasiyor: ic parca yarisi 0.7, degme merkezi x = 2.0
    cift(kapsul(glm::vec3(2 + EPS, 0, 0), x_yon, 2, 0.3f), sil, false, "kapsul-sil yan +eps");
    cift(kapsul(glm::vec3(2 - EPS, 0, 0), x_yon, 2, 0.3f), sil, true, "kapsul-sil yan -eps");
    // yatay kapsul ustten: kapaga uzaklik y - 1
    cift(kapsul(glm::vec3(0, 1.3f + EPS, 0), z_yon, 2, 0.3f), sil, false, "kapsul-sil ust +eps");
    cift(kapsul(glm::vec3(0, 1.3f - EPS, 0), z_yon, 2, 0.3f), sil, true, "kapsul-sil ust -eps");
    // z yonlu kapsul kenar cemberinin capraz disinda: uzaklik 0.5
    cift(kapsul(glm::vec3(1.3f, 1.4f, 0), z_yon, 2, 0.49f), sil, false, "kapsul-sil kenar ayrik");
    cift(kapsul(glm::vec3(1.3f, 1.4f, 0), z_yon, 2, 0.51f), sil, true, "kapsul-sil kenar carp");
    // xy duzleminde egik kapsul, kenar cemberine (1,1,0) dik yonde 0.5 uzakta
    glm::vec3 kose_disi = glm::vec3(1, 1, 0) + glm::normalize(glm::vec3(1, 1, 0)) * 0.5f;
    glm::vec3 teget(1, -1, 0);
    cift(kapsul(kose_disi, teget, 2, 0.49f), sil, false, "kapsul-sil egik kenar ayrik");
    cift(kapsul(kose_disi, teget, 2, 0.51f), sil, true, "kapsul-sil egik kenar carp");
    // yanda, en yakin nokta parcanin ortasinda
    cift(kapsul(glm::vec3(1.3f, 0, 0), z_yon, 4, 0.29f), sil, false, "kapsul-sil yan orta ayrik");
    cift(kapsul(glm::vec3(1.3f, 0, 0), z_yon, 4, 0.31f), sil, true, "kapsul-sil yan orta carp");
    cift(kapsul(glm::vec3(0, 3, 0), y_yon, 2, 0.3f), sil, false, "kapsul-sil dik ustte ayrik");
    cift(kapsul(glm::vec3(0.5f, 1.95f, 0), y_yon, 2, 0.3f), sil, true, "kapsul-sil dik ustte carp");
    cift(kapsul(glm::vec3(0, 0, 0), x_yon, 10, 0.1f), sil, true, "kapsul silindiri deliyor");
    cift(kapsul(glm::vec3(0, 0.2f, 0), z_yon, 1, 0.2f), sil, true, "kapsul silindirin icinde");
    glm::vec3 kose(1.3f, 1.4f, 0);
    cift(kapsul(kose, x_yon, 0.98f, 0.49f), sil, false, "kapsul-sil sifir ayrik");
    cift(kapsul(kose, x_yon, 1.02f, 0.51f), sil, true, "kapsul-sil sifir carp");
}

void sistem_testleri() {
    PhysObj n1 = kure(0, 0, 0, 1);
    PhysObj n2 = kure(1.5f, 0, 0, 1);
    PhysObj n3 = silindir(10, 0, 0, 1, 2);
    CollisionSystem sistem;
    sistem.nesneler.push_back(&n1);
    sistem.nesneler.push_back(&n2);
    sistem.nesneler.push_back(&n3);
    sistem.denetle();
    bekle(sistem.carpisan_cift == 1, "sistem cift sayisi");
    bekle(n1.carpisti && n2.carpisti && !n3.carpisti, "sistem bayraklar");
    n2.konum.x = 5;
    sistem.denetle();
    bekle(sistem.carpisan_cift == 0 && !n1.carpisti && !n2.carpisti, "sistem bayrak sifirlama");

    bekle(fabsf(sinir_yaricapi(kure(0, 0, 0, 2)) - 2) < 1e-5f, "sinir kure");
    bekle(fabsf(sinir_yaricapi(silindir(0, 0, 0, 3, 8)) - 5) < 1e-5f, "sinir silindir");
    bekle(fabsf(sinir_yaricapi(kapsul(glm::vec3(0), glm::vec3(1, 0, 0), 4, 0.5f)) - 2) < 1e-5f,
          "sinir kapsul");
}

// kaba kuvvet karsilastirmasi icin bagimsiz hesaplar
unsigned int lcg_durum = 12345u;

float rastgele(float en_az, float en_cok) {
    lcg_durum = lcg_durum * 1664525u + 1013904223u;
    return en_az + (en_cok - en_az) * ((lcg_durum >> 8) / 16777216.0f);
}

PhysObj rastgele_sekil() {
    glm::vec3 merkez(rastgele(-1.2f, 1.2f), rastgele(-1.2f, 1.2f), rastgele(-1.2f, 1.2f));
    float r = rastgele(0.1f, 0.8f);
    float boy = rastgele(0.2f, 2.5f);
    int tip = (int)rastgele(0.0f, 3.0f);
    if (tip == 0) return kure(merkez.x, merkez.y, merkez.z, r);
    if (tip == 1) return silindir(merkez.x, merkez.y, merkez.z, r, boy);
    glm::vec3 eksen;
    do {
        eksen = glm::vec3(rastgele(-1, 1), rastgele(-1, 1), rastgele(-1, 1));
        if (rastgele(0, 1) < 0.25f) eksen.y = 0;
    } while (glm::length(eksen) < 0.2f);
    // ic parcasi uzun kapsuller de cikabilsin diye ayri aralik
    return kapsul(merkez, eksen, rastgele(0.2f, 3.5f), rastgele(0.05f, 0.5f));
}

// dik silindire uzaklik, durumlara ayirarak
float ref_silindir_uzaklik(const glm::vec3& p, const PhysObj& sil) {
    float dx = p.x - sil.konum.x, dz = p.z - sil.konum.z;
    float radyal_tasma = sqrtf(dx * dx + dz * dz) - sil.yaricap;
    float dikey_tasma = fabsf(p.y - sil.konum.y) - sil.boy * 0.5f;
    if (radyal_tasma <= 0 && dikey_tasma <= 0) return 0;
    if (radyal_tasma <= 0) return dikey_tasma;
    if (dikey_tasma <= 0) return radyal_tasma;
    return sqrtf(radyal_tasma * radyal_tasma + dikey_tasma * dikey_tasma);
}

// kure ve kapsulun cekirdegi (merkez ya da ic parca) uzerinde sik ornek noktalar
std::vector<glm::vec3> cekirdek_ornekleri(const PhysObj& n) {
    std::vector<glm::vec3> noktalar;
    if (n.tip == KURE) {
        noktalar.push_back(n.konum);
        return noktalar;
    }
    float yarim = glm::max(n.boy * 0.5f - n.yaricap, 0.0f);
    for (int i = 0; i <= 400; i++)
        noktalar.push_back(n.konum + n.eksen * (yarim * (2.0f * i / 400 - 1.0f)));
    return noktalar;
}

// silindirin yuzeyinde sik ornek noktalar (yan yuzey + iki kapak)
std::vector<glm::vec3> silindir_yuzeyi(const PhysObj& sil) {
    std::vector<glm::vec3> noktalar;
    float yarim = sil.boy * 0.5f;
    for (int i = 0; i < 128; i++) {
        float aci = 2.0f * PI * i / 128;
        for (int k = 0; k <= 48; k++) {
            float y = -yarim + sil.boy * k / 48;
            noktalar.push_back(sil.konum + glm::vec3(sil.yaricap * cosf(aci), y,
                                                     sil.yaricap * sinf(aci)));
        }
        for (int k = 0; k <= 24; k++) {
            float r = sil.yaricap * k / 24;
            noktalar.push_back(sil.konum + glm::vec3(r * cosf(aci), yarim, r * sinf(aci)));
            noktalar.push_back(sil.konum + glm::vec3(r * cosf(aci), -yarim, r * sinf(aci)));
        }
    }
    return noktalar;
}

float ref_cekirdek_uzaklik(const glm::vec3& p, const PhysObj& n) {
    if (n.tip == KURE) return glm::length(p - n.konum);
    std::vector<glm::vec3> noktalar = cekirdek_ornekleri(n);
    float en_az = 1e9f;
    for (size_t i = 0; i < noktalar.size(); i++)
        en_az = glm::min(en_az, glm::length(p - noktalar[i]));
    return en_az;
}

bool ref_carpisma(const PhysObj& a, const PhysObj& b) {
    if (a.tip == SILINDIR && b.tip == SILINDIR) {
        // konveks iki cisim kesisiyorsa birinin yuzeyi digerinin icine girer
        std::vector<glm::vec3> yuzey_a = silindir_yuzeyi(a);
        std::vector<glm::vec3> yuzey_b = silindir_yuzeyi(b);
        for (size_t i = 0; i < yuzey_a.size(); i++)
            if (ref_silindir_uzaklik(yuzey_a[i], b) == 0) return true;
        for (size_t i = 0; i < yuzey_b.size(); i++)
            if (ref_silindir_uzaklik(yuzey_b[i], a) == 0) return true;
        return false;
    }
    if (a.tip == SILINDIR) return ref_carpisma(b, a);
    std::vector<glm::vec3> noktalar = cekirdek_ornekleri(a);
    for (size_t i = 0; i < noktalar.size(); i++) {
        if (b.tip == SILINDIR) {
            if (ref_silindir_uzaklik(noktalar[i], b) <= a.yaricap) return true;
        } else if (ref_cekirdek_uzaklik(noktalar[i], b) <= a.yaricap + b.yaricap) {
            return true;
        }
    }
    return false;
}

PhysObj buyut(PhysObj n, float miktar) {
    n.yaricap += miktar;
    if (n.tip != KURE) n.boy += 2 * miktar;
    return n;
}

void rastgele_karsilastirma() {
    int denenen = 0, atlanan = 0, uyusmayan = 0, carpisan = 0;
    int tip_sayisi[3][3] = {{0}};
    for (int i = 0; i < 2000; i++) {
        PhysObj a = rastgele_sekil();
        PhysObj b = rastgele_sekil();
        // sekiller biraz kucultulup buyutulunce sonuc degisiyorsa sinira cok yakin, atla
        bool kucuk = ref_carpisma(buyut(a, -PAY * 0.5f), buyut(b, -PAY * 0.5f));
        bool buyuk = ref_carpisma(buyut(a, PAY * 0.5f), buyut(b, PAY * 0.5f));
        if (kucuk != buyuk) {
            atlanan++;
            continue;
        }
        denenen++;
        if (a.tip <= b.tip) tip_sayisi[a.tip][b.tip]++;
        else tip_sayisi[b.tip][a.tip]++;
        if (buyuk) carpisan++;
        if (carpisiyor_mu(a, b) != buyuk || carpisiyor_mu(b, a) != buyuk) {
            uyusmayan++;
            printf("uyusmazlik: tipler %d %d\n", a.tip, b.tip);
        }
    }
    printf("rastgele: %d cift denendi (%d carpisan), %d sinirda atlandi, %d uyusmazlik\n",
           denenen, carpisan, atlanan, uyusmayan);
    printf("tip ciftleri: kk %d, ks %d, kp %d, ss %d, sp %d, pp %d\n",
           tip_sayisi[0][0], tip_sayisi[0][1], tip_sayisi[0][2],
           tip_sayisi[1][1], tip_sayisi[1][2], tip_sayisi[2][2]);
    bekle(uyusmayan == 0, "rastgele karsilastirma");
}

int main() {
    kure_kure_testleri();
    kure_kapsul_testleri();
    kapsul_kapsul_testleri();
    kure_silindir_testleri();
    silindir_silindir_testleri();
    kapsul_silindir_testleri();
    sistem_testleri();
    rastgele_karsilastirma();
    printf("%d kontrol, %d hata\n", toplam_kontrol, hata_sayisi);
    return hata_sayisi == 0 ? 0 : 1;
}
