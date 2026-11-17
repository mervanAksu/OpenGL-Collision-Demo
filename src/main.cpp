// Bilgisayar Grafigi - Odev 1
// 5 FBX modelli sahne, kure/silindir/kapsul ile carpisma denetimi, orbit kamera, frustum kirpma
//
// Kamera: sol fare dondur, sag fare kaydir, tekerlek yaklas (klavye: W/A/S/D, Z/X)
// 1-5 / TAB: nesne sec, ok tuslari: kameraya gore tasi, PgUp/PgDn: yukselt/alcalt, Q/E: dondur
// G: carpisma sekilleri, C: kirpma ac/kapa, K: frustumu kilitle (sari cizilir), R: sifirla

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cmath>
#include <vector>
#include "ufbx.h"

const float PI = 3.14159265f;
const int KOSE_FLOAT = 9;   // konum(3) + normal(3) + renk(3)
const int UCLU_ARAMA_ADIM = 30;
const int DILIM = 24;   // cemberdeki kose sayisi
const float CIZIM_PAYI = 1.0f / cosf(PI / DILIM);
const float DONDURME_HASSASIYET = 0.005f;
const float KAYDIRMA_HASSASIYET = 0.0015f;
const float MIN_UZAKLIK = 1.5f;
const float MAX_UZAKLIK = 60.0f;
const float MAX_DIKEY = 85.0f * PI / 180.0f;
const int NESNE_SAYISI = 5;
const float HAREKET_HIZI = 3.0f;    // birim/saniye
const float DONME_HIZI = 90.0f;     // derece/saniye
const float KAMERA_TUS_HIZI = 250.0f;   // klavyeyle donuste saniyedeki "piksel" karsiligi

glm::vec3 materyal_rengi(const ufbx_material* m) {
    if (m == NULL) return glm::vec3(0.8f, 0.8f, 0.8f);
    ufbx_vec3 c;
    if (m->pbr.base_color.has_value) c = m->pbr.base_color.value_vec3;
    else if (m->fbx.diffuse_color.has_value) c = m->fbx.diffuse_color.value_vec3;
    else return glm::vec3(0.8f, 0.8f, 0.8f);
    return glm::vec3((float)c.x, (float)c.y, (float)c.z);
}

// GL kullanmadan dosyayi okur, ucgenleri tek diziye toplar ve olcekler
bool fbx_oku(const char* yol, float hedef_boy, std::vector<float>& veri,
             glm::vec3& kmin, glm::vec3& kmax, glm::vec3& renk) {
    ufbx_load_opts ayar = {};
    ayar.target_axes = ufbx_axes_right_handed_y_up;
    ayar.target_unit_meters = 1.0f;
    ayar.generate_missing_normals = true;
    ufbx_error hata;
    ufbx_scene* sahne = ufbx_load_file(yol, &ayar, &hata);
    if (sahne == NULL) {
        printf("FBX acilamadi: %s (%s)\n", yol, hata.description.data);
        return false;
    }

    renk = glm::vec3(0.8f, 0.8f, 0.8f);
    if (sahne->materials.count > 0) renk = materyal_rengi(sahne->materials.data[0]);

    veri.clear();
    std::vector<uint32_t> ucgen;
    for (size_t i = 0; i < sahne->meshes.count; i++) {
        ufbx_mesh* mesh = sahne->meshes.data[i];
        ucgen.resize(mesh->max_face_triangles * 3);
        // ayni mesh birden fazla node'da kullanilmis olabilir
        for (size_t j = 0; j < mesh->instances.count; j++) {
            ufbx_node* node = mesh->instances.data[j];
            ufbx_matrix normal_mat = ufbx_matrix_for_normals(&node->geometry_to_world);
            for (size_t f = 0; f < mesh->num_faces; f++) {
                glm::vec3 yuz_rengi = renk;
                if (mesh->face_material.count > 0) {
                    uint32_t mi = mesh->face_material.data[f];
                    if (mi < node->materials.count)
                        yuz_rengi = materyal_rengi(node->materials.data[mi]);
                }
                uint32_t n = ufbx_triangulate_face(ucgen.data(), ucgen.size(), mesh,
                                                   mesh->faces.data[f]);
                for (uint32_t k = 0; k < n * 3; k++) {
                    ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, ucgen[k]);
                    ufbx_vec3 nr = ufbx_get_vertex_vec3(&mesh->vertex_normal, ucgen[k]);
                    p = ufbx_transform_position(&node->geometry_to_world, p);
                    nr = ufbx_transform_direction(&normal_mat, nr);
                    glm::vec3 nn((float)nr.x, (float)nr.y, (float)nr.z);
                    if (glm::length(nn) > 0.0f) nn = glm::normalize(nn);
                    float kose[KOSE_FLOAT] = { (float)p.x, (float)p.y, (float)p.z, nn.x, nn.y, nn.z,
                                               yuz_rengi.r, yuz_rengi.g, yuz_rengi.b };
                    veri.insert(veri.end(), kose, kose + KOSE_FLOAT);
                }
            }
        }
    }
    ufbx_free_scene(sahne);
    if (veri.empty()) {
        printf("FBX icinde mesh yok: %s\n", yol);
        return false;
    }

    kmin = kmax = glm::vec3(veri[0], veri[1], veri[2]);
    for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT) {
        glm::vec3 p(veri[i], veri[i + 1], veri[i + 2]);
        kmin = glm::min(kmin, p);
        kmax = glm::max(kmax, p);
    }
    glm::vec3 boyut = kmax - kmin;
    float en_uzun = glm::max(boyut.x, glm::max(boyut.y, boyut.z));
    float olcek = en_uzun > 0.0f ? hedef_boy / en_uzun : 1.0f;
    // taban y=0'a, x ve z merkeze gelsin
    glm::vec3 taban((kmin.x + kmax.x) * 0.5f, kmin.y, (kmin.z + kmax.z) * 0.5f);
    for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT) {
        for (int e = 0; e < 3; e++) veri[i + e] = (veri[i + e] - taban[e]) * olcek;
    }
    kmin = (kmin - taban) * olcek;
    kmax = (kmax - taban) * olcek;
    return true;
}

