
#ifndef _SHAPE_HPP_
#define _SHAPE_HPP_

#include <variant>

using vec2f = float[2];


class circle
{
public:
	vec2f pos;
	float radius;

	//void accept(shape_visitor&) override;
};

class square
{
public:
	vec2f pos;
	float side;

	//void accept(shape_visitor&) override;
};

using shape = std::variant<circle, square>;



class draw_operation
{
public:
	void operator()(const circle&) const;
	void operator()(const square&) const;
};

#endif //!_SHAPE_HPP_
