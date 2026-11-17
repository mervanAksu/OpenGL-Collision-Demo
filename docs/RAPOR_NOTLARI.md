# Rapor için notlar

Bu dosya rapor değildir; raporu yazarken yanında duracak özet. Raporu kendi cümlelerinle yaz,
her başlığın altına programdan aldığın ekran görüntüsünü koy.

## Genel yapı
- Çizim dersteki gibi sabit işlem hattı (fixed-function) ile: `glMatrixMode`, `glLoadMatrixf`,
  `glTranslatef/glRotatef`, `glBegin/glEnd`, `glLightfv`. Shader yok.
- Matrisler GLM ile hesaplanıp `glLoadMatrixf` ile OpenGL'e veriliyor.
- GLEW'in işi: `glGenBuffers / glBindBuffer / glBufferData` (VBO) fonksiyonlarını yüklemek.

## FBX yükleme (`fbx_oku`, `FbxModel`)
- ufbx sahneyi Y-yukarı, metre birimine çevirerek okur (`target_axes`, `target_unit_meters`).
- Her mesh'in her kullanımı (instance) için yüzler `ufbx_triangulate_face` ile üçgenlenir,
  köşeler `geometry_to_world` matrisiyle dünya uzayına taşınır.
- Köşe başına 9 float: konum, normal, renk. Renk yüzün materyalinden gelir (doku yok).
- Model normalize edilir: en uzun kenar istenen boya ölçeklenir, taban y=0'a oturur, x-z'de ortalanır.
  Böylece farklı ölçekte gelen 5 model aynı sahnede tutarlı durur.
- Çizim: `glVertexPointer / glNormalPointer / glColorPointer` + `glDrawArrays(GL_TRIANGLES)`.

## Çarpışma şekilleri (`PhysObj`, `sekil_olustur`)
| Model | Şekil | Ölçü nereden |
|---|---|---|
| araba | yatay kapsül | uzun kenar = boy, diğer iki kenarın büyüğü / 2 = yarıçap |
| insan | dik kapsül | yükseklik = boy, dar yatay kenar / 2 = yarıçap |
| ağaç, varil | dik silindir | yükseklik = boy, y eksenine en uzak köşenin uzaklığı = yarıçap |
| kasa | küre | kutu yarı köşegeninin 0.8'i |

Şekiller `glBegin(GL_LINE_LOOP / GL_LINE_STRIP / GL_LINES)` ile tel kafes çizilir; çarpışan kırmızı, diğerleri yeşil.

## Çarpışma testleri (`carpisiyor_mu`)
1. **Küre–küre:** merkezler arası uzaklığın karesi ≤ (r1+r2)².
2. **Küre–kapsül:** küre merkezinin kapsülün iç doğru parçasına en yakın noktası bulunur
   (izdüşüm oranı 0–1 arasına sıkıştırılır), uzaklık yarıçaplar toplamıyla karşılaştırılır.
3. **Kapsül–kapsül:** iki doğru parçası arasındaki en kısa uzaklık ya bir uçtadır ya da iki
   doğrunun ortak dikmesindedir. Dört uç–parça uzaklığı ve (varsa) ortak dikme uzaklığının
   en küçüğü alınır. Ortak dikme, uzaklık karesinin iki parametreye göre türevleri sıfıra
   eşitlenerek bulunur; paralel parçalarda payda sıfır olduğundan bu adım atlanır.
4. **Küre–silindir:** küre merkezi silindirin içine sıkıştırılır (x-z'de yarıçapa, y'de ±boy/2'ye);
   merkez ile bu nokta arasındaki uzaklık küre yarıçapıyla karşılaştırılır. Kenar (köşe) durumu da doğru çıkar.
5. **Silindir–silindir:** ikisi de dik olduğundan silindir = daire × yükseklik aralığı.
   Hem y aralıkları hem x-z daireleri örtüşüyorsa çarpışma vardır.
6. **Kapsül–silindir:** silindir konveks olduğu için kapsül parçası boyunca "silindire uzaklık"
   tek çukurlu bir fonksiyondur; minimumu üçlü arama (ternary search, 30 adım) ile bulunur
   ve kapsül yarıçapıyla karşılaştırılır.

`CollisionSystem::denetle` her karede tüm çiftleri (5 nesne → 10 çift) dolaşır.

## Kamera (`Camera`)
Hedef nokta etrafında küresel koordinat: yatay açı, dikey açı, uzaklık.
`goz = hedef + uzaklik * (cos(d)sin(y), sin(d), cos(d)cos(y))`, görüş matrisi `glm::lookAt`.
Dikey açı ±85° ile sınırlı (tepe noktasında kamera ters dönmesin diye).

## Frustum kırpma (`Frustum`)
Bir nokta görüş hacminin içindeyse kırpma (clip) koordinatlarında −w ≤ x, y, z ≤ w sağlanır.
clip = PV · p olduğundan her eşitsizlik p için bir düzlem denklemidir: düzlemler
PV matrisinin `4. satır ± 1., 2., 3. satır` toplamlarıdır. Düzlemler normalize edilir;
bir nesnenin sınır küresi herhangi bir düzlemin arkasında yarıçapından fazla kalıyorsa çizilmez.
Pencere başlığındaki "cizilen: x/5" sayacı sonucu, "kirpma: acik/kapali/kilitli" durumu gösterir.
`K` tuşu frustumu dondurur ve sarı çizer: kamera uzaklaşınca sarı hacmin dışında kalan nesnelerin
çizilmediği görülür. Sarı çizgiler, NDC küpünün 8 köşesinin (PV)⁻¹ ile dünyaya taşınmasıyla bulunur.

## Doğrulama
- Çarpışma fonksiyonları: 143 kontrol (elle hesaplanmış durumlar + 2000 rastgele çiftin kaba
  örneklemeyle karşılaştırılması).
- Frustum: 28 kontrol; rastgele noktalar clip uzayındaki −w ≤ x, y, z ≤ w koşuluyla karşılaştırılır.
- Sahne: 109 kontrol; örneğin kırpma küresinin her konum ve açıda modelin bütün köşelerini
  kapsadığı, yani görünen bir nesnenin asla kırpılmadığı sınanır. Her nesne çifti için
  çarpışmanın başladığı mesafe, çarpışma kodundan bağımsız bir yöntemle (iki şeklin içinde aynı
  anda bulunan bir nokta sık bir ızgarada aranarak) bulunur; iki sonuç 1 cm içinde örtüşür.
- Ekran testi: program gerçek OpenGL ile çalıştırılıp ekrandan piksel okunur. Kırpma açıkken ve
  kapalıyken görüntünün piksel piksel aynı çıkması, kırpmanın yalnızca görünmeyenleri attığını gösterir.