struct FbxModel {
    GLuint vbo;              // pos(3)+normal(3)+renk(3) float, ucgen listesi, index yok
    int kose_sayisi;
    glm::vec3 renk;          // ilk materyalin rengi, yoksa acik gri
    glm::vec3 kutu_min, kutu_max;
    float yatay_yaricap;     // y eksenine en uzak kosenin uzakligi; dik silindir bunu sarar

    bool yukle(const char* yol, float hedef_boy) {
        vbo = 0;
        kose_sayisi = 0;
        yatay_yaricap = 0.0f;
        std::vector<float> veri;
        if (!fbx_oku(yol, hedef_boy, veri, kutu_min, kutu_max, renk)) return false;
        kose_sayisi = (int)(veri.size() / KOSE_FLOAT);
        // cokgen kesitli modellerde (varil gibi) koseler kutunun yari genisliginden disari tasar
        for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT)
            yatay_yaricap = glm::max(yatay_yaricap, glm::length(glm::vec2(veri[i], veri[i + 2])));
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, veri.size() * sizeof(float), veri.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return true;
    }

    void ciz() const {
        if (vbo == 0) return;
        // cagiranin cizgi cizimi bozulmasin diye durumu saklayip geri aliyoruz
        glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);

        GLsizei adim = (GLsizei)(KOSE_FLOAT * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, adim, (void*)0);
        glNormalPointer(GL_FLOAT, adim, (void*)(3 * sizeof(float)));
        glColorPointer(3, GL_FLOAT, adim, (void*)(6 * sizeof(float)));
        glDrawArrays(GL_TRIANGLES, 0, kose_sayisi);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glPopClientAttrib();
        glPopAttrib();
    }

    void bosalt() {
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        vbo = 0;
        kose_sayisi = 0;
    }
};

enum SekilTipi { KURE, SILINDIR, KAPSUL };

struct PhysObj {
    SekilTipi tip;
    glm::vec3 konum;     // seklin merkezi
    float yaricap;
    float boy;           // silindirde toplam yukseklik, kapsulde uctan uca uzunluk
    glm::vec3 eksen;     // kapsulun birim ekseni
    bool carpisti;
};

