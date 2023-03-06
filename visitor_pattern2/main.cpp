
#include "shape.hpp"

#include <vector>

int main()
{
	std::vector<shape> shapes;
	shapes.push_back(circle{ .pos = { 0.5, -0.5 }, .radius = 3.14f });
	shapes.push_back(square{ .pos = { 0.5, -0.5 }, .side = 6.28f });

	draw_operation dr{};
	for (auto& shape : shapes)
		std::visit(dr, shape);
}
