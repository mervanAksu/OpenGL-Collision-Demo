# OpenGL Çarpışma Demosu

Sakarya Üniversitesi Bilgisayar Grafiği dersi, Ödev I: OpenGL ve C++ ile yazılmış, 5 FBX modeli
içeren, çarpışma denetimi ve orbit kamera destekleyen etkileşimli 3B sahne.

## Özellikler

- **FBX yükleme:** [ufbx](https://github.com/ufbx/ufbx) ile 5 model okunur, materyal renkleriyle çizilir.
- **Çarpışma şekilleri:** her model için küre, dik silindir ya da kapsül; tel kafes olarak çizilir,
  çarpışınca yeşilden kırmızıya döner.
- **Çarpışma testleri:** küre–küre, küre–kapsül, kapsül–kapsül, küre–silindir, silindir–silindir,
  kapsül–silindir.
- **Orbit kamera:** fare ve klavyeyle döndürme, yaklaşma, kaydırma.
- **View frustum culling:** görüş hacminin dışındaki nesneler çizilmez; pencere başlığında sayaç.

Çizim, derste işlendiği gibi OpenGL 2.1 sabit işlem hattı (fixed-function) ile yapılır; shader yoktur.

## Kontroller

| Tuş | İşlev |
|---|---|
| Sol fare / sağ fare / tekerlek | Kamerayı döndür / kaydır / yaklaştır |
| W A S D, Z X | Kamerayı klavyeyle döndür, yaklaştır-uzaklaştır |
| 1–5, TAB | Nesne seç |
| Ok tuşları, PgUp / PgDn | Seçili nesneyi taşı, yükselt-alçalt |
| Q / E | Seçili nesneyi döndür |
| G | Çarpışma şekillerini göster / gizle |
| C | Frustum kırpmayı aç / kapa |
| K | Frustumu kilitle (kırpılan nesneleri dışarıdan görmek için) |
| R | Sahneyi sıfırla |
| ESC | Çıkış |

## Derleme

Gerekenler: GLFW 3.3+, GLEW, GLM ve bir C++11 derleyicisi.

**Windows (MSYS2 MINGW64 kabuğu):**
```
pacman -S --needed mingw-w64-x86_64-gcc make mingw-w64-x86_64-glfw mingw-w64-x86_64-glew mingw-w64-x86_64-glm
make calistir
```

**Ubuntu / Debian:** `sudo apt install libglfw3-dev libglew-dev libglm-dev && make calistir`

**macOS:** `brew install glfw glew glm && make calistir`

Program `models/` klasörünü göreli yoldan okur; proje kök klasöründen çalıştırılmalıdır.

## Klasör yapısı

```
src/        main.cpp (tek kaynak dosyası), ufbx.h, ufbx.c
models/     5 FBX model
Makefile
parcalar/   main.cpp'yi oluşturan parçalar (FBX, çarpışma, kamera, sahne)
test/       OpenGL gerektirmeyen birim testleri
araclar/    birlestir.sh (parçalardan main.cpp üretir), test.sh
docs/       ödev metni, mimari sözleşme, kod stili, model kaynakları, rapor notları
.claude/    alt ajan tanımları
```

`src/main.cpp` elle düzenlenmez; `parcalar/` altındaki dosyalardan `sh araclar/birlestir.sh` ile üretilir.

## Geliştirme süreci

Proje, Claude ile "şef + alt ajanlar" düzeninde geliştirildi. Ana oturum mimariyi ve arayüz
sözleşmesini (`docs/MIMARI.md`) belirledi, işi dağıttı ve parçaları birleştirdi. Her alt ajan yalnızca
kendi parçasını yazdı:

| Ajan | Sorumluluk |
|---|---|
| `fbx-cizim` | ufbx ile yükleme, VBO ile çizim |
| `carpisma` | çarpışma şekilleri, altı çift testi, tel kafes çizimi |
| `kamera-frustum` | orbit kamera, altı düzlemli kırpma |
| `model-bulucu` | serbest lisanslı modellerin bulunması ve doğrulanması |
| `denetci` | kodu yazmamış bağımsız göz: rubrik ve hata denetimi |

Kurallar `CLAUDE.md`, ajan tanımları `.claude/agents/` altındadır.

## Testler

```
sh araclar/test.sh
```

Çarpışma matematiği için 143, frustum için 28 kontrol çalışır (elle hesaplanmış durumlar ve rastgele
üretilmiş çiftlerin kaba örneklemeyle karşılaştırılması).

## Modeller

Modeller Quaternius'un CC0 paketlerinden alınmıştır; ayrıntılar `docs/MODELLER.md` içinde.
