# Kod stili

Kod, dersi alan bir ogrencinin kendi eliyle yazdigi tek dosyalik bir program gibi okunmali.
Dersin slaytlarindaki ornekler referans: kisa fonksiyonlar, Turkce adlar, duz C++.

- Dil: C++11. Sinif hiyerarsisi, template, lambda, akilli isaretci, namespace, `auto` yok.
  `struct` + icinde birkac uye fonksiyon yeterli. Odevin istedigi tip adlari aynen:
  `FbxModel`, `PhysObj`, `CollisionSystem`, `Camera`, `Frustum`.
- Degisken ve fonksiyon adlari Turkce, kucuk harf, alt cizgili: `yaricap`, `en_yakin_nokta`, `sahne_ciz`.
  Sabitler buyuk harf: `PI`, `DILIM`. Turkce karakter (s,g,u,o,c,i noktali/kuyruklu) KAYNAKTA KULLANILMAZ.
- Girinti 4 bosluk, suslu parantez ayni satirda. Satir ~100 karakteri gecmesin.
- Yorumlar az ve kisa, Turkce, `//` ile. Sadece "neden" veya formulun ne oldugu yazilir.
  YASAK: adim numarali yorumlar ("1) ...", "Adim 2"), bolum suslemeleri ("=====", "-----"),
  doxygen bloklari, her satiri aciklayan yorumlar, "yardimci fonksiyon" gibi bos etiketler.
- Hata kontrolu sadece gercekten olabilecek yerde (dosya acilamadi, pencere acilamadi). Mesaj `printf` ile.
- `float` kullan, sabitlerde `f` eki (0.5f). `std::vector` disinda STL'e gerek yok.
- Sihirli sayilar yerine dosya basinda birkac sabit; ama her seyi sabite cevirme.
- Gereksiz genelleme yok: kullanilmayan parametre, kullanilmayan fonksiyon, "ileride lazim olur" kodu yazilmaz.
