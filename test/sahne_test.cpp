// Sahne mantigi testi, OpenGL cagirmadan. Proje kokunden calistirilir (models/ okunur):
//   g++ -std=c++11 -DTEST_DERLEME -DGLEW_NO_GLU <glm/glew/glfw basliklari> -Isrc -Iparcalar
//       test/sahne_test.cpp ufbx.o -lm
#include "ortak.inc"
#include "fbx_model.inc"
#include "carpisma.inc"
#include "kamera.inc"
#include "sahne.inc"

int toplam_kontrol = 0;
int hata_sayisi = 0;

void bekle(bool kosul, const char* ad) {
    toplam_kontrol++;
    if (!kosul) {
        hata_sayisi++;
        printf("HATA: %s\n", ad);
    }
}

bool yakin(float a, float b, float pay) { return fabsf(a - b) <= pay; }
bool yakin3(const glm::vec3& a, const glm::vec3& b, float pay) { return glm::length(a - b) <= pay; }

unsigned int tohum = 12345;
float rastgele(float a, float b) {
    tohum = tohum * 1103515245u + 12345u;
    return a + (b - a) * (float)((tohum >> 8) & 0xFFFF) / 65535.0f;
}

// modelin cizimdeki gibi (glTranslatef + glRotatef) dunyaya tasinmis koseleri
std::vector<glm::vec3> dunya_koseleri(const Nesne& n, const std::vector<float>& veri) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), n.konum);
    m = glm::rotate(m, glm::radians(n.aci), glm::vec3(0.0f, 1.0f, 0.0f));
    std::vector<glm::vec3> sonuc;
    for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT)
        sonuc.push_back(glm::vec3(m * glm::vec4(veri[i], veri[i + 1], veri[i + 2], 1.0f)));
    return sonuc;
}

void sekil_sozlesmesi(std::vector<float>* veri) {
    char ad[160];
    for (int i = 0; i < NESNE_SAYISI; i++) {
        Nesne& n = nesneler[i];
        glm::vec3 b = n.model.kutu_max - n.model.kutu_min;
        glm::vec3 kutu_merkezi = n.konum + (n.model.kutu_min + n.model.kutu_max) * 0.5f;
        snprintf(ad, sizeof(ad), "%s: sekil merkezi model kutusunun merkezinde", n.ad);
        bekle(yakin3(n.sekil.konum, kutu_merkezi, 1e-4f), ad);
        snprintf(ad, sizeof(ad), "%s: yaricap pozitif", n.ad);
        bekle(n.sekil.yaricap > 0.0f, ad);

        if (n.sekil.tip == SILINDIR || (n.sekil.tip == KAPSUL && !n.yatay_kapsul)) {
            snprintf(ad, sizeof(ad), "%s: dik seklin boyu model yuksekligi", n.ad);
            bekle(yakin(n.sekil.boy, b.y, 1e-4f), ad);
        }
        if (n.sekil.tip == SILINDIR) {
            snprintf(ad, sizeof(ad), "%s: silindir yaricapi en az genis kenarin yarisi", n.ad);
            bekle(n.sekil.yaricap >= glm::max(b.x, b.z) * 0.5f - 1e-5f, ad);
            // silindir dik oldugundan modelin butun koseleri icinde kalmali
            std::vector<glm::vec3> k = dunya_koseleri(n, veri[i]);
            bool hepsi_icinde = true;
            for (size_t j = 0; j < k.size(); j++)
                if (nokta_silindir_uzaklik(k[j], n.sekil) > 1e-3f) hepsi_icinde = false;
            snprintf(ad, sizeof(ad), "%s: modelin tum koseleri silindirin icinde", n.ad);
            bekle(hepsi_icinde, ad);
            // ve gereksiz buyuk degil: en az bir kose yuzeye degiyor
            float en_uzak = 0.0f;
            for (size_t j = 0; j < k.size(); j++) {
                glm::vec3 f = k[j] - n.sekil.konum;
                en_uzak = glm::max(en_uzak, glm::length(glm::vec2(f.x, f.z)));
            }
            snprintf(ad, sizeof(ad), "%s: silindir modele siki oturuyor", n.ad);
            bekle(yakin(en_uzak, n.sekil.yaricap, 1e-4f), ad);
        }
        if (n.sekil.tip == KURE) {
            // kure modelin butun koselerini sarmali ve gereksiz buyuk olmamali
            std::vector<glm::vec3> k = dunya_koseleri(n, veri[i]);
            float en_uzak = 0.0f;
            for (size_t j = 0; j < k.size(); j++)
                en_uzak = glm::max(en_uzak, glm::length(k[j] - n.sekil.konum));
            snprintf(ad, sizeof(ad), "%s: modelin tum koseleri kurenin icinde", n.ad);
            bekle(en_uzak <= n.sekil.yaricap + 1e-4f, ad);
            snprintf(ad, sizeof(ad), "%s: kure modele siki oturuyor", n.ad);
            bekle(yakin(en_uzak, n.sekil.yaricap, 1e-4f), ad);
        }
        if (n.sekil.tip == KAPSUL && !n.yatay_kapsul) {
            snprintf(ad, sizeof(ad), "%s: dik kapsul ekseni y", n.ad);
            bekle(yakin3(n.sekil.eksen, glm::vec3(0.0f, 1.0f, 0.0f), 1e-6f), ad);
        }
        if (n.yatay_kapsul) {
            float uzun = glm::max(b.x, b.z);
            snprintf(ad, sizeof(ad), "%s: yatay kapsulun boyu modelin uzun yatay kenari", n.ad);
            bekle(yakin(n.sekil.boy, uzun, 1e-4f), ad);
            bool x_uzun = b.x > b.z;
            snprintf(ad, sizeof(ad), "%s: yatay kapsul ekseni uzun kenar boyunca", n.ad);
            bekle(x_uzun ? n.uzun_eksen.x == 1.0f : n.uzun_eksen.z == 1.0f, ad);
        }
    }
}

