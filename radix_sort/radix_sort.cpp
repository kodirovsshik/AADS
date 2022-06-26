
#include <concepts>
#include <span>
#include <vector>
#include <numeric>
#include <ranges>
#include <utility>
#include <algorithm>



template<class T>
static uint8_t _radix_sort_log(T max, uint8_t base_log2)
{
	uint8_t result = -1;
	while (max > 0)
	{
		max >>= base_log2;
		++result;
	} 
	return result;
}

static constexpr uint8_t _radix_sort_max_base_log = 8;

template<class T>
struct _radix_sort_data_t
{
	static bool always_free_auxillary;

	std::vector<T> auxillary;
};

template<class T>
bool _radix_sort_data_t<T>::always_free_auxillary = true;

template<class T>
thread_local _radix_sort_data_t<T> _radix_sort_data;



template<std::random_access_iterator Iter>
void _radix_sort_based(Iter begin, Iter end, uint8_t base_log2, uint8_t iterations) noexcept
{
	using T = std::iterator_traits<Iter>::value_type;
	using RSIter = std::reverse_iterator<typename std::span<T>::iterator>;

	const int base = 1 << base_log2;

	static thread_local size_t counts[1 << _radix_sort_max_base_log];
	auto& aux = _radix_sort_data<T>.auxillary;

	size_t n = std::distance(begin, end);

	RSIter rbegin;
	RSIter rend;

	std::span<T> main_span(aux.begin(), aux.end());
	std::span<T> aux_span(begin, end);

	int shift = 0;
	int buffer_swap_parity = 1;

	auto swap_buffers = [&]
	{
		std::swap(main_span, aux_span);
		rbegin = RSIter(main_span.end());
		rend = RSIter(main_span.begin());
		buffer_swap_parity = 1 - buffer_swap_parity;
	};

	auto classify = [&]
	(auto&& x)
	{
		return static_cast<int>((x >> shift) & (base - 1));
	};

	swap_buffers();
	while (true)
	{
		memset(counts, 0, sizeof(counts));

		auto main_begin = main_span.begin();
		auto main_end = main_span.end();

		for (auto&& x : main_span)
			++counts[classify(x)];

		std::partial_sum(counts + 0, counts + base, counts + 0);

		for (auto p = rbegin; p != rend; ++p)
			aux_span[--counts[classify(*p)]] = std::move(*p);

		swap_buffers();

		if (--iterations == 0)
			break;

		shift += base_log2;
	}

	if (buffer_swap_parity != 0)
		std::ranges::copy(main_span, aux_span.begin());
}


template<class T>
auto reconstruct(T& obj)
{
	using std::swap;
	T aux{};
	swap(obj, aux);
}
template<class T>
void ensure_vector_size(T& v, size_t n)
{
	if (v.size() < n)
		v.resize(n);
}

template<class T>
std::pair<uint8_t, uint8_t> _radix_sort_get_optimal_base(const T& max, size_t n)
{
	if (max < (1 << _radix_sort_max_base_log))
		return { _radix_sort_log(max, 2) + 1 , 1 };
	else
		return { _radix_sort_max_base_log, _radix_sort_log(max, _radix_sort_max_base_log) + 1 };
}

template<std::random_access_iterator Iter>
void radix_sort(Iter begin, Iter end) noexcept
{
	using T = std::iterator_traits<Iter>::value_type;

	size_t n = std::distance(begin, end);
	if (n <= 1)
		return;

	ensure_vector_size(_radix_sort_data<T>.auxillary, n);

	auto [pmin, pmax] = std::minmax_element(begin, end);
	T min = *pmin;
	auto&& max = *pmax;

	if (min < 0)
		for (auto p = begin; p != end; ++p)
			*p -= min;
	
	auto [base_log2, iterations] = _radix_sort_get_optimal_base<T>(max, n);
	_radix_sort_based(begin, end, base_log2, iterations);

	if (min < 0)
		for (auto p = begin; p != end; ++p)
			*p += min;

	if (_radix_sort_data<T>.always_free_auxillary)
		reconstruct(_radix_sort_data<T>.auxillary);
}



#include <array>
#include <chrono>
#include <random>
#include <iostream>

static constexpr size_t N = 1'000'000;
using arr_t = std::array<unsigned int, N>;

template<class callable_t>
float single_test(const arr_t& starting_arr, callable_t&& callee)
{
	auto& arr = *new arr_t(starting_arr);

	using namespace std;
	using namespace chrono;
	auto clock = steady_clock::now;

	auto t1 = clock();
	callee(arr);
	auto t2 = clock();

	if (!std::ranges::is_sorted(arr))
		abort();

	delete &arr;

	return (t2 - t1).count() * 1e-9f;
}

template<class callable_t>
float measure(const arr_t& starting_arr, callable_t&& callee, const char* name)
{
	constexpr size_t n_tests = 1;

	auto invoke = [&] { return single_test(starting_arr, std::forward<callable_t>(callee)); };
	invoke();

	float time = 0;
	for (size_t i = 0; i < n_tests; ++i)
		time += invoke();
	time /= n_tests;

	std::cout << name << ":\n\t" << std::fixed << std::setprecision(10) << time << " s\n";
	return time;
}


int main()
{
	std::mt19937 rng(0);

	auto& starting_arr = *new arr_t;
	for (auto& x : starting_arr)
		x = rng();

	auto test_std_sort = [&]
	(arr_t& x)
	{
		std::sort(x.begin(), x.end());
	};
	auto test_std_stable_sort = [&]
	(arr_t& x)
	{
		std::stable_sort(x.begin(), x.end());
	};
	auto test_radix_sort = [&]
	(arr_t& x)
	{
		radix_sort(x.begin(), x.end());
	};

	measure(starting_arr, test_std_sort, "std::sort");
	measure(starting_arr, test_std_stable_sort, "std::stable_sort");
	measure(starting_arr, test_radix_sort, "LSD Radix sort (base autopicked)");

	return 0;
}
