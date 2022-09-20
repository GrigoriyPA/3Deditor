#pragma once

#include "Vect3.h"
#include "Line3.h"
#include "Cut3.h"


class Flat {
	double eps = 0.000001;

	Vect3 normal;

public:
	double k;

	Flat(Vect3 point1, Vect3 point2, Vect3 point3) {
		normal = (point1 - point2) ^ (point1 - point3);

		if (normal.length() < eps) {
			if ((point2 - point1).length() > eps) {
				point3 = (point2 - point1).horizont() + point1;
				normal = (point1 - point2) ^ (point1 - point3);
			}
			else {
				normal = Vect3(0, 1, 0);
			}
		}

		normal = normal.normalize();
		k = normal * point1;
	}

	Flat(std::vector < Vect3 > points) {
		if (points.size() < 3) {
			std::cout << "ERROR::FLAT::BUILDER\n" << "The number of points is less than three.\n";
			assert(0);
		}

		*this = Flat(points[0], points[1], points[2]);
	}

	Vect3 get_normal() {
		return normal;
	}

	Vect3 project_point(Vect3 point) {
		return normal * (normal * (normal * k - point)) + point;
	}

	bool on_plane(Vect3 point) {
		return point * normal == k;
	}

	bool is_intersect(Line3 line) {
		return abs(line.get_direction() * normal) > eps;
	}

	bool is_intersect(Cut3 cut) {
		if (!is_intersect(cut.get_line()))
			return false;

		double k1 = cut.get_point1() * normal - k, k2 = cut.get_point2() * normal - k;

		return k1 <= 0 && k2 >= 0 || k1 >= 0 && k2 <= 0;
	}

	bool is_intersect(Flat plane) {
		return (normal ^ plane.get_normal()).length() > eps;
	}

	Vect3 intersect(Line3 line) {
		Vect3 direct = line.get_direction();
		double prod = direct * normal;

		if (abs(prod) < eps)
			return Vect3(0, 0, 0);

		double alf = (k - normal * line.p0) / prod;

		return line.p0 + direct * alf;
	}

	Vect3 intersect(Cut3 cut) {
		return intersect(cut.get_line());
	}

	Line3 intersect(Flat plane) {
		Vect3 direct = normal ^ plane.get_normal();

		if (direct.length() < eps)
			return Line3(Vect3(0, 0, 0), normal);

		direct.normalize();
		Vect3 p0 = normal * k;
		Line3 ort_line(p0, p0 + (direct ^ normal));

		Vect3 intersect = plane.intersect(ort_line);

		return Line3(intersect, intersect + direct);
	}

	Vect3 symmetry(Vect3 point) {
		Vect3 proj = project_point(point);

		return point.symmetry(proj);
	}
};
