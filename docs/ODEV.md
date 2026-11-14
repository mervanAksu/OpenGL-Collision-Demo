# <img src="media/image1.jpeg" style="width:1.76922in;height:2.125in"
alt="http://upload.wikimedia.org/wikipedia/en/9/96/Sakarya_%C3%9Cniversitesi_(logo).jpg" />

# SAKARYA ÜNİVERSİTESİ

# BİLGİSAYAR VE BİLİŞİM BİLİMLERİ FAKÜLTESİ

# BİLGİSAYAR MÜHENDİSLİĞİ BÖLÜMÜ

# 

# 

# 

# 

# Bilgisayar Grafiği

# Ödevi I

3B Sahne, FBX Modeller ve Çarpışma Denetimi

Ödev

# 1. Ödev Tanımı ve Amaçlar

Bu ödevde OpenGL ve C++ kullanarak 5 adet FBX modeli içeren, çarpışma
denetimi ve kamera kontrolü destekleyen interaktif bir 3B sahne
geliştirilecektir.

## 1.1 Kazanımlar

> • OpenGL ile gerçek zamanlı 3B render pipeline kurmak
>
> • ufbx kütüphanesi ile FBX model yüklemek ve çizmek
>
> • Her model için Küre, Silindir veya Kapsül primitifi oluşturmak
>
> • Birden fazla nesne çifti için çarpışma algoritmaları geliştirmek
>
> • Orbit/Arcball kamera sistemi ile sahneyi serbestçe incelemek
>
> • View Frustum Culling (kırpma) ile sahne optimizasyonu yapmak
>
> • Primitifleri wireframe çizmek; çarpışmada renk değiştirmek

## 1.2 Teslim Edilecekler

| **\#** | **Dosya / Bileşen** | **Açıklama**                                                   |
|--------|---------------------|----------------------------------------------------------------|
| 1      | main.cpp            | Tam kaynak kodu                                                |
| 2      | ufbx.h + ufbx.c     | FBX yükleyici                                                  |
| 3      | model1..5.fbx       | 5 adet FBX model                                               |
| 4      | Makefile            | Derleme kodu                                                   |
| 5      | rapor.pdf           | Ekran görüntüleri + algoritma açıklaması(kendinize özgü olsun) |

## 1.3 Puan Dağılımı

| **Değerlendirme Kriteri**         | **Puan** |
|-----------------------------------|----------|
| FBX yükleme ve çizim              | 20       |
| Çarpışma primitifleri — wireframe | 25       |
| Çarpışma denetimi algoritmaları   | 25       |
| Kamera kontrolü                   | 15       |
| View Frustum Culling              | 10       |
| Kod kalitesi ve rapor             | 5        |
| TOPLAM                            | 100      |

# 2. Teknik Gereksinimler

## 2.1 Kütüphaneler

| **Kütüphane** | **Sürüm** | **Amaç**                    |
|---------------|-----------|-----------------------------|
| OpenGL        | 2.1+      | Grafik API                  |
| GLFW          | 3.3+      | Pencere ve giriş yönetimi   |
| GLEW          | 2.2+      | OpenGL uzantı yükleyici     |
| GLM           | 0.9.9+    | Vektör ve matris matematiği |
| ufbx          | son sürüm | FBX model yükleme           |

## 2.2 Platform Kurulumu

### Ubuntu / Debian

> sudo apt install libglfw3-dev libglew-dev libglm-dev

### macOS

> brew install glfw glew glm

### Windows (MSYS2 / MinGW64)

> pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
> mingw-w64-x86_64-glm

## 2.3 ufbx Kurulumu

ufbx header-only değildir; iki dosya proje klasörüne kopyalanmalıdır:

> • https://github.com/bqqbarbhg/ufbx adresinden ufbx.h ve ufbx.c
> dosyaları indirilir
>
> • Her ikisi src/ klasörüne kopyalanır
>
> • Derleme sırasında ufbx.c kaynak listesine eklenir

# 3. Proje Yapısı ve Dosyalar

> collision_demo/
>
> ├── src/
>
> │ ├── main.cpp \# Ana uygulama
>
> │ ├── ufbx.h \# FBX yükleyici başlığı
>
> │ └── ufbx.c \# FBX yükleyici kaynağı
>
> ├── models/
>
> │ ├── araba.fbx
>
> │ ├── insan.fbx
>
> │ ├── agac.fbx
>
> │ ├── kasa.fbx
>
> │ └── varil.fbx
>
> ├── Makefile
>
> └── rapor.pdf

## 3.1 Mimari Bileşenler

| **Bileşen**     | **Açıklama**                                           |
|-----------------|--------------------------------------------------------|
| FbxModel        | ufbx ile mesh yükler, GPU'ya aktarır, çizer            |
| PhysObj         | Çarpışma primitifini (tip, konum, boyut) tutar         |
| CollisionSystem | Tüm nesne çiftleri arasında çarpışma denetimi yapar    |
| Camera          | Orbit/Arcball modeli; fare ve klavyeyle kontrol edilir |
| Frustum         | 6 düzlemli View Frustum Culling uygular                |

KOPYA ÖDEVLER

Ödevler bireyseldir bu yüzden ödev için yazılan kodların kesinlikle
paylaşılmaması gerekiyor. Bütün ödevler karşılaştırılacaktır. Birbirine
çok benzeyen ödevler 0 puan olarak değerlendirilecektir.

SON TESLİM TARİHİ

Sistemde belirtilen tarih ve saattir
