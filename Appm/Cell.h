#pragma once
#include "GeometryItem.h"

#include "Face.h"
#include <limits>
class Face;

class Cell :
	public GeometryItem
{
public:
	Cell();
	Cell(const std::vector<Face*> & faces);
	~Cell();

	const double getVolume() const;
	const std::vector<Face*> getFaceList() const;
	bool hasFace(const Face * face) const;
	const int getOrientation(const Face * face) const;

	const Eigen::Vector3d & getCenter() const;

private:
	double volume = 0;
	Eigen::Vector3d center;
	std::vector<Face*> faceList;
};
