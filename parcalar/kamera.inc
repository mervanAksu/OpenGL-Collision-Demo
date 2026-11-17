const float DONDURME_HASSASIYET = 0.005f;
const float KAYDIRMA_HASSASIYET = 0.0015f;
const float MIN_UZAKLIK = 1.5f;
const float MAX_UZAKLIK = 60.0f;
const float MAX_DIKEY = 85.0f * PI / 180.0f;

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