// donus: eksen glRotatef ile ayni olmali, modelin eksen boyunca uzunlugu kapsul boyu kalmali
void donus_testi(const std::vector<float>& araba_veri) {
    Nesne& araba = nesneler[0];
    float acilar[] = { 0.0f, 37.0f, 90.0f, 180.0f, 270.0f, -45.0f, 400.0f };
    char ad[160];
    for (int i = 0; i < 7; i++) {
        araba.aci = acilar[i];
        sekli_guncelle(araba);
        glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(acilar[i]), glm::vec3(0, 1, 0));
        glm::vec3 beklenen = glm::vec3(r * glm::vec4(araba.uzun_eksen, 0.0f));
        snprintf(ad, sizeof(ad), "araba %.0f derece: kapsul ekseni glRotatef ile ayni", acilar[i]);
        bekle(yakin3(araba.sekil.eksen, beklenen, 1e-5f), ad);
        snprintf(ad, sizeof(ad), "araba %.0f derece: eksen birim ve yatay", acilar[i]);
        bekle(yakin(glm::length(araba.sekil.eksen), 1.0f, 1e-5f)
              && yakin(araba.sekil.eksen.y, 0.0f, 1e-6f), ad);

        std::vector<glm::vec3> k = dunya_koseleri(araba, araba_veri);
        float en_az = 1e9f, en_cok = -1e9f;
        for (size_t j = 0; j < k.size(); j++) {
            float t = glm::dot(k[j] - araba.sekil.konum, araba.sekil.eksen);
            en_az = glm::min(en_az, t);
            en_cok = glm::max(en_cok, t);
        }
        snprintf(ad, sizeof(ad), "araba %.0f derece: model eksen boyunca kapsul kadar uzun",
                 acilar[i]);
        bekle(yakin(en_cok - en_az, araba.sekil.boy, 1e-3f) && yakin(en_cok, -en_az, 1e-3f), ad);
    }
    araba.aci = 0.0f;
    sekli_guncelle(araba);
}

