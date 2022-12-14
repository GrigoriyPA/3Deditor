#pragma once

#include "GraphicFunctions.h"


namespace gre {
	class Texture {
		inline static GLint max_texture_image_units_ = 0;

		size_t width_ = 0;
		size_t height_ = 0;
		size_t* count_links_ = nullptr;
		GLuint texture_id_ = 0;

		void deallocate() {
			if (count_links_ != nullptr) {
				--(*count_links_);
				if (*count_links_ == 0) {
					delete count_links_;

					glDeleteTextures(1, &texture_id_);
					check_gl_errors(__FILE__, __LINE__, __func__);
				}
			}
			count_links_ = nullptr;
			texture_id_ = 0;
		}

		void swap(Texture& other) noexcept {
			std::swap(width_, other.width_);
			std::swap(height_, other.height_);
			std::swap(count_links_, other.count_links_);
			std::swap(texture_id_, other.texture_id_);
		}

	public:
		Texture() {
			if (!glew_is_ok()) {
				throw GreRuntimeError(__FILE__, __LINE__, "Texture, failed to initialize GLEW.\n\n");
			}

			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_image_units_);
		}

		explicit Texture(const std::string& texture_path, bool gamma = true) {
			if (!glew_is_ok()) {
				throw GreRuntimeError(__FILE__, __LINE__, "Texture, failed to initialize GLEW.\n\n");
			}

			sf::Image image;
			if (!image.loadFromFile(texture_path)) {
				throw GreRuntimeError(__FILE__, __LINE__, "Texture, texture file loading failed.\n\n");
			}

			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_image_units_);
			width_ = image.getSize().x;
			height_ = image.getSize().y;
			count_links_ = new size_t(1);

			glGenTextures(1, &texture_id_);
			glBindTexture(GL_TEXTURE_2D, texture_id_);

			if (gamma) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, static_cast<GLsizei>(width_), static_cast<GLsizei>(height_), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width_), static_cast<GLsizei>(height_), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
			}

			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glBindTexture(GL_TEXTURE_2D, 0);

			check_gl_errors(__FILE__, __LINE__, __func__);
		}

		Texture(const Texture& other) noexcept {
			width_ = other.width_;
			height_ = other.height_;
			texture_id_ = other.texture_id_;
			count_links_ = other.count_links_;
			if (count_links_ != nullptr) {
				++(*count_links_);
			}
		}

		Texture(Texture&& other) noexcept {
			swap(other);
		}

		Texture& operator=(const Texture& other)& noexcept {
			Texture object(other);
			swap(object);
			return *this;
		}

		Texture& operator=(Texture&& other)& noexcept {
			deallocate();
			swap(other);
			return *this;
		}

		bool operator==(const Texture& other) const noexcept {
			return texture_id_ == other.texture_id_;
		}

		bool operator!=(const Texture& other) const noexcept {
			return !(*this == other);
		}

		Texture& set_wrapping(GLint wrapping) {
			if (wrapping != GL_REPEAT && wrapping != GL_MIRRORED_REPEAT && wrapping != GL_CLAMP_TO_EDGE && wrapping != GL_CLAMP_TO_BORDER) {
				throw GreInvalidArgument(__FILE__, __LINE__, "set_wrapping, invalid wrapping type.\n\n");
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);
			glBindTexture(GL_TEXTURE_2D, 0);

			check_gl_errors(__FILE__, __LINE__, __func__);
			return *this;
		}

		GLuint get_id() const noexcept {
			return texture_id_;
		}

		size_t get_width() const noexcept {
			return width_;
		}

		size_t get_height() const noexcept {
			return height_;
		}

		template <typename T>
		T get_value(T value, std::function<void(sf::Color, T*)> func) const {
			uint8_t* buffer = new uint8_t[4 * width_ * height_];
			glBindTexture(GL_TEXTURE_2D, texture_id_);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
			glBindTexture(GL_TEXTURE_2D, 0);
			check_gl_errors(__FILE__, __LINE__, __func__);

			for (size_t i = 0; i < 4 * width_ * height_; i += 4) {
				func(sf::Color(buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3]), &value);
			}

			delete[] buffer;
			return value;
		}

		void activate(GLenum unit_id) const {
			if (max_texture_image_units_ <= static_cast<GLint>(unit_id)) {
				throw GreInvalidArgument(__FILE__, __LINE__, "activate, invalid texture unit id.\n\n");
			}

			if (texture_id_ == 0) {
				return;
			}

			glActiveTexture(GL_TEXTURE0 + unit_id);
			glBindTexture(GL_TEXTURE_2D, texture_id_);
			glActiveTexture(GL_TEXTURE0);

			check_gl_errors(__FILE__, __LINE__, __func__);
		}

		void deactive(GLenum unit_id) const {
			if (max_texture_image_units_ <= static_cast<GLint>(unit_id)) {
				throw GreInvalidArgument(__FILE__, __LINE__, "deactive, invalid texture unit id.\n\n");
			}

			if (texture_id_ == 0) {
				return;
			}

			glActiveTexture(GL_TEXTURE0 + unit_id);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0);

			check_gl_errors(__FILE__, __LINE__, __func__);
		}

		~Texture() {
			deallocate();
		}
	};
}
