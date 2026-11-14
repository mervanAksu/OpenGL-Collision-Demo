const int KOSE_FLOAT = 9;   // konum(3) + normal(3) + renk(3)

glm::vec3 materyal_rengi(const ufbx_material* m) {
    if (m == NULL) return glm::vec3(0.8f, 0.8f, 0.8f);
    ufbx_vec3 c;
    if (m->pbr.base_color.has_value) c = m->pbr.base_color.value_vec3;
    else if (m->fbx.diffuse_color.has_value) c = m->fbx.diffuse_color.value_vec3;
    else return glm::vec3(0.8f, 0.8f, 0.8f);
    return glm::vec3((float)c.x, (float)c.y, (float)c.z);
}

// GL kullanmadan dosyayi okur, ucgenleri tek diziye toplar ve olcekler
bool fbx_oku(const char* yol, float hedef_boy, std::vector<float>& veri,
             glm::vec3& kmin, glm::vec3& kmax, glm::vec3& renk) {
    ufbx_load_opts ayar = {};
    ayar.target_axes = ufbx_axes_right_handed_y_up;
    ayar.target_unit_meters = 1.0f;
    ayar.generate_missing_normals = true;
    ufbx_error hata;
    ufbx_scene* sahne = ufbx_load_file(yol, &ayar, &hata);
    if (sahne == NULL) {
        printf("FBX acilamadi: %s (%s)\n", yol, hata.description.data);
        return false;
    }

    renk = glm::vec3(0.8f, 0.8f, 0.8f);
    if (sahne->materials.count > 0) renk = materyal_rengi(sahne->materials.data[0]);

    veri.clear();
    std::vector<uint32_t> ucgen;
    for (size_t i = 0; i < sahne->meshes.count; i++) {
        ufbx_mesh* mesh = sahne->meshes.data[i];
        ucgen.resize(mesh->max_face_triangles * 3);
        // ayni mesh birden fazla node'da kullanilmis olabilir
        for (size_t j = 0; j < mesh->instances.count; j++) {
            ufbx_node* node = mesh->instances.data[j];
            ufbx_matrix normal_mat = ufbx_matrix_for_normals(&node->geometry_to_world);
            for (size_t f = 0; f < mesh->num_faces; f++) {
                glm::vec3 yuz_rengi = renk;
                if (mesh->face_material.count > 0) {
                    uint32_t mi = mesh->face_material.data[f];
                    if (mi < node->materials.count)
                        yuz_rengi = materyal_rengi(node->materials.data[mi]);
                }
                uint32_t n = ufbx_triangulate_face(ucgen.data(), ucgen.size(), mesh,
                                                   mesh->faces.data[f]);
                for (uint32_t k = 0; k < n * 3; k++) {
                    ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, ucgen[k]);
                    ufbx_vec3 nr = ufbx_get_vertex_vec3(&mesh->vertex_normal, ucgen[k]);
                    p = ufbx_transform_position(&node->geometry_to_world, p);
                    nr = ufbx_transform_direction(&normal_mat, nr);
                    glm::vec3 nn((float)nr.x, (float)nr.y, (float)nr.z);
                    if (glm::length(nn) > 0.0f) nn = glm::normalize(nn);
                    float kose[KOSE_FLOAT] = { (float)p.x, (float)p.y, (float)p.z, nn.x, nn.y, nn.z,
                                               yuz_rengi.r, yuz_rengi.g, yuz_rengi.b };
                    veri.insert(veri.end(), kose, kose + KOSE_FLOAT);
                }
            }
        }
    }
    ufbx_free_scene(sahne);
    if (veri.empty()) {
        printf("FBX icinde mesh yok: %s\n", yol);
        return false;
    }

    kmin = kmax = glm::vec3(veri[0], veri[1], veri[2]);
    for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT) {
        glm::vec3 p(veri[i], veri[i + 1], veri[i + 2]);
        kmin = glm::min(kmin, p);
        kmax = glm::max(kmax, p);
    }
    glm::vec3 boyut = kmax - kmin;
    float en_uzun = glm::max(boyut.x, glm::max(boyut.y, boyut.z));
    float olcek = en_uzun > 0.0f ? hedef_boy / en_uzun : 1.0f;
    // taban y=0'a, x ve z merkeze gelsin
    glm::vec3 taban((kmin.x + kmax.x) * 0.5f, kmin.y, (kmin.z + kmax.z) * 0.5f);
    for (size_t i = 0; i < veri.size(); i += KOSE_FLOAT) {
        for (int e = 0; e < 3; e++) veri[i + e] = (veri[i + e] - taban[e]) * olcek;
    }
    kmin = (kmin - taban) * olcek;
    kmax = (kmax - taban) * olcek;
    return true;
}

struct FbxModel {
    GLuint vbo;              // pos(3)+normal(3)+renk(3) float, ucgen listesi, index yok
    int kose_sayisi;
    glm::vec3 renk;          // ilk materyalin rengi, yoksa acik gri
    glm::vec3 kutu_min, kutu_max;

    bool yukle(const char* yol, float hedef_boy) {
        vbo = 0;
        kose_sayisi = 0;
        std::vector<float> veri;
        if (!fbx_oku(yol, hedef_boy, veri, kutu_min, kutu_max, renk)) return false;
        kose_sayisi = (int)(veri.size() / KOSE_FLOAT);
#ifndef TEST_DERLEME
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, veri.size() * sizeof(float), veri.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
        return true;
    }

    void ciz() const {
#ifndef TEST_DERLEME
        if (vbo == 0) return;
        // cagiranin cizgi cizimi bozulmasin diye durumu saklayip geri aliyoruz
        glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        GLsizei adim = (GLsizei)(KOSE_FLOAT * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, adim, (void*)0);
        glNormalPointer(GL_FLOAT, adim, (void*)(3 * sizeof(float)));
        glColorPointer(3, GL_FLOAT, adim, (void*)(6 * sizeof(float)));
        glDrawArrays(GL_TRIANGLES, 0, kose_sayisi);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glPopClientAttrib();
        glPopAttrib();
#endif
    }

    void bosalt() {
#ifndef TEST_DERLEME
        if (vbo != 0) glDeleteBuffers(1, &vbo);
#endif
        vbo = 0;
        kose_sayisi = 0;
    }
};