// kirpma kuresi nesneyi her konum ve acida tamamen kapsamali; yoksa gorunen nesne kirpilir
void kirpma_kuresi_testi(std::vector<float>* veri) {
    char ad[160];
    for (int i = 0; i < NESNE_SAYISI; i++) {
        Nesne n = nesneler[i];
        int disarida = 0;
        for (int d = 0; d < 60; d++) {
            n.konum = glm::vec3(rastgele(-8, 8), rastgele(0, 3), rastgele(-8, 8));
            n.aci = rastgele(-360, 360);
            sekli_guncelle(n);
            float r = kirpma_yaricapi(n);
            std::vector<glm::vec3> k = dunya_koseleri(n, veri[i]);
            for (size_t j = 0; j < k.size(); j++)
                if (glm::length(k[j] - n.sekil.konum) > r + 1e-4f) disarida++;
            // carpisma seklinin yuzeyinden ornekler de kurenin icinde olmali
            for (int s = 0; s < 200; s++) {
                glm::vec3 yon = glm::normalize(glm::vec3(rastgele(-1, 1), rastgele(-1, 1),
                                                         rastgele(-1, 1)) + glm::vec3(1e-3f));
                glm::vec3 p;
                if (n.sekil.tip == KURE) {
                    p = n.sekil.konum + yon * n.sekil.yaricap;
                } else if (n.sekil.tip == SILINDIR) {
                    glm::vec2 xz = glm::normalize(glm::vec2(yon.x, yon.z) + glm::vec2(1e-3f));
                    float r = n.sekil.yaricap;
                    p = n.sekil.konum + glm::vec3(xz.x * r, yon.y * n.sekil.boy * 0.5f, xz.y * r);
                } else {
                    glm::vec3 alt, ust;
                    kapsul_uclari(n.sekil, alt, ust);
                    p = (yon.y > 0 ? ust : alt) + yon * n.sekil.yaricap;
                }
                if (glm::length(p - n.sekil.konum) > r + 1e-4f) disarida++;
            }
        }
        snprintf(ad, sizeof(ad), "%s: 60 rastgele konum/acida model ve sekil kirpma kuresinde",
                 n.ad);
        bekle(disarida == 0, ad);
    }
}

// carpisma koduna dokunmadan, yalnizca "nokta seklin icinde mi" tanimiyla yazilan ikinci yol
bool sekil_icinde(const PhysObj& n, const glm::vec3& p) {
    glm::vec3 f = p - n.konum;
    if (n.tip == KURE) return glm::dot(f, f) <= n.yaricap * n.yaricap;
    if (n.tip == SILINDIR)
        return f.x * f.x + f.z * f.z <= n.yaricap * n.yaricap && fabsf(f.y) <= n.boy * 0.5f;
    float yarim = glm::max(0.0f, n.boy * 0.5f - n.yaricap);
    float t = glm::clamp(glm::dot(f, n.eksen), -yarim, yarim);
    glm::vec3 d = f - n.eksen * t;
    return glm::dot(d, d) <= n.yaricap * n.yaricap;
}

void sekil_kutusu(const PhysObj& n, glm::vec3& kmin, glm::vec3& kmax) {
    glm::vec3 yari(n.yaricap);
    if (n.tip == SILINDIR) yari.y = n.boy * 0.5f;
    if (n.tip == KAPSUL) yari += glm::abs(n.eksen) * glm::max(0.0f, n.boy * 0.5f - n.yaricap);
    kmin = n.konum - yari;
    kmax = n.konum + yari;
}

// iki seklin kutularinin kesisimini sik bir izgarayla tarar: ikisinin de icinde bir nokta var mi
bool izgara_ile_carpisiyor(const PhysObj& a, const PhysObj& b, float aralik) {
    glm::vec3 amin, amax, bmin, bmax;
    sekil_kutusu(a, amin, amax);
    sekil_kutusu(b, bmin, bmax);
    glm::vec3 kmin = glm::max(amin, bmin), kmax = glm::min(amax, bmax);
    if (kmin.x > kmax.x || kmin.y > kmax.y || kmin.z > kmax.z) return false;
    for (float x = kmin.x; x <= kmax.x; x += aralik)
        for (float y = kmin.y; y <= kmax.y; y += aralik)
            for (float z = kmin.z; z <= kmax.z; z += aralik) {
                glm::vec3 p(x, y, z);
                if (sekil_icinde(a, p) && sekil_icinde(b, p)) return true;
            }
    return false;
}