// kapsulun icindeki dogru parcasinin iki ucu, uclar yarim kure oldugu icin boydan yaricap duser
void kapsul_uclari(const PhysObj& n, glm::vec3& alt, glm::vec3& ust) {
    float yarim = n.boy * 0.5f - n.yaricap;
    if (yarim < 0.0f) yarim = 0.0f;
    alt = n.konum - n.eksen * yarim;
    ust = n.konum + n.eksen * yarim;
}

glm::vec3 parcada_en_yakin(const glm::vec3& p, const glm::vec3& bas, const glm::vec3& son) {
    glm::vec3 yon = son - bas;
    float uzunluk2 = glm::dot(yon, yon);
    if (uzunluk2 < 1e-12f) return bas;
    float oran = glm::clamp(glm::dot(p - bas, yon) / uzunluk2, 0.0f, 1.0f);
    return bas + yon * oran;
}

float nokta_parca_uzaklik2(const glm::vec3& p, const glm::vec3& bas, const glm::vec3& son) {
    glm::vec3 fark = p - parcada_en_yakin(p, bas, son);
    return glm::dot(fark, fark);
}

// iki dogru parcasi arasindaki en kisa uzakligin karesi
float parca_parca_uzaklik2(const glm::vec3& bas1, const glm::vec3& son1,
                           const glm::vec3& bas2, const glm::vec3& son2) {
    // en kisa uzaklik ya bir parcanin ucundadir ya da iki dogrunun ortak dikmesindedir
    float en_kucuk = nokta_parca_uzaklik2(bas1, bas2, son2);
    en_kucuk = glm::min(en_kucuk, nokta_parca_uzaklik2(son1, bas2, son2));
    en_kucuk = glm::min(en_kucuk, nokta_parca_uzaklik2(bas2, bas1, son1));
    en_kucuk = glm::min(en_kucuk, nokta_parca_uzaklik2(son2, bas1, son1));

    glm::vec3 yon1 = son1 - bas1;
    glm::vec3 yon2 = son2 - bas2;
    glm::vec3 arada = bas1 - bas2;
    float boy1 = glm::dot(yon1, yon1);
    float boy2 = glm::dot(yon2, yon2);
    float ortak = glm::dot(yon1, yon2);
    float payda = boy1 * boy2 - ortak * ortak;
    // paralel ya da sifir boylu parcada ortak dikme yok, uclar yeterli
    if (payda <= 1e-6f * boy1 * boy2) return en_kucuk;

    // |arada + yon1*oran1 - yon2*oran2|^2 nin turevleri sifira esitlenip cozuldu
    float izdusum1 = glm::dot(yon1, arada);
    float izdusum2 = glm::dot(yon2, arada);
    float oran1 = (ortak * izdusum2 - boy2 * izdusum1) / payda;
    float oran2 = (boy1 * izdusum2 - ortak * izdusum1) / payda;
    if (oran1 >= 0.0f && oran1 <= 1.0f && oran2 >= 0.0f && oran2 <= 1.0f) {
        glm::vec3 fark = (bas1 + yon1 * oran1) - (bas2 + yon2 * oran2);
        en_kucuk = glm::min(en_kucuk, glm::dot(fark, fark));
    }
    return en_kucuk;
}

// noktadan dik silindirin dolu govdesine uzaklik, icerideyse 0
float nokta_silindir_uzaklik(const glm::vec3& p, const PhysObj& sil) {
    glm::vec3 fark = p - sil.konum;
    float yarim_boy = sil.boy * 0.5f;
    glm::vec2 yatay(fark.x, fark.z);
    float radyal = glm::length(yatay);
    if (radyal > sil.yaricap) yatay *= sil.yaricap / radyal;
    glm::vec3 en_yakin(yatay.x, glm::clamp(fark.y, -yarim_boy, yarim_boy), yatay.y);
    return glm::length(fark - en_yakin);
}

