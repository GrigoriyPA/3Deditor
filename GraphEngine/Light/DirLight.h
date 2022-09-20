#pragma once


class DirLight : public Light {
    double shadow_width = 10, shadow_height = 10, shadow_depth = 10;

    Vect3 direction;
    Matrix projection;

    void set_projection_matrix() {
        projection = scale_matrix(Vect3(2.0 / shadow_width, 2.0 / shadow_height, 2.0 / shadow_depth)) * trans_matrix(Vect3(0, 0, -shadow_depth / 2));
    }

    Matrix get_view_matrix() {
        Vect3 horizont = direction.horizont();

        return Matrix(horizont, direction ^ horizont, direction).transpose() * trans_matrix(-shadow_position);
    }

public:
    Vect3 shadow_position = Vect3(0, 0, 0);

    DirLight(Vect3 direction) {
        if (direction.length() < eps) {
            std::cout << "ERROR::DIR_LIGHT::BUILDER\n" << "The direction vector has zero length.\n";
            assert(0);
        }

        this->direction = direction.normalize();

        set_projection_matrix();
    }

    void set_uniforms(int draw_id, Shader* shader_program) {
        if (draw_id < 0) {
            std::cout << "ERROR::DIR_LIGHT::SET_UNIFORMS\n" << "Invalid draw id.\n";
            assert(0);
        }

        try {
            std::string name = "lights[" + std::to_string(draw_id) + "].";
            glUniform1i(glGetUniformLocation(shader_program->program, (name + "shadow").c_str()), shadow);
            glUniform1i(glGetUniformLocation(shader_program->program, (name + "type").c_str()), 0);
            glUniform3f(glGetUniformLocation(shader_program->program, (name + "direction").c_str()), direction.x, direction.y, direction.z);
            glUniform3f(glGetUniformLocation(shader_program->program, (name + "ambient").c_str()), ambient.x, ambient.y, ambient.z);
            glUniform3f(glGetUniformLocation(shader_program->program, (name + "diffuse").c_str()), diffuse.x, diffuse.y, diffuse.z);
            glUniform3f(glGetUniformLocation(shader_program->program, (name + "specular").c_str()), specular.x, specular.y, specular.z);
            if (shadow)
                glUniformMatrix4fv(glGetUniformLocation(shader_program->program, (name + "light_space").c_str()), 1, GL_FALSE, &this->get_light_space_matrix().value_vector()[0]);
        }
        catch (const std::exception& error) {
            std::cout << "ERROR::DIR_LIGHT::SET_UNIFORMS\n" << "Unknown error, description:\n" << error.what() << "\n";
            assert(0);
        }
    }

    void set_shadow_width(double shadow_width) {
        if (shadow_width < eps) {
            std::cout << "ERROR::DIR_LIGHT::SET_SHADOW_WIDTH\n" << "Not a positive shadow width.\n";
            assert(0);
        }

        this->shadow_width = shadow_width;
        set_projection_matrix();
    }

    void set_shadow_height(double shadow_height) {
        if (shadow_height < eps) {
            std::cout << "ERROR::DIR_LIGHT::SET_SHADOW_HEIGHT\n" << "Not a positive shadow height.\n";
            assert(0);
        }

        this->shadow_height = shadow_height;
        set_projection_matrix();
    }

    void set_shadow_depth(double shadow_depth) {
        if (shadow_depth < eps) {
            std::cout << "ERROR::DIR_LIGHT::SET_SHADOW_DEPTH\n" << "Not a positive shadow depth.\n";
            assert(0);
        }

        this->shadow_depth = shadow_depth;
        set_projection_matrix();
    }

    Matrix get_light_space_matrix() {
        return projection * get_view_matrix();
    }

    GraphObject get_shadow_box() {
        GraphObject shadow_box = get_cube();
        shadow_box.transparent = true;

        Material material;
        material.diffuse = Vect3(1, 1, 1);
        material.alpha = 0.3;
        shadow_box.set_material(material);

        int model_id = shadow_box.add_model(scale_matrix(Vect3(shadow_width, shadow_height, shadow_depth)));
        shadow_box.change_matrix(trans_matrix(Vect3(0, 0, (1 - eps) * shadow_depth / 2)), model_id);
        shadow_box.change_matrix(get_view_matrix().inverse(), model_id);

        return shadow_box;
    }
};
