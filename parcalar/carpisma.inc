enum SekilTipi { KURE, SILINDIR, KAPSUL };

const int UCLU_ARAMA_ADIM = 30;

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

#ifndef TEST_DERLEME
const int DILIM = 24;   // cemberdeki kose sayisi

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

    glm::vec3 x_yon(1.0f, 0.0f, 0.0f), y_yon(0.0f, 1.0f, 0.0f), z_yon(0.0f, 0.0f, 1.0f);
    if (n.tip == KURE) {
        for (int i = -2; i <= 2; i++) {
            float enlem = i * PI / 6.0f;
            glm::vec3 merkez = n.konum + y_yon * (n.yaricap * sinf(enlem));
            cember_ciz(merkez, x_yon, z_yon, n.yaricap * cosf(enlem));
        }
        for (int i = 0; i < 4; i++) {
            float boylam = i * PI / 4.0f;
            glm::vec3 yatay(cosf(boylam), 0.0f, sinf(boylam));
            cember_ciz(n.konum, yatay, y_yon, n.yaricap);
        }
    } else if (n.tip == SILINDIR) {
        glm::vec3 yarim = y_yon * (n.boy * 0.5f);
        cember_ciz(n.konum - yarim, x_yon, z_yon, n.yaricap);
        cember_ciz(n.konum, x_yon, z_yon, n.yaricap);
        cember_ciz(n.konum + yarim, x_yon, z_yon, n.yaricap);
        yan_cizgiler(n.konum - yarim, n.konum + yarim, x_yon, z_yon, n.yaricap);
    } else {
        // eksene dik iki birim vektor; eksen y'ye yakinsa yardimci olarak x alinir
        glm::vec3 yardimci = fabsf(n.eksen.y) < 0.9f ? y_yon : x_yon;
        glm::vec3 u = glm::normalize(glm::cross(n.eksen, yardimci));
        glm::vec3 v = glm::cross(n.eksen, u);
        glm::vec3 alt, ust;
        kapsul_uclari(n, alt, ust);
        cember_ciz(alt, u, v, n.yaricap);
        cember_ciz(ust, u, v, n.yaricap);
        yan_cizgiler(alt, ust, u, v, n.yaricap);
        yarim_cember_ciz(ust, u, n.eksen, n.yaricap);
        yarim_cember_ciz(ust, v, n.eksen, n.yaricap);
        yarim_cember_ciz(alt, u, -n.eksen, n.yaricap);
        yarim_cember_ciz(alt, v, -n.eksen, n.yaricap);
    }
}
#endif
