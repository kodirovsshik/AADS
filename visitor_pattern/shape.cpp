
#include "shape.hpp"
#include <iostream>

void circle::accept(shape_visitor& v)
{
	v.visit(*this);
}

void square::accept(shape_visitor& v)
{
	v.visit(*this);
}

void shape_visitor_draw::visit(circle& c)
{
	std::cout << "Circle at (" << c.pos[0] << "; " << c.pos[1] << ") with radius " << c.radius << "\n";
}

void shape_visitor_draw::visit(square& s)
{
	std::cout << "Square at (" << s.pos[0] << "; " << s.pos[1] << ") with side " << s.side << "\n";
}

shape::~shape()
{
}

shape_visitor::~shape_visitor()
{
}
