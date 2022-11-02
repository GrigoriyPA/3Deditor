#pragma once


class SpotLight : public Light {
    double shadow_min_distance = 1, shadow_max_distance = 10;

    double cut_in, cut_out;
    eng::Vect3 direction;
    eng::Matrix projection;

    void set_projection_matrix() {
        projection = eng::Matrix::scale_matrix(eng::Vect3(1.0 / tan(cut_out), 1.0 / tan(cut_out), (shadow_max_distance + shadow_min_distance) / (shadow_max_distance - shadow_min_distance)));
        projection *= eng::Matrix::translation_matrix(eng::Vect3(0, 0, -2 * shadow_min_distance * shadow_max_distance / (shadow_min_distance + shadow_max_distance)));
        projection[3][3] = 0;
        projection[3][2] = 1;
    }

    eng::Matrix get_view_matrix() {
        eng::Vect3 horizont = direction.horizont();

        return eng::Matrix(horizont, direction ^ horizont, direction).transpose() * eng::Matrix::translation_matrix(-position);
    }

public:
    double constant = 1, linear = 0, quadratic = 0;

    eng::Vect3 position;

    SpotLight(eng::Vect3 position, eng::Vect3 direction, double cut_in, double cut_out) : projection(eng::Matrix::one_matrix(4)) {
        if (direction.length() < eps) {
            std::cout << "ERROR::SPOT_LIGHT::BUILDER\n" << "The direction vector has zero length.\n";
            assert(0);
        }

        if (cut_in < eps || cut_in > cut_out || cut_out >= eng::PI / 2 - eps || abs(cut_in - cut_out) < eps) {
            std::cout << "ERROR::SPOT_LIGHT::BUILDER\n" << "Invalid values of the external and internal angles of the spotlight.\n";
            assert(0);
        }

        this->position = position;
        this->direction = direction.normalize();
        this->cut_in = cut_in;
        this->cut_out = cut_out;

        set_projection_matrix();
    }

    void set_uniforms(int draw_id, eng::Shader<size_t>* shader_program) {
        if (draw_id < 0) {
            std::cout << "ERROR::SPOT_LIGHT::SET_UNIFORMS\n" << "Invalid draw id.\n";
            assert(0);
        }

        try {
            std::string name = "lights[" + std::to_string(draw_id) + "].";
            shader_program->set_uniform_i((name + "shadow").c_str(), shadow);
            shader_program->set_uniform_i((name + "type").c_str(), 2);
            shader_program->set_uniform_f((name + "position").c_str(), position.x, position.y, position.z);
            shader_program->set_uniform_f((name + "direction").c_str(), direction.x, direction.y, direction.z);
            shader_program->set_uniform_f((name + "cut_in").c_str(), cos(cut_in));
            shader_program->set_uniform_f((name + "cut_out").c_str(), cos(cut_out));
            shader_program->set_uniform_f((name + "constant").c_str(), constant);
            shader_program->set_uniform_f((name + "linear").c_str(), linear);
            shader_program->set_uniform_f((name + "quadratic").c_str(), quadratic);
            shader_program->set_uniform_f((name + "ambient").c_str(), ambient.x, ambient.y, ambient.z);
            shader_program->set_uniform_f((name + "diffuse").c_str(), diffuse.x, diffuse.y, diffuse.z);
            shader_program->set_uniform_f((name + "specular").c_str(), specular.x, specular.y, specular.z);
            if (shadow)
                shader_program->set_uniform_matrix((name + "light_space").c_str(), this->get_light_space_matrix());
        }
        catch (const std::exception& error) {
            std::cout << "ERROR::SPOT_LIGHT::SET_UNIFORMS\n" << "Unknown error, description:\n" << error.what() << "\n";
            assert(0);
        }
    }

    void set_shadow_distance(double shadow_min_distance, double shadow_max_distance) {
        if (shadow_min_distance < eps || shadow_min_distance > shadow_max_distance || abs(shadow_min_distance - shadow_max_distance) < eps) {
            std::cout << "ERROR::SPOT_LIGHT::SET_SHADOW_DISTANCE\n" << "Invalid shadow distance.\n";
            assert(0);
        }

        this->shadow_min_distance = shadow_min_distance;
        this->shadow_max_distance = shadow_max_distance;
        set_projection_matrix();
    }

