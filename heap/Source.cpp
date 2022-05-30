
import <vector>;
import <concepts>;
import <utility>;
import <iterator>;
import <algorithm>;
import <functional>;
import <random>;

import <ksn/metapr.hpp>;

template<class T, ksn::universal_reference<T> Tref, class Comp = std::less<T>>
void push_heap(std::pmr::vector<T>& arr, Tref&& x, Comp&& comp = {})
{
	size_t n = arr.size();
	arr.push_back(std::forward<T>(x));
	while (n)
	{
		size_t parent = (n - 1) / 2;
		if (!comp(arr[parent], arr[n]))
			std::swap(arr[parent], arr[n]);
		n = parent;
	}
}

template<class T, class Comp = std::less<T>>
void sift_down(std::pmr::vector<T>& arr, size_t n, size_t N, Comp&& comp = {})
{
	while (n < N)
	{
		size_t min = n;
		for (size_t i = 2 * n + 1; i <= 2 * n + 2; ++i)
		{
			if (i < N && !comp(arr[min], arr[i]))
				min = i;
		}
		if (min == n)
			break;
		std::swap(arr[n], arr[min]);
		n = min;
	}
}

template<class T, class Comp = std::less<T>>
T erase_min_heap(std::pmr::vector<T>& arr, Comp&& comp = {})
{
	T min_value = std::move(arr.front());
	arr.front() = std::move(arr.back());
	arr.pop_back();
	sift_down(arr, 0, arr.size(), comp);
	return min_value;
}

size_t round_down_pow2(size_t x)
{
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >> 16);
	x = x | (x >> 32);
	return (x + 1) >> 1;
}

template<class T, class Comp = std::less<T>>
void make_heap(std::pmr::vector<T>& arr, Comp&& comp = {})
{
	const size_t N = arr.size();
	const size_t lim = round_down_pow2(N);
	if (lim == 0)
		return;
	for (size_t i = lim - 1; i != -1; --i)
	{
		sift_down(arr, i, arr.size(), comp);
	}
}

template<class T, class Comp = std::less<T>>
void heap_sort(std::pmr::vector<T>& arr, Comp&& comp = {})
{
	auto inverse_comp = std::not_fn(comp);
	make_heap(arr, inverse_comp);
	for (size_t i = arr.size() - 1; i != -1; --i)
	{
		std::swap(arr.front(), arr[i]);
		sift_down(arr, 0, i, inverse_comp);
	}
}


int main()
{
	std::pmr::vector<int> v0, v;
	std::mt19937_64 engine;
	constexpr size_t N = 1000000;
	std::generate_n(std::back_inserter(v0), N, [&] {return (int)engine(); });
	bool b = true;

	v = v0;
	heap_sort(v);
	b = std::ranges::is_sorted(v);

	v = v0;
	std::ranges::sort(v);
	b = std::ranges::is_sorted(v);

	v = v0;
	std::ranges::stable_sort(v);
	b = std::ranges::is_sorted(v);

	return 0;
}
