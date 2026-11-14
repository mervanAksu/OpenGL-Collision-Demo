# collision_demo — Bilgisayar Grafigi Odev I

Sakarya Universitesi Bilgisayar Grafigi dersi icin OpenGL + C++ odevi: 5 FBX modelli,
carpisma denetimli, orbit kamerali 3B sahne. Odev metni: `docs/ODEV.md`.

## Degismez kurallar
- Teslim tek kaynak dosyasidir: `src/main.cpp` (+ `ufbx.h`, `ufbx.c`). Baska .cpp/.h eklenmez.
- Kutuphaneler: OpenGL 2.1, GLFW 3.3+, GLEW, GLM, ufbx. GLAD, GLUT, GLU, Assimp, stb KULLANILMAZ.
- Cizim tarzi derstekiyle ayni: fixed-function (glMatrixMode/glLoadMatrixf, glBegin/glEnd,
  glColor, glLight). Shader YOK. Mesh verisi icin VBO + glVertexPointer (GLEW bunun icin var).
- Matematik GLM ile yapilir, matris OpenGL'e glLoadMatrixf(glm::value_ptr(m)) ile verilir.
- Derleme: `make` (MSYS2 MinGW64, Linux, macOS). Makefile disinda derleme sistemi yok.
- Kod stili: `docs/STIL.md`. Arayuz sozlesmesi: `docs/MIMARI.md`. Ikisine uymayan parca kabul edilmez.

## Calisma duzeni (sef + alt ajanlar)
- Sef (ana oturum): mimariyi belirler, isi dagitir, parcalari `src/main.cpp` icinde birlestirir.
- Alt ajanlar `.claude/agents/` altinda tanimli. Her ajan SADECE kendi dosyasina yazar:
  `parcalar/*.inc` ve `test/*`.
- `src/main.cpp` ELLE DUZENLENMEZ: `parcalar/` altindaki dosyalardan `sh araclar/birlestir.sh`
  ile uretilir. Degisiklik ilgili .inc dosyasinda yapilir, sonra yeniden birlestirilir.
  Sahne ve ana dongu (`parcalar/sahne.inc`) sefin sorumlulugundadir.
- Bir is bitti sayilmasi icin: derleme kontrolu gecer, varsa testi gecer, denetci ajan rubrigi onaylar.

## Dogrulama komutlari
- Birlestir: `sh araclar/birlestir.sh`   - Birim testleri: `sh araclar/test.sh`   - Derle: `make`

## Teslim
Odev icin yalnizca `src/`, `models/`, `Makefile` ve `rapor.pdf` gonderilir. `.claude/`, `CLAUDE.md`,
`docs/`, `parcalar/`, `test/`, `araclar/` gelistirme dosyalaridir, teslim paketine girmez.
