
#include "shape.hpp"
#include <iostream>

void draw_operation::operator()(const circle& c) const
{
	std::cout << "Circle at (" << c.pos[0] << "; " << c.pos[1] << ") with radius " << c.radius << "\n";
}

void draw_operation::operator()(const square& s) const
{
	std::cout << "Square at (" << s.pos[0] << "; " << s.pos[1] << ") with side " << s.side << "\n";
}