// j nesnesi i'ye dogru yurutulur; carpismanin basladigi oran ikili aramayla bulunur
float baslangic_orani(int i, int j, bool izgara) {
    glm::vec3 hedef = nesneler[i].konum, bas = nesneler[j].konum;
    float alt = 0.0f, ust = 1.0f;
    for (int k = 0; k < 18; k++) {
        float orta = (alt + ust) * 0.5f;
        nesneler[j].konum = bas + (hedef - bas) * orta;
        sekli_guncelle(nesneler[j]);
        bool carpti;
        if (izgara) {
            carpti = izgara_ile_carpisiyor(nesneler[i].sekil, nesneler[j].sekil, 0.01f);
        } else {
            carpisma.denetle();
            carpti = nesneler[i].sekil.carpisti && nesneler[j].sekil.carpisti
                && carpisma.carpisan_cift == 1;
        }
        if (carpti) ust = orta;
        else alt = orta;
    }
    nesneler[j].konum = bas;
    sekli_guncelle(nesneler[j]);
    return ust;
}

void carpisma_testi() {
    baslangic_konumlari();
    carpisma.denetle();
    bekle(carpisma.carpisan_cift == 0, "baslangicta hicbir cift carpismiyor");
    bool bayrak_yok = true;
    for (int i = 0; i < NESNE_SAYISI; i++)
        if (nesneler[i].sekil.carpisti) bayrak_yok = false;
    bekle(bayrak_yok, "baslangicta hicbir nesne kirmizi degil");

    // her cift: carpismanin basladigi mesafe, izgarayla bulunanla ayni olmali
    bool tip_goruldu[3][3] = {};
    char ad[200];
    for (int i = 0; i < NESNE_SAYISI; i++) {
        for (int j = i + 1; j < NESNE_SAYISI; j++) {
            baslangic_konumlari();
            float yol = glm::length(nesneler[i].konum - nesneler[j].konum);
            float sistem = baslangic_orani(i, j, false);
            float izgara = baslangic_orani(i, j, true);
            // izgara noktasi ancak ortusme bir aralik kadar derinlesince yakalanir
            float fark = (izgara - sistem) * yol;
            snprintf(ad, sizeof(ad), "%s - %s: carpisma ayni mesafede basliyor (fark %.4f)",
                     nesneler[i].ad, nesneler[j].ad, fark);
            bekle(fark >= -1e-3f && fark <= 0.03f, ad);
            int a = nesneler[i].sekil.tip, b = nesneler[j].sekil.tip;
            tip_goruldu[glm::min(a, b)][glm::max(a, b)] = true;
        }
    }
    int cesit = 0;
    for (int a = 0; a < 3; a++)
        for (int b = a; b < 3; b++) cesit += tip_goruldu[a][b];
    bekle(cesit == 5, "sahnede 5 farkli sekil cifti turu canli olarak sinaniyor");
    baslangic_konumlari();
    carpisma.denetle();
}

