
#ifndef _ROTATE_HPP_
#define _ROTATE_HPP_

#include <iterator>
#include <concepts>
#include <span>
#include <algorithm>

#include <cstddef>


template<class T>
T replace_if_equal(T x, const T& to, T with)
{
	if (x == to)
		return with;
	return x;
}

template<std::random_access_iterator Iter>
void xrotate(Iter first, Iter new_first, Iter end)
{
	const size_t n = end - first;
	const size_t k = replace_if_equal<size_t>(new_first - first, n, 0);
	std::span _debug(first, n);

	if (n <= 1 || k == 0)
		return;

	for (size_t i = 0; i < n - k; ++i)
		iter_swap(first + i, first + i + k);
	return xrotate(first + n - k, first + n - n % k, end);
}

#endif //!_ROTATE_HPP_
