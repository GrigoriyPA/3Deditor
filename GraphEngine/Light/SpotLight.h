#pragma once

#include "Light.h"
#include "../GraphObjects/GraphObject.h"


namespace gre {
    class SpotLight : public Light {
        static const uint8_t LIGHT_TYPE = 2;

        double shadow_min_distance_ = 1.0;
        double shadow_max_distance_ = 10.0;
        double constant_ = 1.0;
        double linear_ = 0.0;
        double quadratic_ = 0.0;

        double border_in_;
        double border_out_;
        Vec3 direction_;
        Matrix projection_;

        void set_projection_matrix() {
            if (equality(tan(border_out_), 0.0) || equality(shadow_max_distance_, shadow_min_distance_) || equality(shadow_max_distance_ + shadow_min_distance_, 0.0)) {
                throw GreDomainError(__FILE__, __LINE__, "set_projection_matrix, invalid matrix settings.\n\n");
            }

            projection_ = Matrix::scale_matrix(Vec3(1.0 / tan(border_out_), 1.0 / tan(border_out_), (shadow_max_distance_ + shadow_min_distance_) / (shadow_max_distance_ - shadow_min_distance_)));
            projection_ *= Matrix::translation_matrix(Vec3(0.0, 0.0, -2.0 * shadow_max_distance_ * shadow_min_distance_ / (shadow_max_distance_ + shadow_min_distance_)));
            projection_[3][3] = 0.0;
            projection_[3][2] = 1.0;
        }

        Matrix get_view_matrix() const noexcept {
            const Vec3& horizont = direction_.horizont();
            return Matrix(horizont, direction_ ^ horizont, direction_).transpose() * Matrix::translation_matrix(-position);
        }

    public:
        Vec3 position;

        SpotLight(const Vec3& position, const Vec3& direction, double border_in, double border_out) : projection_(4, 4) {
            if (!glew_is_ok()) {
                throw GreRuntimeError(__FILE__, __LINE__, "SpotLight, failed to initialize GLEW.\n\n");
            }
            if (border_in < 0.0 || less_equality(PI / 2.0, border_out) || less_equality(border_out, border_in)) {
                throw GreInvalidArgument(__FILE__, __LINE__, "SpotLight, invalid values of the external and internal angles of the spotlight.\n\n");
            }

            try {
                direction_ = direction.normalize();
            }
            catch (GreDomainError) {
                throw GreInvalidArgument(__FILE__, __LINE__, "SpotLight, the direction vector has zero length.\n\n");
            }

            border_in_ = border_in;
            border_out_ = border_out;
            this->position = position;

            set_projection_matrix();
        }

        void set_uniforms(size_t id, const Shader<size_t>& shader) const override {
            if (shader.description != ShaderType::MAIN) {
                throw GreInvalidArgument(__FILE__, __LINE__, "set_uniforms, invalid shader type.\n\n");
            }

            std::string name = "lights[" + std::to_string(id) + "].";
            set_light_uniforms(name, shader);

            shader.set_uniform_i((name + "type").c_str(), LIGHT_TYPE);
            shader.set_uniform_f((name + "constant").c_str(), static_cast<GLfloat>(constant_));
            shader.set_uniform_f((name + "linear").c_str(), static_cast<GLfloat>(linear_));
            shader.set_uniform_f((name + "quadratic").c_str(), static_cast<GLfloat>(quadratic_));
            shader.set_uniform_f((name + "cut_in").c_str(), static_cast<GLfloat>(cos(border_in_)));
            shader.set_uniform_f((name + "cut_out").c_str(), static_cast<GLfloat>(cos(border_out_)));
            shader.set_uniform_f((name + "direction").c_str(), direction_);
            shader.set_uniform_f((name + "position").c_str(), position);
            if (shadow) {
                shader.set_uniform_matrix((name + "light_space").c_str(), get_light_space_matrix());
            }
        }

        SpotLight& set_shadow_distance(double shadow_min_distance, double shadow_max_distance) {
            if (less_equality(shadow_min_distance, 0.0) || less_equality(shadow_max_distance, shadow_min_distance)) {
                throw GreInvalidArgument(__FILE__, __LINE__, "set_shadow_distance, invalid shadow distance.\n\n");
            }

            shadow_min_distance_ = shadow_min_distance;
            shadow_max_distance_ = shadow_max_distance;
            set_projection_matrix();
            return *this;
        }