// silindir konveks oldugu icin parca boyunca uzaklik konveks bir fonksiyon, uclu arama yeter
bool kapsul_silindir_carpisma(const PhysObj& kapsul, const PhysObj& sil) {
    glm::vec3 alt, ust;
    kapsul_uclari(kapsul, alt, ust);
    glm::vec3 yon = ust - alt;
    float sol = 0.0f, sag = 1.0f;
    for (int i = 0; i < UCLU_ARAMA_ADIM; i++) {
        float ara1 = sol + (sag - sol) / 3.0f;
        float ara2 = sag - (sag - sol) / 3.0f;
        float uzak1 = nokta_silindir_uzaklik(alt + yon * ara1, sil);
        float uzak2 = nokta_silindir_uzaklik(alt + yon * ara2, sil);
        if (uzak1 < uzak2) sag = ara2;
        else sol = ara1;
    }
    float en_az = nokta_silindir_uzaklik(alt + yon * ((sol + sag) * 0.5f), sil);
    return en_az <= kapsul.yaricap;
}

bool carpisiyor_mu(const PhysObj& a, const PhysObj& b) {
    // sira onemsiz: kucuk tip hep ilk gelsin
    if (a.tip > b.tip) return carpisiyor_mu(b, a);

    float toplam = a.yaricap + b.yaricap;
    if (a.tip == KURE && b.tip == KURE) {
        glm::vec3 fark = b.konum - a.konum;
        return glm::dot(fark, fark) <= toplam * toplam;
    }
    if (a.tip == KURE && b.tip == SILINDIR)
        return nokta_silindir_uzaklik(a.konum, b) <= a.yaricap;
    if (a.tip == KURE && b.tip == KAPSUL) {
        glm::vec3 alt, ust;
        kapsul_uclari(b, alt, ust);
        return nokta_parca_uzaklik2(a.konum, alt, ust) <= toplam * toplam;
    }
    if (a.tip == SILINDIR && b.tip == SILINDIR) {
        // dik silindir = daire x aralik, iki izdusum de ortusmeli
        float dikey = fabsf(b.konum.y - a.konum.y);
        float dx = b.konum.x - a.konum.x;
        float dz = b.konum.z - a.konum.z;
        return dikey <= (a.boy + b.boy) * 0.5f && dx * dx + dz * dz <= toplam * toplam;
    }
    if (a.tip == SILINDIR && b.tip == KAPSUL)
        return kapsul_silindir_carpisma(b, a);

    glm::vec3 alt1, ust1, alt2, ust2;
    kapsul_uclari(a, alt1, ust1);
    kapsul_uclari(b, alt2, ust2);
    return parca_parca_uzaklik2(alt1, ust1, alt2, ust2) <= toplam * toplam;
}

float sinir_yaricapi(const PhysObj& n) {
    if (n.tip == SILINDIR) {
        float yarim_boy = n.boy * 0.5f;
        return sqrtf(n.yaricap * n.yaricap + yarim_boy * yarim_boy);
    }
    if (n.tip == KAPSUL) return glm::max(n.boy * 0.5f, n.yaricap);
    return n.yaricap;
}

struct CollisionSystem {
    std::vector<PhysObj*> nesneler;
    int carpisan_cift;

    CollisionSystem() : carpisan_cift(0) {}

    void denetle() {
        int adet = (int)nesneler.size();
        carpisan_cift = 0;
        for (int i = 0; i < adet; i++) nesneler[i]->carpisti = false;
        for (int i = 0; i < adet; i++) {
            for (int j = i + 1; j < adet; j++) {
                if (carpisiyor_mu(*nesneler[i], *nesneler[j])) {
                    nesneler[i]->carpisti = true;
                    nesneler[j]->carpisti = true;
                    carpisan_cift++;
                }
            }
        }
    }
};

// cokgenin kenarlari cemberin icinden gecer; koseler bu kadar disariya konunca
// cizilen tel kafes gercek seklin tamamen disinda kalir

// merkez etrafinda u ve v birim vektorlerinin duzleminde cember
void cember_ciz(const glm::vec3& merkez, const glm::vec3& u, const glm::vec3& v, float r) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < DILIM; i++) {
        float aci = 2.0f * PI * i / DILIM;
        glm::vec3 p = merkez + (u * cosf(aci) + v * sinf(aci)) * r;
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
}

// u'dan baslayip v uzerinden -u'ya giden yarim cember
void yarim_cember_ciz(const glm::vec3& merkez, const glm::vec3& u, const glm::vec3& v, float r) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= DILIM / 2; i++) {
        float aci = 2.0f * PI * i / DILIM;
        glm::vec3 p = merkez + (u * cosf(aci) + v * sinf(aci)) * r;
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
}

