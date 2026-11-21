// GL olmadan yukleme ve normalizasyon testi, proje kokunden calistirilir (models/ okunur):
//   g++ -std=c++11 -DTEST_DERLEME -DGLEW_NO_GLU <basliklar> -Isrc -Iparcalar
//       test/fbx_yukle_test.cpp ufbx.o
#include "ortak.inc"
#include "fbx_model.inc"

int hata_sayisi = 0;

bool yakin(float a, float b) { return fabsf(a - b) < 1e-4f; }

void dene(const char* yol, float hedef_boy) {
    FbxModel m;
    if (!m.yukle(yol, hedef_boy)) { hata_sayisi++; return; }
    glm::vec3 b = m.kutu_max - m.kutu_min;
    float en_uzun = glm::max(b.x, glm::max(b.y, b.z));
    printf("%s\n  kose=%d ucgen=%d renk=(%.2f %.2f %.2f)\n", yol, m.kose_sayisi, m.kose_sayisi / 3,
           m.renk.r, m.renk.g, m.renk.b);
    printf("  min=(%.4f %.4f %.4f) max=(%.4f %.4f %.4f)\n", m.kutu_min.x, m.kutu_min.y,
           m.kutu_min.z, m.kutu_max.x, m.kutu_max.y, m.kutu_max.z);

    // tampondaki gercek koselerle de karsilastir
    std::vector<float> veri;
    glm::vec3 a1, a2, r;
    fbx_oku(yol, hedef_boy, veri, a1, a2, r);
    glm::vec3 gmin(1e9f), gmax(-1e9f);
    bool normal_iyi = true;
    for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT) {
        glm::vec3 p(veri[i], veri[i + 1], veri[i + 2]);
        gmin = glm::min(gmin, p);
        gmax = glm::max(gmax, p);
        float nl = glm::length(glm::vec3(veri[i + 3], veri[i + 4], veri[i + 5]));
        if (!yakin(nl, 1.0f)) normal_iyi = false;
    }
    // renkler gecerli araliktaysa ve model tek renk degilse materyaller okunmus demektir
    bool renk_iyi = true;
    std::vector<glm::vec3> renkler;
    float en_uzak_yatay = 0.0f;
    for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT) {
        glm::vec3 c(veri[i + 6], veri[i + 7], veri[i + 8]);
        if (glm::min(c.r, glm::min(c.g, c.b)) < 0.0f || glm::max(c.r, glm::max(c.g, c.b)) > 1.0f)
            renk_iyi = false;
        bool yeni = true;
        for (size_t j = 0; j < renkler.size(); j++)
            if (glm::length(renkler[j] - c) < 1e-3f) yeni = false;
        if (yeni) renkler.push_back(c);
        en_uzak_yatay = glm::max(en_uzak_yatay, glm::length(glm::vec2(veri[i], veri[i + 2])));
    }
    bool iyi = m.kose_sayisi > 0 && m.kose_sayisi % 3 == 0 && yakin(en_uzun, hedef_boy)
        && yakin(m.kutu_min.y, 0.0f)
        && yakin(m.kutu_min.x, -m.kutu_max.x) && yakin(m.kutu_min.z, -m.kutu_max.z)
        && yakin(glm::length(gmin - m.kutu_min), 0.0f)
        && yakin(glm::length(gmax - m.kutu_max), 0.0f)
        && normal_iyi && renk_iyi && renkler.size() >= 2 && yakin(m.yatay_yaricap, en_uzak_yatay);
    printf("  en_uzun=%.4f hedef=%.4f normaller_birim=%d renk_sayisi=%d yatay_r=%.4f -> %s\n",
           en_uzun, hedef_boy, normal_iyi, (int)renkler.size(), m.yatay_yaricap,
           iyi ? "GECTI" : "KALDI");
    if (!iyi) hata_sayisi++;
    m.bosalt();
}

int main() {
    // sahnedeki dosyalar ve boylar
    const char* dosyalar[] = { "models/araba.fbx", "models/insan.fbx", "models/kuyu.fbx",
                               "models/bomba.fbx", "models/mantar.fbx" };
    float boylar[] = { 3.2f, 1.8f, 2.2f, 0.9f, 1.2f };
    for (int i = 0; i < 5; i++) dene(dosyalar[i], boylar[i]);

    // olmayan dosya cokmeden false donmeli
    FbxModel bos;
    bool acilmadi = !bos.yukle("models/olmayan_dosya.fbx", 1.0f);
    printf("olmayan dosya -> %s\n", acilmadi ? "GECTI" : "KALDI");
    if (!acilmadi) hata_sayisi++;

    printf("hata_sayisi=%d\n", hata_sayisi);
    return hata_sayisi == 0 ? 0 : 1;
}
