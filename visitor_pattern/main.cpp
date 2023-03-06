
#include "shape.hpp"
#include <vector>
#include <memory>
#include <algorithm>


using shapes_t = std::vector<std::unique_ptr<shape>>;

auto new_circle(float x, float y, float r)
{
	auto p = std::make_unique<circle>();
	p->pos[0] = x;
	p->pos[1] = y;
	p->radius = r;
	return p;
}

int main()
{
	shapes_t shapes;
	shapes.push_back(new_circle(0, 1, 2));

	shape_visitor_draw dr;
	for (auto&& p : shapes)
		p->accept(dr);
}