// u-v duzleminde esit aralikli 8 dogru, alt ve ust cemberi birlestirir
void yan_cizgiler(const glm::vec3& alt, const glm::vec3& ust,
                  const glm::vec3& u, const glm::vec3& v, float r) {
    glBegin(GL_LINES);
    for (int i = 0; i < 8; i++) {
        float aci = 2.0f * PI * i / 8;
        glm::vec3 kayma = (u * cosf(aci) + v * sinf(aci)) * r;
        glVertex3f(alt.x + kayma.x, alt.y + kayma.y, alt.z + kayma.z);
        glVertex3f(ust.x + kayma.x, ust.y + kayma.y, ust.z + kayma.z);
    }
    glEnd();
}

void sekil_ciz(const PhysObj& n) {
    if (n.carpisti) glColor3f(1.0f, 0.0f, 0.0f);
    else glColor3f(0.0f, 1.0f, 0.0f);
    float r = n.yaricap * CIZIM_PAYI;

    glm::vec3 x_yon(1.0f, 0.0f, 0.0f), y_yon(0.0f, 1.0f, 0.0f), z_yon(0.0f, 0.0f, 1.0f);
    if (n.tip == KURE) {
        for (int i = -2; i <= 2; i++) {
            float enlem = i * PI / 6.0f;
            glm::vec3 merkez = n.konum + y_yon * (r * sinf(enlem));
            cember_ciz(merkez, x_yon, z_yon, r * cosf(enlem));
        }
        for (int i = 0; i < 4; i++) {
            float boylam = i * PI / 4.0f;
            glm::vec3 yatay(cosf(boylam), 0.0f, sinf(boylam));
            cember_ciz(n.konum, yatay, y_yon, r);
        }
    } else if (n.tip == SILINDIR) {
        glm::vec3 yarim = y_yon * (n.boy * 0.5f);
        cember_ciz(n.konum - yarim, x_yon, z_yon, r);
        cember_ciz(n.konum, x_yon, z_yon, r);
        cember_ciz(n.konum + yarim, x_yon, z_yon, r);
        yan_cizgiler(n.konum - yarim, n.konum + yarim, x_yon, z_yon, r);
    } else {
        // eksene dik iki birim vektor; eksen y'ye yakinsa yardimci olarak x alinir
        glm::vec3 yardimci = fabsf(n.eksen.y) < 0.9f ? y_yon : x_yon;
        glm::vec3 u = glm::normalize(glm::cross(n.eksen, yardimci));
        glm::vec3 v = glm::cross(n.eksen, u);
        glm::vec3 alt, ust;
        kapsul_uclari(n, alt, ust);
        cember_ciz(alt, u, v, r);
        cember_ciz(ust, u, v, r);
        yan_cizgiler(alt, ust, u, v, r);
        yarim_cember_ciz(ust, u, n.eksen, r);
        yarim_cember_ciz(ust, v, n.eksen, r);
        yarim_cember_ciz(alt, u, -n.eksen, r);
        yarim_cember_ciz(alt, v, -n.eksen, r);
    }
}

struct Camera {
    glm::vec3 hedef;
    float yatay, dikey, uzaklik;   // radyan

    void sifirla() {
        hedef = glm::vec3(0.0f, 1.0f, 0.0f);
        yatay = 0.6f;
        dikey = 0.4f;
        uzaklik = 12.0f;
    }

    glm::vec3 goz() const {
        glm::vec3 yon(cosf(dikey) * sinf(yatay), sinf(dikey), cosf(dikey) * cosf(yatay));
        return hedef + yon * uzaklik;
    }