// klavye girdisinin nesneye etkisi (hareket_et yalnizca tuslari okuyup bunu cagirir)
void tasima_testi() {
    char ad[160];
    Camera k;
    k.sifirla();
    float acilar[] = { 0.6f, 2.5f, -1.2f };
    for (int i = 0; i < 3; i++) {
        k.yatay = acilar[i];
        Nesne n = nesneler[3];
        n.konum = glm::vec3(0.0f);
        sekli_guncelle(n);
        glm::vec3 goz_zemin(k.goz().x, 0.0f, k.goz().z);
        float once = glm::length(n.konum - goz_zemin);
        nesneyi_tasi(n, k.yatay, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f);
        snprintf(ad, sizeof(ad), "kamera %.1f rad: yukari oku nesneyi kameradan uzaklastirir",
                 acilar[i]);
        bekle(glm::length(n.konum - goz_zemin) > once + 1.4f, ad);
    }
    Nesne n = nesneler[3];
    n.konum = glm::vec3(0.0f, 1.0f, 0.0f);
    nesneyi_tasi(n, 0.6f, 1.0f, 1.0f, 1.0f, 0.0f, 0.1f);
    bekle(yakin(glm::length(n.konum - glm::vec3(0.0f, 1.0f, 0.0f)), HAREKET_HIZI * 0.1f, 1e-5f),
          "capraz ve yukari ayni anda da hiz sabit");
    n.konum = glm::vec3(0.0f, 0.1f, 0.0f);
    nesneyi_tasi(n, 0.6f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    bekle(n.konum.y == 0.0f, "nesne zeminin altina inmiyor");
    bekle(yakin3(n.sekil.konum, n.konum + glm::vec3(0.0f, n.model.kutu_max.y * 0.5f, 0.0f),
                 1e-5f), "tasinan nesnenin sekli de tasiniyor");
    Nesne araba = nesneler[0];
    nesneyi_tasi(araba, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    bekle(yakin(araba.aci, DONME_HIZI, 1e-4f), "Q bir saniyede DONME_HIZI derece dondurur");
    glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(DONME_HIZI), glm::vec3(0, 1, 0));
    bekle(yakin3(araba.sekil.eksen, glm::vec3(r * glm::vec4(araba.uzun_eksen, 0.0f)), 1e-5f),
          "donen arabanin kapsulu de donuyor");
}

// kirpma kuresi, sekil modelden buyuk olsa da tel kafesi kapsamali
void kirpma_yaricapi_testi() {
    Nesne n = nesneler[3];
    float model_r = glm::length(n.model.kutu_max - n.model.kutu_min) * 0.5f;
    n.sekil.yaricap = model_r * 2.0f;
    bekle(yakin(kirpma_yaricapi(n), sinir_yaricapi(n.sekil), 1e-6f),
          "sekil modelden buyukse kirpma kuresi sekli kapsar");
    n = nesneler[3];
    bekle(yakin(kirpma_yaricapi(n), model_r, 1e-6f), "normalde model kutusu belirler");
}

void tasima_yonu_testi() {
    char ad[160];
    float acilar[] = { 0.0f, 0.6f, 1.5708f, 3.1416f, -2.0f, 5.0f };
    Camera k;
    k.sifirla();
    for (int i = 0; i < 6; i++) {
        k.yatay = acilar[i];
        glm::vec3 bakis = k.hedef - k.goz();
        glm::vec3 ileri = glm::normalize(glm::vec3(bakis.x, 0.0f, bakis.z));
        glm::vec3 sag = glm::normalize(glm::cross(ileri, glm::vec3(0.0f, 1.0f, 0.0f)));
        snprintf(ad, sizeof(ad), "kamera %.2f rad: yukari oku kameradan uzaklastirir", acilar[i]);
        bekle(yakin3(tasima_yonu(k.yatay, 1.0f, 0.0f), ileri, 1e-4f), ad);
        snprintf(ad, sizeof(ad), "kamera %.2f rad: sag ok ekranin sagina goturur", acilar[i]);
        bekle(yakin3(tasima_yonu(k.yatay, 0.0f, 1.0f), sag, 1e-4f), ad);
        snprintf(ad, sizeof(ad), "kamera %.2f rad: asagi oku yukarinin tersi", acilar[i]);
        bekle(yakin3(tasima_yonu(k.yatay, -1.0f, 0.0f), -ileri, 1e-4f), ad);
        snprintf(ad, sizeof(ad), "kamera %.2f rad: capraz hareket hizlanmaz", acilar[i]);
        bekle(yakin(glm::length(tasima_yonu(k.yatay, 1.0f, 1.0f)), 1.0f, 1e-5f), ad);
    }
    bekle(glm::length(tasima_yonu(0.6f, 0.0f, 0.0f)) == 0.0f, "tus yokken hareket yok");
    bekle(yakin3(tasima_yonu(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 1e-6f),
          "kamera +z'de iken yukari oku -z");
}

void frustum_testi() {
    Camera k;
    k.sifirla();
    pencere_en = 640;
    pencere_boy = 480;
    glm::mat4 p43 = projeksiyon_matrisi();
    bekle(yakin(p43[1][1] / p43[0][0], 640.0f / 480.0f, 1e-5f),
          "projeksiyon pencerenin en-boy oranini kullaniyor");
    pencere_en = 1000;
    pencere_boy = 700;
    glm::mat4 p = projeksiyon_matrisi();
    Frustum f;
    f.guncelle(p * k.bakis());

    bool hepsi = true;
    for (int i = 0; i < NESNE_SAYISI; i++)
        if (!f.kure_gorunur(nesneler[i].sekil.konum, kirpma_yaricapi(nesneler[i]))) hepsi = false;
    bekle(hepsi, "acilis kamerasi 5 nesnenin hepsini goruyor");

    k.uzaklik = MIN_UZAKLIK;
    f.guncelle(p * k.bakis());
    int gorunen = 0;
    for (int i = 0; i < NESNE_SAYISI; i++)
        gorunen += f.kure_gorunur(nesneler[i].sekil.konum, kirpma_yaricapi(nesneler[i]));
    bekle(gorunen < NESNE_SAYISI, "en yakin zoomda en az bir nesne kirpiliyor");

    // cizilen koseler duzlemlerle tutarli: her kose kendi 3 duzleminde, digerlerinin icinde
    k.sifirla();
    f.guncelle(p * k.bakis());
    bekle(f.pv == p * k.bakis(), "frustum cikarildigi matrisi sakliyor");
    glm::vec3 kose[8];
    frustum_koseleri(f.pv, kose);
    bool tutarli = true;
    for (int i = 0; i < 8; i++) {
        int uzerinde[3] = { (i & 1) ? 1 : 0, (i & 2) ? 3 : 2, (i & 4) ? 5 : 4 };
        for (int d = 0; d < 6; d++) {
            float m = glm::dot(glm::vec3(f.duzlem[d]), kose[i]) + f.duzlem[d].w;
            bool kendi = d == uzerinde[0] || d == uzerinde[1] || d == uzerinde[2];
            // uzak duzlem 80 birimde, float hassasiyeti icin goreli pay
            float pay = 1e-4f * glm::max(1.0f, glm::length(kose[i]));
            if (kendi ? fabsf(m) > pay : m < -pay) tutarli = false;
        }
    }
    bekle(tutarli, "frustum koseleri duzlem denklemleriyle tutarli");
    // yakin duzlem 0.1, uzak duzlem 80 birimde: koseler gozden bu uzakliklarin biraz otesinde
    bool yakinlar = true, uzaklar = true;
    for (int i = 0; i < 8; i++) {
        float u = glm::length(kose[i] - k.goz());
        if (!(i & 4) && (u < 0.1f || u > 0.2f)) yakinlar = false;
        if ((i & 4) && (u < 80.0f || u > 160.0f)) uzaklar = false;
    }
    bekle(yakinlar, "yakin duzlemin 4 kosesi kameranin hemen onunde");
    bekle(uzaklar, "uzak duzlemin 4 kosesi 80 birim otede");
}

int main() {
    if (!sahneyi_kur()) {
        printf("models/ okunamadi; testi proje kokunden calistirin\n");
        return 1;
    }
    // sahneyi_kur ile ayni dosya ve boylar; koseler kapsama testleri icin yeniden okunur
    const char* dosya[NESNE_SAYISI] = { "models/araba.fbx", "models/insan.fbx", "models/kuyu.fbx",
                                        "models/bomba.fbx", "models/mantar.fbx" };
    float boy[NESNE_SAYISI] = { 3.2f, 1.8f, 2.2f, 0.9f, 1.2f };
    std::vector<float> veri[NESNE_SAYISI];
    for (int i = 0; i < NESNE_SAYISI; i++) {
        glm::vec3 a, b, r;
        fbx_oku(dosya[i], boy[i], veri[i], a, b, r);
        bekle(yakin3(a, nesneler[i].model.kutu_min, 1e-6f), "test sahneyle ayni modeli okudu");
    }
    sekil_sozlesmesi(veri);
    donus_testi(veri[0]);
    kirpma_kuresi_testi(veri);
    carpisma_testi();
    tasima_yonu_testi();
    tasima_testi();
    kirpma_yaricapi_testi();
    frustum_testi();
    printf("%d kontrol, %d hata\n", toplam_kontrol, hata_sayisi);
    return hata_sayisi == 0 ? 0 : 1;
}
