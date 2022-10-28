#pragma once

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <unordered_map>
#include "Mesh.h"


namespace eng {
	struct Model {
		bool border;
		int used_memory;
		eng::Matrix matrix;

		eng::Model(eng::Matrix matrix = eng::Matrix::one_matrix(4), bool border = false, int used_memory = -1) : matrix(matrix) {
			this->matrix = matrix;
			this->border = border;
			this->used_memory = used_memory;
		}
	};


	class GraphObject {
		int count_borders = 0, border_bit = 1;
		double eps = 0.00001;
		eng::Vect3 center = eng::Vect3(0, 0, 0);

		int max_count_models;
		unsigned int matrix_buffer;
		std::vector < int > used_memory;
		std::unordered_map < int, eng::Model > models;
		std::unordered_map < int, eng::Mesh > polygons;

		template <size_t T>
		void set_uniforms(eng::Shader<T>* shader_program, int object_id) {
			try {
				shader_program->use();
				shader_program->set_uniform_i("object_id", object_id);
			}
			catch (const std::exception& error) {
				//std::cout << "ERROR::GRAPH_OBJECT::SET_UNIFORMS\n" << "Unknown error, description:\n" << error.what() << "\n";
				//assert(0);
			}
		}

		void create_matrix_buffer() {
			glGenBuffers(1, &matrix_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16 * max_count_models, NULL, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			for (std::unordered_map < int, eng::Mesh >::iterator polygon = polygons.begin(); polygon != polygons.end(); polygon++)
				polygon->second.set_matrix_buffer(matrix_buffer);
		}

		void delete_buffers() {
			glDeleteBuffers(1, &matrix_buffer);
		}

		template <size_t T>
		void draw_polygons(eng::Shader<T>* shader_program, int model_id = -1) {
			if (model_id != -1 && !models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::DRAW_POLYGONS\n" << "Invalid model id.\n";
				//assert(0);
			}

			int cnt = models.size();
			if (model_id != -1) {
				shader_program->set_uniform_matrix("not_instance_model", models[model_id].matrix);
				cnt = 1;
			}

			int cur_id = -1;
			if (model_id != -1)
				cur_id = models[model_id].used_memory;
			shader_program->set_uniform_i("model_id", cur_id);

			for (std::unordered_map < int, eng::Mesh >::iterator polygon = polygons.begin(); polygon != polygons.end(); polygon++)
				polygon->second.draw(cnt, *shader_program);
		}

		std::vector<eng::Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, const aiScene* scene, std::string& directory) {
			std::vector<eng::Texture> textures;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
				aiString str;
				material->GetTexture(type, i, &str);

				std::string path(str.data);
				if (path[0] == '*') {
					path.erase(path.begin());
					path = "inline_texture" + path + "." + std::string(scene->mTextures[std::stoi(path)]->achFormatHint);
				}

				eng::Texture texture(directory + "/" + path, true);
				textures.push_back(texture);
			}
			return textures;
		}

