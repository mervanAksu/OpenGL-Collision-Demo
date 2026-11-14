// GL olmadan yukleme ve normalizasyon testi: g++ -DTEST_DERLEME ... ufbx.o
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
    bool iyi = m.kose_sayisi % 3 == 0 && yakin(en_uzun, hedef_boy) && yakin(m.kutu_min.y, 0.0f)
        && yakin(m.kutu_min.x, -m.kutu_max.x) && yakin(m.kutu_min.z, -m.kutu_max.z)
        && yakin(glm::length(gmin - m.kutu_min), 0.0f) && yakin(glm::length(gmax - m.kutu_max), 0.0f)
        && normal_iyi;
    printf("  en_uzun=%.4f hedef=%.4f normaller_birim=%d -> %s\n", en_uzun, hedef_boy, normal_iyi,
           iyi ? "GECTI" : "KALDI");
    if (!iyi) hata_sayisi++;
    m.bosalt();
}

int main() {
    const char* d = "/home/claude/deps/ufbx/data/";
    const char* dosyalar[] = { "blender_272_cube_7400_binary.fbx", "maya_cube_7500_binary.fbx",
        "blender_282_suzanne_and_transform_7400_binary.fbx", "blender_293_instancing_7400_binary.fbx",
        "blender_293_material_mapping_7400_binary.fbx" };
    float boylar[] = { 2.0f, 1.5f, 3.0f, 1.0f, 2.5f };
    for (int i = 0; i < 5; i++) {
        char yol[512];
        snprintf(yol, sizeof(yol), "%s%s", d, dosyalar[i]);
        dene(yol, boylar[i]);
    }
    printf("hata_sayisi=%d\n", hata_sayisi);
    return hata_sayisi == 0 ? 0 : 1;
}