    void set_direction(eng::Vect3 direction) {
        if (direction.length() < eps) {
            std::cout << "ERROR::SPOT_LIGHT::SET_DIRECTION\n" << "The direction vector has zero length.\n";
            assert(0);
        }

        this->direction = direction.normalize();
    }

    eng::Matrix get_light_space_matrix() {
        return projection * get_view_matrix();
    }

    eng::GraphObject get_shadow_box() {
        double delt = shadow_min_distance / shadow_max_distance;

        eng::GraphObject shadow_box(1);
        shadow_box.transparent = true;

        eng::Mesh mesh(4);
        mesh.set_positions({
        eng::Vect3(1, 1, 1),
        eng::Vect3(1, -1, 1),
        eng::Vect3(-1, -1, 1),
        eng::Vect3(-1, 1, 1)
        });
        mesh.material.set_diffuse(eng::Vect3(1, 1, 1));
        mesh.material.set_alpha(0.3);
        shadow_box.meshes.insert(mesh);

        mesh.apply_matrix(eng::Matrix::scale_matrix(delt));
        mesh.invert_points_order();
        shadow_box.meshes.insert(mesh);

        mesh = eng::Mesh(4);
        mesh.set_positions({
        eng::Vect3(1, -1, 1),
        eng::Vect3(1, 1, 1),
        eng::Vect3(delt, delt, delt),
        eng::Vect3(delt, -delt, delt)
        });
        mesh.material.set_diffuse(eng::Vect3(1, 1, 1));
        mesh.material.set_alpha(0.3);
        shadow_box.meshes.insert(mesh);

        mesh.apply_matrix(eng::Matrix::rotation_matrix(eng::Vect3(0, 0, 1), eng::PI / 2));
        shadow_box.meshes.insert(mesh);

        mesh.apply_matrix(eng::Matrix::rotation_matrix(eng::Vect3(0, 0, 1), eng::PI / 2));
        shadow_box.meshes.insert(mesh);

        mesh.apply_matrix(eng::Matrix::rotation_matrix(eng::Vect3(0, 0, 1), eng::PI / 2));
        shadow_box.meshes.insert(mesh);

        int model_id = shadow_box.models.insert(eng::Matrix::scale_matrix((1 - eps) * shadow_max_distance * eng::Vect3(tan(cut_out), tan(cut_out), 1)));
        shadow_box.models.change_left(model_id, get_view_matrix().inverse());

        return shadow_box;
    }

    eng::GraphObject get_light_object() {
        eng::GraphObject light_object(1);

        eng::Mesh mesh(4);
        mesh.set_positions({
        eng::Vect3(1, 1, 1),
        eng::Vect3(1, -1, 1),
        eng::Vect3(-1, -1, 1),
        eng::Vect3(-1, 1, 1)
        });
        mesh.material.set_emission(eng::Vect3(1, 1, 1));
        mesh.material.shadow = true;
        light_object.meshes.insert(mesh);

        mesh = eng::Mesh(3);
        mesh.set_positions({
        eng::Vect3(1, -1, 1),
        eng::Vect3(1, 1, 1),
        eng::Vect3(0, 0, 0)
        });
        mesh.material.set_emission(eng::Vect3(1, 1, 1));
        mesh.material.shadow = true;
        light_object.meshes.insert(mesh);

        mesh.apply_matrix(eng::Matrix::rotation_matrix(eng::Vect3(0, 0, 1), eng::PI / 2));
        light_object.meshes.insert(mesh);

        mesh.apply_matrix(eng::Matrix::rotation_matrix(eng::Vect3(0, 0, 1), eng::PI / 2));
        light_object.meshes.insert(mesh);

        mesh.apply_matrix(eng::Matrix::rotation_matrix(eng::Vect3(0, 0, 1), eng::PI / 2));
        light_object.meshes.insert(mesh);

        int model_id = light_object.models.insert(eng::Matrix::scale_matrix(0.25 * eng::Vect3(tan(cut_out), tan(cut_out), 1)));
        light_object.models.change_left(model_id, get_view_matrix().inverse());

        return light_object;
    }
};
