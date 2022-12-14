#pragma once

#include "Cut.h"


namespace gre {
	class Plane {
		Vec3 normal_ = Vec3(0, 1, 0);

	public:
		double distance = 0;

		Plane() noexcept {
		}

		Plane(const Vec3& point1, const Vec3& point2, const Vec3& point3) {
			try {
				normal_ = ((point1 - point2) ^ (point1 - point3)).normalize();
			}
			catch (GreDomainError) {
				throw GreInvalidArgument(__FILE__, __LINE__, "Plane, points to initialize are collinear.\n\n");
			}

			distance = normal_ * point1;
		}

		Plane(const std::initializer_list<Vec3>& points) {
			if (points.size() < 3) {
				throw GreInvalidArgument(__FILE__, __LINE__, "Plane, the number of points is less than three.\n\n");
			}

			std::vector<Vec3> init;
			for (const Vec3& point : points) {
				init.push_back(point);

				if (init.size() == 3) {
					break;
				}
			}
			*this = Plane(init[0], init[1], init[2]);
		}

		explicit Plane(const std::vector<Vec3>& points) {
			if (points.size() < 3) {
				throw GreInvalidArgument(__FILE__, __LINE__, "Plane, the number of points is less than three.\n\n");
			}

			*this = Plane(points[0], points[1], points[2]);
		}

		Plane& set_normal(const Vec3& normal) {
			try {
				normal_ = normal.normalize();
			}
			catch (GreDomainError) {
				throw GreInvalidArgument(__FILE__, __LINE__, "set_normal, the normal vector has zero length.\n\n");
			}
			return *this;
		}

		Vec3 get_normal() const noexcept {
			return normal_;
		}

		Vec3 project_point(const Vec3& point) const noexcept {
			return normal_ * (normal_ * (normal_ * distance - point)) + point;
		}

		bool on_plane(const Vec3& point) const noexcept {
			return equality(point * normal_, distance);
		}

		bool is_intersect(const Line& line) const noexcept {
			return !equality(line.get_direction() * normal_, 0.0);
		}

		bool is_intersect(const Cut& cut) const noexcept {
			if (!is_intersect(cut.get_line())) {
				return false;
			}

			int32_t diff1 = sgn(cut.get_point1() * normal_ - distance);
			int32_t diff2 = sgn(cut.get_point2() * normal_ - distance);
			return diff1 == 0 || diff2 == 0 || diff1 != diff2;
		}

		bool is_intersect(const Plane& plane) const noexcept {
			return !equality((normal_ ^ plane.normal_).length(), 0.0);
		}

		// Returns some point on other object if there is no intersection
		Vec3 intersect(const Line& line) const noexcept {
			double product = line.get_direction() * normal_;
			if (equality(product, 0.0)) {
				return line.start_point;
			}

			double alf = (distance - normal_ * line.start_point) / product;
			return line.start_point + alf * line.get_direction();
		}

		// Returns some point on other object if there is no intersection
		Vec3 intersect(const Cut& cut) const noexcept {
			return cut.project_point(intersect(cut.get_line()));
		}

		// Returns some line on other plane if there is no intersection
		Line intersect(const Plane& plane) const noexcept {
			try {
				Vec3 direction = (normal_ ^ plane.normal_).normalize();
				Vec3 start_point = normal_ * distance;
				Line ort_line(start_point, start_point + (direction ^ normal_));

				Vec3 intersection = plane.intersect(ort_line);
				return Line(intersection, intersection + direction);
			}
			catch (GreDomainError) {
				return Line(plane.distance * plane.get_normal(), plane.distance * plane.get_normal() + plane.get_normal().horizont());
			}
		}

		Vec3 symmetry(const Vec3& point) const noexcept {
			return point.symmetry(project_point(point));
		}
	};
}