    glm::mat4 bakis() const {
        return glm::lookAt(goz(), hedef, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void dondur(float dx, float dy) {
        yatay -= dx * DONDURME_HASSASIYET;
        dikey += dy * DONDURME_HASSASIYET;
        // dikey aci sinirli, yoksa kamera tepeden asip ters doner
        if (dikey > MAX_DIKEY) dikey = MAX_DIKEY;
        if (dikey < -MAX_DIKEY) dikey = -MAX_DIKEY;
    }

    void yaklas(float tekerlek) {
        uzaklik *= powf(0.9f, tekerlek);
        if (uzaklik < MIN_UZAKLIK) uzaklik = MIN_UZAKLIK;
        if (uzaklik > MAX_UZAKLIK) uzaklik = MAX_UZAKLIK;
    }

    void kaydir(float dx, float dy) {
        glm::vec3 ileri = glm::normalize(hedef - goz());
        glm::vec3 sag = glm::normalize(glm::cross(ileri, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 yukari = glm::cross(sag, ileri);
        // uzaklikla carpilir, boylece her zoom'da kaydirma hissi ayni kalir
        hedef += (-sag * dx + yukari * dy) * (uzaklik * KAYDIRMA_HASSASIYET);
    }
};

struct Frustum {
    glm::vec4 duzlem[6];   // sol, sag, alt, ust, yakin, uzak
    glm::mat4 pv;          // duzlemlerin cikarildigi matris, kilitliyken cizmek icin saklanir

    void guncelle(const glm::mat4& pv_yeni) {
        pv = pv_yeni;
        // GLM sutun-oncelikli: i. satir (m[0][i], m[1][i], m[2][i], m[3][i])
        glm::vec4 satir[4];
        for (int i = 0; i < 4; i++) {
            satir[i] = glm::vec4(pv[0][i], pv[1][i], pv[2][i], pv[3][i]);
        }
        // clip uzayinda -w <= x,y,z <= w; her esitsizlik bir duzlem verir
        duzlem[0] = satir[3] + satir[0];
        duzlem[1] = satir[3] - satir[0];
        duzlem[2] = satir[3] + satir[1];
        duzlem[3] = satir[3] - satir[1];
        duzlem[4] = satir[3] + satir[2];
        duzlem[5] = satir[3] - satir[2];
        for (int i = 0; i < 6; i++) {
            float uzunluk = glm::length(glm::vec3(duzlem[i]));
            duzlem[i] /= uzunluk;
        }
    }

    bool kure_gorunur(const glm::vec3& merkez, float r) const {
        for (int i = 0; i < 6; i++) {
            float mesafe = glm::dot(glm::vec3(duzlem[i]), merkez) + duzlem[i].w;
            if (mesafe < -r) return false;
        }
        return true;
    }
};

struct Nesne {
    const char* ad;
    FbxModel model;
    PhysObj sekil;
    glm::vec3 konum;      // modelin taban merkezi
    float aci;            // y ekseni etrafinda, derece
    bool yatay_kapsul;    // araba gibi yatan kapsul nesneyle birlikte doner
    glm::vec3 uzun_eksen; // yatay kapsulun model uzayindaki ekseni
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
    n.yatay_kapsul = tip == KAPSUL && yatay;
    n.uzun_eksen = glm::vec3(0.0f, 1.0f, 0.0f);

    if (tip == KURE) {
        // kutunun kosegenleri disarida kalsin diye yarim kosegenden biraz kucuk
        n.sekil.yaricap = glm::length(boyut) * 0.5f * 0.8f;
        n.sekil.boy = 0.0f;
    } else if (tip == SILINDIR) {
        n.sekil.yaricap = n.model.yatay_yaricap;
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
    if (n.yatay_kapsul) {
        // glRotatef(aci, 0, 1, 0) ile ayni donus
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

// nesne her acida bu kurenin icinde kalir: model kutusunun yari kosegeni ya da seklin siniri
float kirpma_yaricapi(const Nesne& n) {
    float model_r = glm::length(n.model.kutu_max - n.model.kutu_min) * 0.5f;
    return glm::max(model_r, sinir_yaricapi(n.sekil));
}

// ok tuslari kameraya gore calissin: zemin uzerinde kameranin baktigi yon ve sagi
glm::vec3 tasima_yonu(float kamera_yatay, float ileri, float saga) {
    glm::vec3 ileri_yon(-sinf(kamera_yatay), 0.0f, -cosf(kamera_yatay));
    glm::vec3 sag_yon(cosf(kamera_yatay), 0.0f, -sinf(kamera_yatay));
    glm::vec3 yon = ileri_yon * ileri + sag_yon * saga;
    if (glm::length(yon) > 0.0f) yon = glm::normalize(yon);   // capraz giderken hizlanmasin
    return yon;
}

// girdiler -1, 0 ya da 1; yatay hareket kameraya gore, hiz her yonde ayni
void nesneyi_tasi(Nesne& n, float kamera_yatay, float ileri, float saga, float yukari,
                  float donus, float dt) {
    glm::vec3 yon = tasima_yonu(kamera_yatay, ileri, saga) + glm::vec3(0.0f, yukari, 0.0f);
    if (glm::length(yon) > 1.0f) yon = glm::normalize(yon);   // capraz yukselirken de
    n.konum += yon * (HAREKET_HIZI * dt);
    if (n.konum.y < 0.0f) n.konum.y = 0.0f;
    n.aci += donus * DONME_HIZI * dt;
    sekli_guncelle(n);
}

glm::mat4 projeksiyon_matrisi() {
    float oran = (float)pencere_en / (float)pencere_boy;
    return glm::perspective(glm::radians(55.0f), oran, 0.1f, 80.0f);
}

// NDC kupunun 8 kosesi ters matrisle dunyaya tasinir; i'nin bitleri x, y, z isaretidir
void frustum_koseleri(const glm::mat4& pv, glm::vec3 kose[8]) {
    glm::mat4 ters = glm::inverse(pv);
    for (int i = 0; i < 8; i++) {
        glm::vec4 p = ters * glm::vec4((i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f,
                                       (i & 4) ? 1.0f : -1.0f, 1.0f);
        kose[i] = glm::vec3(p) / p.w;
    }
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

// K ile dondurulan gorus hacmi sari cizilir, disinda kalan nesnelerin neden kayboldugu gorulsun
void frustum_ciz() {
    glm::vec3 k[8];
    frustum_koseleri(frustum.pv, k);
    // kupun 12 kenari: yalnizca bir bitte (x, y ya da z isaretinde) farkli kose ciftleri
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 8; i++) {
        for (int bit = 1; bit < 8; bit <<= 1) {
            if (i & bit) continue;
            glVertex3f(k[i].x, k[i].y, k[i].z);
            glVertex3f(k[i | bit].x, k[i | bit].y, k[i | bit].z);
        }
    }
    glEnd();
}

void baslik_yaz(GLFWwindow* pencere, int cizilen) {
    static int onceki_cizilen = -1, onceki_cift = -1, onceki_secili = -1, onceki_kirpma = -1;
    int cift = carpisma.carpisan_cift;
    int kirpma = !kirpma_acik ? 0 : (frustum_kilitli ? 2 : 1);
    if (cizilen == onceki_cizilen && cift == onceki_cift && secili == onceki_secili
        && kirpma == onceki_kirpma)
        return;
    onceki_cizilen = cizilen;
    onceki_cift = cift;
    onceki_secili = secili;
    onceki_kirpma = kirpma;
    const char* kirpma_yazi[3] = { "kapali", "acik", "kilitli" };
    char yazi[200];
    snprintf(yazi, sizeof(yazi),
             "Carpisma Demo | cizilen: %d/%d | carpisan cift: %d | secili: %s | kirpma: %s",
             cizilen, NESNE_SAYISI, cift, nesneler[secili].ad, kirpma_yazi[kirpma]);
    glfwSetWindowTitle(pencere, yazi);
}

void sahne_ciz(GLFWwindow* pencere) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projeksiyon = projeksiyon_matrisi();
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
        gorunur[i] = !kirpma_acik || frustum.kure_gorunur(n.sekil.konum, kirpma_yaricapi(n));
        if (!gorunur[i]) continue;
        cizilen++;

        glPushMatrix();
        glTranslatef(n.konum.x, n.konum.y, n.konum.z);
        glRotatef(n.aci, 0.0f, 1.0f, 0.0f);
        n.model.ciz();
        glPopMatrix();
    }

    // tel kafesler en son ve derinlik testi kapaliyken: modelin icinde kalan cizgiler de gorunsun
    glDisable(GL_DEPTH_TEST);
    if (sekilleri_goster) {
        for (int i = 0; i < NESNE_SAYISI; i++) {
            if (!gorunur[i]) continue;
            glLineWidth(i == secili ? 2.5f : 1.0f);
            sekil_ciz(nesneler[i].sekil);
        }
        glLineWidth(1.0f);
    }
    if (frustum_kilitli) frustum_ciz();
    glEnable(GL_DEPTH_TEST);
    baslik_yaz(pencere, cizilen);
}

// basili tutulan tuslar: secili nesneyi kameraya gore tasir ve dondurur
void hareket_et(GLFWwindow* pencere, float dt) {
    float ileri = 0.0f, saga = 0.0f, yukari = 0.0f, donus = 0.0f;
    if (glfwGetKey(pencere, GLFW_KEY_UP) == GLFW_PRESS) ileri += 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_DOWN) == GLFW_PRESS) ileri -= 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_RIGHT) == GLFW_PRESS) saga += 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_LEFT) == GLFW_PRESS) saga -= 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_PAGE_UP) == GLFW_PRESS) yukari += 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) yukari -= 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_Q) == GLFW_PRESS) donus += 1.0f;
    if (glfwGetKey(pencere, GLFW_KEY_E) == GLFW_PRESS) donus -= 1.0f;
    nesneyi_tasi(nesneler[secili], kamera.yatay, ileri, saga, yukari, donus, dt);
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
        frustum_kilitli = false;
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