		eng::Mesh process_mesh(aiMesh* mesh, const aiScene* scene, std::string& directory, eng::Matrix transform) {
			std::vector < eng::Vect2 > tex_coords;
			std::vector < eng::Vect3 > positions, normals;
			std::vector < eng::Vect3 > colors;
			std::vector<unsigned int> indices;

			eng::Matrix norm_transform = transform.inverse().transpose();
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				positions.push_back(transform * eng::Vect3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
				if (mesh->mNormals)
					normals.push_back(norm_transform * eng::Vect3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
				if (mesh->mTextureCoords[0])
					tex_coords.push_back(eng::Vect2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y));
				else
					tex_coords.push_back(eng::Vect2(0, 0));
				if (mesh->mColors[0])
					colors.push_back(eng::Vect3(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b));
				else
					colors.push_back(eng::Vect3(0, 0, 0));
			}
			eng::Mesh polygon_mesh(positions.size());
			polygon_mesh.set_positions(positions, normals.empty());
			if (!normals.empty())
				polygon_mesh.set_normals(normals);
			polygon_mesh.set_tex_coords(tex_coords);
			polygon_mesh.set_colors(colors);

			polygon_mesh.material.set_diffuse(eng::Vect3(1, 1, 1));
			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++)
					indices.push_back(face.mIndices[j]);
			}
			polygon_mesh.set_indices(indices);

			if (mesh->mMaterialIndex >= 0) {
				aiColor3D color(0, 0, 0);
				aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

				std::vector<eng::Texture> diffuse_textures = loadMaterialTextures(material, aiTextureType_DIFFUSE, scene, directory);
				if (!diffuse_textures.empty())
					polygon_mesh.material.diffuse_map = diffuse_textures[0];
				//std::cout << material->GetTextureCount(aiTextureType_DIFFUSE) << "\n";

				std::vector<eng::Texture> specular_textures = loadMaterialTextures(material, aiTextureType_SPECULAR, scene, directory);
				if (!specular_textures.empty())
					polygon_mesh.material.specular_map = specular_textures[0];
				//std::cout << material->GetTextureCount(aiTextureType_SPECULAR) << "\n";

				std::vector<eng::Texture> emissive_textures = loadMaterialTextures(material, aiTextureType_EMISSIVE, scene, directory);
				if (!emissive_textures.empty())
					polygon_mesh.material.emission_map = emissive_textures[0];
				//std::cout << material->GetTextureCount(aiTextureType_EMISSIVE) << "\n";

				material->Get(AI_MATKEY_COLOR_AMBIENT, color);
				polygon_mesh.material.set_ambient(eng::Vect3(color.r, color.g, color.b));
				//polygon_mesh.material.ambient.print();

				material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
				polygon_mesh.material.set_diffuse(eng::Vect3(color.r, color.g, color.b));
				//polygon_mesh.material.diffuse.print();

				material->Get(AI_MATKEY_COLOR_SPECULAR, color);
				polygon_mesh.material.set_specular(eng::Vect3(color.r, color.g, color.b));
				//polygon_mesh.material.specular.print();

				material->Get(AI_MATKEY_COLOR_EMISSIVE, color);
				polygon_mesh.material.set_emission(eng::Vect3(color.r, color.g, color.b));
				//polygon_mesh.material.emission.print();

				float opacity;
				material->Get(AI_MATKEY_OPACITY, opacity);
				polygon_mesh.material.set_alpha(opacity);
				//std::cout << polygon_mesh.material.alpha << "\n";

				float shininess;
				material->Get(AI_MATKEY_SHININESS, shininess);
				polygon_mesh.material.set_shininess(shininess);
				//std::cout << polygon_mesh.material.shininess << "\n";

				//if (polygon_mesh.material.shininess > 0)
				//	polygon_mesh.material.specular = eng::Vect3(1, 1, 1);
			}

			return polygon_mesh;
		}

		void process_node(aiNode* node, const aiScene* scene, std::string& directory, eng::Matrix transform) {
			eng::Matrix trans(4, 4, 0);
			aiMatrix4x4 cur_transform = node->mTransformation;
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++)
					trans[i][j] = cur_transform[i][j];
			}
			transform = trans * transform;

			for (int i = 0; i < node->mNumMeshes; i++) {
				polygons[polygons.size()] = process_mesh(scene->mMeshes[node->mMeshes[i]], scene, directory, transform);
				polygons[polygons.size() - 1].set_matrix_buffer(matrix_buffer);
				break;
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++) {
				process_node(node->mChildren[i], scene, directory, transform);
			}
		}

	public:
		bool transparent = false;

		eng::GraphObject(const eng::GraphObject& object) {
			*this = object;
		}

		eng::GraphObject(int max_count_models = 0) {
			if (max_count_models < 0) {
				//std::cout << "ERROR::GRAPH_OBJECT::BUILDER\n" << "Negative number of models.\n";
				//assert(0);
			}

			this->max_count_models = max_count_models;
			used_memory.resize(max_count_models, -1);
			matrix_buffer = 0;

			create_matrix_buffer();
		}

		eng::GraphObject& operator=(const eng::GraphObject& other) {
			delete_buffers();

			count_borders = other.count_borders;
			border_bit = other.border_bit;
			eps = other.eps;
			center = other.center;
			max_count_models = other.max_count_models;
			used_memory = other.used_memory;
			models = other.models;
			polygons = other.polygons;
			transparent = other.transparent;

			create_matrix_buffer();
			set_center();

			glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);
			glBindBuffer(GL_COPY_READ_BUFFER, other.matrix_buffer);
			glBindBuffer(GL_COPY_WRITE_BUFFER, matrix_buffer);

			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(float) * 16 * max_count_models);

			glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
			glBindBuffer(GL_COPY_READ_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			return *this;
		}

		eng::Mesh& operator[](int polygon_id) {
			if (!polygons.count(polygon_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::OPERATOR[]\n" << "Invalid polygon id.\n";
				//assert(0);
			}

			return polygons[polygon_id];
		}

		void set_center() {
			center = eng::Vect3(0, 0, 0);
			std::vector < eng::Vect3 > used_positions;
			for (std::unordered_map < int, eng::Mesh >::iterator polygon = polygons.begin(); polygon != polygons.end(); polygon++) {
				for (eng::Vect3 position : polygon->second.get_positions()) {
					if (std::count(used_positions.begin(), used_positions.end(), position))
						continue;

					used_positions.push_back(position);
					center += position;
				}
			}
			if (used_positions.size() > 0)
				center /= used_positions.size();
		}

		void set_material(eng::Mesh::Material material) {
			for (std::unordered_map < int, eng::Mesh >::iterator polygon = polygons.begin(); polygon != polygons.end(); polygon++)
				polygon->second.material = material;
		}

		void set_matrix(eng::Matrix trans, int model_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::SET_MATRIX\n" << "Invalid model id.\n";
				//assert(0);
			}

			models[model_id].matrix = trans;

			glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 16 * models[model_id].used_memory, sizeof(float) * 16, &std::vector<float>(models[model_id].matrix)[0]);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		void set_border(bool border, int model_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::SET_BORDER\n" << "Invalid model id.\n";
				//assert(0);
			}

			count_borders += int(border) - int(models[model_id].border);
			models[model_id].border = border;
		}

		void set_border_bit(int bit_id) {
			if (bit_id < 0 || 8 <= bit_id) {
				//std::cout << "ERROR::GRAPH_OBJECT::SET_BORDER_BIT\n" << "Invalid bit id.\n";
				//assert(0);
			}

			border_bit = 1 << bit_id;
		}

		std::vector < std::pair < int, int > > get_models() {
			std::vector < std::pair < int, int > > models_description;
			for (std::unordered_map < int, eng::Model >::iterator model = models.begin(); model != models.end(); model++) {
				for (std::unordered_map < int, eng::Mesh >::iterator polygon = polygons.begin(); polygon != polygons.end(); polygon++)
					models_description.push_back({ model->first, polygon->first });
			}

			return models_description;
		}

		eng::Vect3 get_center(int model_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::GET_CENTER\n" << "Invalid model id.\n";
				//assert(0);
			}

			return models[model_id].matrix * center;
		}

		eng::Vect3 get_polygon_center(int model_id, int polygon_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::GET_POLYGON_CENTER\n" << "Invalid model id.\n";
				//assert(0);
			}

			if (!polygons.count(polygon_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::GET_POLYGON_CENTER\n" << "Invalid polygon id.\n";
				//assert(0);
			}

			return models[model_id].matrix * polygons[polygon_id].get_center();
		}

		std::vector < eng::Vect3 > get_polygon_positions(int model_id, int polygon_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::GET_POLYGON_CENTER\n" << "Invalid model id.\n";
				//assert(0);
			}

			if (!polygons.count(polygon_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::GET_POLYGON_CENTER\n" << "Invalid polygon id.\n";
				//assert(0);
			}

			std::vector < eng::Vect3 > positions;
			for (eng::Vect3 position : polygons[polygon_id].get_positions())
				positions.push_back(models[model_id].matrix * position);

			return positions;
		}

		eng::Matrix get_matrix(int model_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::GET_MATRIX\n" << "Invalid model id.\n";
				//assert(0);
			}

			return models[model_id].matrix;
		}

		int get_count_polygons() {
			return polygons.size();
		}

		int get_count_models() {
			return models.size();
		}

		int get_model_id(int memory_id) {
			if (memory_id == -1)
				return -1;

			if (memory_id < 0 || models.size() <= memory_id) {
				//std::cout << "ERROR::GRAPH_OBJECT::GET_MODEL_ID\n" << "Invalid memory id.\n";
				//assert(0);
			}

			return used_memory[memory_id];
		}

		int add_polygon(eng::Mesh polygon) {
			int free_polygon_id = 0;
			for (; polygons.count(free_polygon_id); free_polygon_id++) {}

			polygons[free_polygon_id] = polygon;
			polygons[free_polygon_id].set_matrix_buffer(matrix_buffer);

			set_center();

			return free_polygon_id;
		}

		int add_model(eng::Matrix matrix = eng::Matrix::one_matrix(4)) {
			if (models.size() == max_count_models) {
				//std::cout << "ERROR::GRAPH_OBJECT::ADD_MATRYX\n" << "Too many instances created.\n";
				//assert(0);
			}

			int free_model_id = 0;
			for (; models.count(free_model_id); free_model_id++) {}

			used_memory[models.size()] = free_model_id;
			models[free_model_id] = eng::Model(matrix, false, models.size());

			glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 16 * models[free_model_id].used_memory, sizeof(float) * 16, &std::vector<float>(models[free_model_id].matrix)[0]);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			return free_model_id;
		}

		void delete_model(int model_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::DELETE_MODEL\n" << "Invalid model id.\n";
				//assert(0);
			}

			glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);
			for (int i = models[model_id].used_memory + 1; i < models.size(); i++) {
				used_memory[i - 1] = used_memory[i];
				models[used_memory[i - 1]].used_memory = i - 1;

				glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 16 * (i - 1), sizeof(float) * 16, &std::vector<float>(models[used_memory[i - 1]].matrix)[0]);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			count_borders -= int(models[model_id].border);
			used_memory[models.size() - 1] = -1;
			models.erase(model_id);
		}

		void change_matrix(eng::Matrix trans, int model_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::CHANGE_MATRIX\n" << "Invalid model id.\n";
				//assert(0);
			}

			models[model_id].matrix = trans * models[model_id].matrix;

			glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 16 * models[model_id].used_memory, sizeof(float) * 16, &std::vector<float>(models[model_id].matrix)[0]);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		bool is_model(int model_id) {
			return models.count(model_id);
		}

		void draw_depth_map() {
			for (std::unordered_map < int, eng::Mesh >::iterator polygon = polygons.begin(); polygon != polygons.end(); polygon++) {
				if (!polygon->second.material.shadow)
					continue;

				polygon->second.draw(models.size(), eng::Shader());
			}
		}

		template <size_t T>
		void draw_polygon(eng::Shader<T>* shader_program, int object_id, int model_id, int polygon_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::DRAW_POLYGON\n" << "Invalid model id.\n";
				//assert(0);
			}

			if (!polygons.count(polygon_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::DRAW_POLYGON\n" << "Invalid polygon id.\n";
				//assert(0);
			}

			set_uniforms(shader_program, object_id);
			shader_program->set_uniform_matrix("not_instance_model", models[model_id].matrix);
			shader_program->set_uniform_i("model_id", models[model_id].used_memory);

			if (models[model_id].border) {
				glStencilFunc(GL_ALWAYS, border_bit, 0xFF);
				glStencilMask(border_bit);
			}

			polygons[polygon_id].draw(1, *shader_program);

			if (models[model_id].border)
				glStencilMask(0x00);
		}

		template <size_t T>
		void draw(eng::Vect3 view_pos, eng::Shader<T>* shader_program, int object_id, int model_id = -1) {
			if (model_id != -1 && !models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::DRAW\n" << "Invalid model id.\n";
				//assert(0);
			}

			set_uniforms(shader_program, object_id);

			if (model_id != -1) {
				if (models[model_id].border) {
					glStencilFunc(GL_ALWAYS, border_bit, 0xFF);
					glStencilMask(border_bit);
				}

				draw_polygons(shader_program, model_id);

				if (models[model_id].border)
					glStencilMask(0x00);

				return;
			}

			if (count_borders) {
				glStencilFunc(GL_ALWAYS, border_bit, 0xFF);
				glStencilMask(border_bit);

				for (std::unordered_map < int, eng::Model >::iterator model = models.begin(); model != models.end(); model++) {
					if (!model->second.border)
						continue;

					draw_polygons(shader_program, model->first);
				}

				glStencilMask(0x00);
			}

			draw_polygons(shader_program);
		}

		void central_scaling(eng::Vect3 scale, int model_id) {
			if (!models.count(model_id)) {
				//std::cout << "ERROR::GRAPH_OBJECT::CENTRAL_SCALING\n" << "Invalid model id.\n";
				//assert(0);
			}

			eng::Matrix model_matrix = get_matrix(model_id);
			change_matrix(model_matrix * eng::Matrix::scale_matrix(scale) * model_matrix.inverse(), model_id);
		}

		void import_from_file(std::string path) {
			polygons.clear();

			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals);

			if (scene == NULL || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == NULL) {
				//std::cout << "ERROR::GRAPH_OBJECT::IMPORT\n" << importer.GetErrorString() << "\n";
				//assert(0);
			}
			std::string directory = path.substr(0, path.find_last_of('/'));

			for (int i = 0; i < scene->mNumTextures; i++) {
				aiTexture* tex = scene->mTextures[i];

				if (tex->mHeight == 0) {
					std::ofstream fout(directory + "/inline_texture" + std::to_string(i) + "." + std::string(tex->achFormatHint), std::ios::binary);
					fout.write((const char*)tex->pcData, tex->mWidth);
				}
			}

			process_node(scene->mRootNode, scene, directory, eng::Matrix::one_matrix(4));
		}

		~GraphObject() {
			delete_buffers();
		}
	};
}

#include "Cube.h"
#include "Cylinder.h"
#include "Sphere.h"
