
#include <vector>

#include "rotate.hpp"

int main1()
{
	std::vector v{ 0,1,2,3,4,5,6,7,8,9 };
	xrotate(v.begin(), v.begin() + 5, v.end());
	return 0;
}
