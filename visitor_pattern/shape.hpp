
#ifndef _SHAPE_HPP_
#define _SHAPE_HPP_

class shape_visitor;
class shape;

class shape
{
public:
	virtual ~shape() = 0;

	virtual void accept(shape_visitor&) = 0;
};

using vec2f = float[2];


class circle
	: public shape
{
public:
	vec2f pos;
	float radius;

	void accept(shape_visitor&) override;
};

class square
	: public shape
{
public:
	vec2f pos;
	float side;

	void accept(shape_visitor&) override;
};



class shape_visitor
{
public:
	virtual ~shape_visitor() = 0;
	virtual void visit(circle&) = 0;
	virtual void visit(square&) = 0;
};

class shape_visitor_draw
	: public shape_visitor
{
	void visit(circle&) override;
	void visit(square&) override;
};

#endif //!_SHAPE_HPP_
