# Mimari ve arayuz sozlesmesi

Koordinat sistemi: Y yukari, zemin y=0, birim ~metre. Nihai dosya su parcalarin sirayla
birlesimidir: `ortak.inc` -> `fbx_model.inc` -> `carpisma.inc` -> `kamera.inc` -> sahne (sef).
Her parca, basinda `#include "ortak.inc"` oldugu varsayilarak tek basina derlenebilmeli.

## fbx_model.inc  (ajan: fbx-cizim)
```cpp
struct FbxModel {
    GLuint vbo;              // pos(3)+normal(3) float, ucgen listesi, index yok
    int kose_sayisi;
    glm::vec3 renk;          // FBX materyalinin diffuse rengi yoksa acik gri
    glm::vec3 kutu_min, kutu_max;   // olceklenmis modelin AABB'si (model uzayi)
    bool yukle(const char* yol, float hedef_boy);
    void ciz() const;        // o anki MODELVIEW ile cizer, renk ve isik durumunu kendi ayarlar
    void bosalt();
};
```
- `yukle`: ufbx ile okur (Y-up, sag el, metre hedefli load opts), tum mesh'leri node'un
  geometry_to_world matrisiyle dunya uzayina tasiyip TEK tampona toplar, yuzleri ucgenler.
  Sonra modeli normalize eder: en uzun AABB kenari `hedef_boy` olur, taban y=0'a oturur,
  x ve z'de merkezlenir. Birden fazla materyal varsa kose rengi yerine ilk materyalin rengi yeter
  (istenirse renk de tampona eklenebilir: o zaman pos+normal+renk).
- Normal yoksa yuz normali hesaplanir.

## carpisma.inc  (ajan: carpisma)
```cpp
enum SekilTipi { KURE, SILINDIR, KAPSUL };
struct PhysObj {
    SekilTipi tip;
    glm::vec3 konum;     // seklin merkezi (dunya)
    float yaricap;
    float boy;           // SILINDIR: toplam yukseklik. KAPSUL: uctan uca toplam uzunluk. KURE: kullanilmaz
    glm::vec3 eksen;     // KAPSUL icin birim eksen (yatay da olabilir). SILINDIR her zaman (0,1,0)
    bool carpisti;
};
bool carpisiyor_mu(const PhysObj& a, const PhysObj& b);   // tum 6 tip cifti
struct CollisionSystem {
    std::vector<PhysObj*> nesneler;
    int carpisan_cift;
    void denetle();      // tum ciftler, carpisti bayraklarini ve carpisan_cift'i gunceller
};
void sekil_ciz(const PhysObj& n);   // wireframe; carpisti ise kirmizi, degilse yesil
float sinir_yaricapi(const PhysObj& n);   // sekli saran kurenin yaricapi (frustum icin)
```
- Saf matematik fonksiyonlari (nokta-dogru parcasi, parca-parca en yakin nokta vb.) OpenGL
  cagirmaz; `test/carpisma_test.cpp` bunlari GL olmadan test eder. Bunun icin GL kullanan
  `sekil_ciz` kismi `#ifndef TEST_DERLEME` ile sarilir.
- Cift testleri: kure-kure, kure-kapsul, kapsul-kapsul (genel parca-parca), kure-silindir,
  silindir-silindir (ikisi de dik), kapsul-silindir (kapsul ekseni serbest, silindir dik).
- `sekil_ciz` derste gorulen yolla cizer: glBegin(GL_LINE_LOOP / GL_LINES / GL_LINE_STRIP).
  Dunya koordinatinda cizer (cagiran MODELVIEW'e sadece kamera matrisini yuklemis olur).

## kamera.inc  (ajan: kamera-frustum)
```cpp
struct Camera {
    glm::vec3 hedef; float yatay, dikey, uzaklik;   // acilar radyan
    glm::vec3 goz() const;
    glm::mat4 bakis() const;
    void dondur(float dx, float dy);   // fare pikseli farki
    void yaklas(float tekerlek);
    void kaydir(float dx, float dy);   // hedefi ekran duzleminde tasir
};
struct Frustum {
    glm::vec4 duzlem[6];               // (a,b,c,d), normal ice bakar, normalize
    void guncelle(const glm::mat4& pv);    // projeksiyon * bakis matrisinden 6 duzlem
    bool kure_gorunur(const glm::vec3& merkez, float r) const;
};
```
- `test/frustum_test.cpp`: bilinen bir kamera icin on/arka/yan noktalarin dogru siniflandigini sinar.

## Sahne (sef)
GLFW pencere + girdi, isik, zemin izgarasi, 5 model + 5 PhysObj, secili nesneyi klavyeyle
tasima, her karede: carpisma denetimi -> frustum guncelle -> gorunenleri ciz -> baslikta sayac.