// dugmeye basildigi anda imlec yeri alinir; yoksa ilk hareket eski bir konumdan
// hesaplanir ve kamera bir anda ziplar (imlec pencereye disaridan girdiyse ya da hic oynamadiysa)
void dugme_olayi(GLFWwindow* pencere, int, int eylem, int) {
    if (eylem == GLFW_PRESS) glfwGetCursorPos(pencere, &fare_x, &fare_y);
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
    printf("Nesne   : 1-5 ya da TAB sec, ok tuslari kameraya gore tasi, PgUp/PgDn yukselt,\n");
    printf("          Q/E dondur\n");
    printf("Diger   : G carpisma sekilleri, C frustum kirpma, K frustumu kilitle (sari cizilir)\n");
    printf("          R sifirla, ESC cikis\n");
}

// GLEW fonksiyonlari GLX ile yukler; GLFW 3.4 Wayland'de acilirsa GLX olmaz, once X11 denenir
bool glfw_baslat() {
#ifdef GLFW_PLATFORM_X11
    if (glfwPlatformSupported(GLFW_PLATFORM_X11)) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        if (glfwInit() == GLFW_TRUE) return true;
        glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
    }
#endif
    return glfwInit() == GLFW_TRUE;
}

int main() {
    if (!glfw_baslat()) {
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
    GLenum glew_durum = glewInit();
#ifdef GLEW_ERROR_NO_GLX_DISPLAY
    // Wayland'de GLX ekrani olmasa da GL fonksiyonlari yuklenmis olur
    if (glew_durum == GLEW_ERROR_NO_GLX_DISPLAY) glew_durum = GLEW_OK;
#endif
    if (glew_durum != GLEW_OK) {
        printf("GLEW baslatilamadi\n");
        glfwTerminate();
        return 1;
    }

    glfwSetKeyCallback(pencere, tus_olayi);
    glfwSetCursorPosCallback(pencere, fare_hareketi);
    glfwSetMouseButtonCallback(pencere, dugme_olayi);
    glfwSetScrollCallback(pencere, tekerlek_olayi);
    glfwSetFramebufferSizeCallback(pencere, boyut_degisti);
    glfwGetFramebufferSize(pencere, &pencere_en, &pencere_boy);
    glfwGetCursorPos(pencere, &fare_x, &fare_y);
    glViewport(0, 0, pencere_en, pencere_boy);

    ayarlar();
    kamera.sifirla();
    if (!sahneyi_kur()) {
        glfwTerminate();
        return 1;
    }
    tuslari_yazdir();
    // ilk kare cizilmeden K'ye basilirsa bos (sifir) bir frustum kilitlenmesin
    frustum.guncelle(projeksiyon_matrisi() * kamera.bakis());

    double onceki = glfwGetTime();
    while (!glfwWindowShouldClose(pencere)) {
        int en = 0, boy = 0;
        glfwGetFramebufferSize(pencere, &en, &boy);
        // simge durumunda cizilecek yer yok; bos donup islemciyi yakmak yerine olay beklenir
        if (en == 0 || boy == 0) {
            glfwWaitEvents();
            continue;
        }
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