        SpotLight& set_constant(double coefficient) {
            if (coefficient < 0.0) {
                throw GreInvalidArgument(__FILE__, __LINE__, "set_constant, negative coefficient value.\n\n");
            }

            constant_ = coefficient;
            return *this;
        }

        SpotLight& set_linear(double coefficient) {
            if (coefficient < 0.0) {
                throw GreInvalidArgument(__FILE__, __LINE__, "set_linear, negative coefficient value.\n\n");
            }

            linear_ = coefficient;
            return *this;
        }

        SpotLight& set_quadratic(double coefficient) {
            if (coefficient < 0.0) {
                throw GreInvalidArgument(__FILE__, __LINE__, "set_quadratic, negative coefficient value.\n\n");
            }

            quadratic_ = coefficient;
            return *this;
        }

        SpotLight& set_angle(double border_in, double border_out) {
            if (border_in < 0.0 || less_equality(PI / 2.0, border_out) || less_equality(border_out, border_in)) {
                throw GreInvalidArgument(__FILE__, __LINE__, "set_angle, invalid values of the external and internal angles of the spotlight.\n\n");
            }

            border_in_ = border_in;
            border_out_ = border_out;
            return *this;
        }

        SpotLight& set_direction(const Vec3& direction) {
            try {
                direction_ = direction.normalize();
            }
            catch (GreDomainError) {
                throw GreInvalidArgument(__FILE__, __LINE__, "set_direction, the direction vector has zero length.\n\n");
            }
            return *this;
        }

        Matrix get_light_space_matrix() const noexcept override {
            return projection_ * get_view_matrix();
        }

        GraphObject get_shadow_box() const {
            if (equality(shadow_max_distance_, 0.0)) {
                throw GreDomainError(__FILE__, __LINE__, "get_shadow_box, invalid matrix settings.\n\n");
            }

            GraphObject shadow_box(1);
            shadow_box.transparent = true;

            Mesh mesh(4);
            mesh.set_positions({
                Vec3(1.0, 1.0, 1.0),
                Vec3(1.0, -1.0, 1.0),
                Vec3(-1.0, -1.0, 1.0),
                Vec3(-1.0, 1.0, 1.0)
            }, true);
            shadow_box.meshes.insert(mesh);

            double delt = shadow_min_distance_ / shadow_max_distance_;
            mesh.apply_matrix(Matrix::scale_matrix(delt));
            mesh.invert_points_order(true);
            shadow_box.meshes.insert(mesh);

            mesh = Mesh(4);
            mesh.set_positions({
                Vec3(1.0, -1.0, 1.0),
                Vec3(1.0, 1.0, 1.0),
                Vec3(delt, delt, delt),
                Vec3(delt, -delt, delt)
            }, true);
            shadow_box.meshes.insert(mesh);

            mesh.apply_matrix(Matrix::rotation_matrix(Vec3(0.0, 0.0, 1.0), PI / 2.0));
            shadow_box.meshes.insert(mesh);

            mesh.apply_matrix(Matrix::rotation_matrix(Vec3(0.0, 0.0, 1.0), PI / 2.0));
            shadow_box.meshes.insert(mesh);

            mesh.apply_matrix(Matrix::rotation_matrix(Vec3(0.0, 0.0, 1.0), PI / 2.0));
            shadow_box.meshes.insert(mesh);

            shadow_box.meshes.apply_func([](auto& mesh) {
                mesh.material.set_diffuse(Vec3(1.0, 1.0, 1.0));
                mesh.material.set_alpha(0.3);
            });

            Matrix model = Matrix::scale_matrix((1.0 - EPS) * shadow_max_distance_ * Vec3(tan(border_out_), tan(border_out_), 1.0));
            model = get_view_matrix().inverse() * model;

            shadow_box.models.insert(model);
            return shadow_box;
        }

        GraphObject get_light_object() const {
            GraphObject light_object = GraphObject::cone(20, true, 1);

            light_object.meshes.apply_func([](auto& mesh) {
                mesh.material.set_emission(Vec3(1.0, 1.0, 1.0));
                mesh.material.shadow = false;
            });

            Matrix model = Matrix::scale_matrix(0.25 * Vec3(tan(border_out_), tan(border_out_), 1.0));
            model = Matrix::rotation_matrix(Vec3(1.0, 0.0, 0.0), -PI / 2.0) * model;
            model = get_view_matrix().inverse() * model;

            light_object.models.insert(model);
            return light_object;
        }
    };
}
