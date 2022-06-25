
#include <concepts>
#include <span>
#include <vector>
#include <numeric>
#include <ranges>
#include <utility>
#include <algorithm>
#include <functional>



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

static constexpr uint8_t _radix_sort_bases_logs[] = { 5, 8, 10, 12, 14, 16, 18 };
static constexpr uint8_t _radix_sort_max_base_log = *std::max_element(std::begin(_radix_sort_bases_logs), std::end(_radix_sort_bases_logs));
static constexpr uint16_t _radix_sort_time_ratios[] = {1193, 5750, 16343, 25519, 25477, 26375, 25632};

static_assert(std::size(_radix_sort_bases_logs) == std::size(_radix_sort_time_ratios));



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
void radix_sort_based(Iter begin, Iter end, uint8_t base_log2, uint8_t iterations) noexcept
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
void make_clean_vector(T& v, size_t n)
{
	v.clear();
	v.resize(n);
}

template<class Iter>
auto _radix_sort_bind(uint8_t base_log2, uint8_t iterations)
{
	struct
	{
		uint8_t base_log2;
		uint8_t iterations;

		void operator()(Iter begin, Iter end) const noexcept
		{
			radix_sort_based(begin, end, this->base_log2, this->iterations);
		}
	} binder{};

	binder.base_log2 = base_log2;
	binder.iterations = iterations;
	return binder;
}

template<class Iter, class T>
auto _radix_sort_get_based_callee(const std::make_unsigned_t<T>& max, size_t n)
{
	//TODO: contract: max >= 1
	if (max < (1 << _radix_sort_max_base_log))
	{
		size_t base_index = 0;
		while (base_index < std::size(_radix_sort_bases_logs))
		{
			if (max < (1u << _radix_sort_bases_logs[base_index]))
				break;
			++base_index;
		}
		return _radix_sort_bind<Iter>(_radix_sort_bases_logs[base_index], 1);
	}

	size_t base_index = 0;
	size_t min_base_index;
	int64_t min_time_estimate = INT64_MAX;
	uint8_t min_iterations;
	while (base_index < std::size(_radix_sort_bases_logs))
	{
		auto base = 1ll << _radix_sort_bases_logs[base_index];
		uint8_t iterations = _radix_sort_log(max, _radix_sort_bases_logs[base_index]) + 1;

		int64_t time_estimate = iterations * (base * 10000 + static_cast<int64_t>(n) * _radix_sort_time_ratios[base_index]);
		if (time_estimate < min_time_estimate)
		{
			min_time_estimate = time_estimate;
			min_base_index = base_index;
			min_iterations = iterations;
		}

		++base_index;
	}
	return _radix_sort_bind<Iter>(_radix_sort_bases_logs[min_base_index], min_iterations);
}

template<std::random_access_iterator Iter>
void radix_sort(Iter begin, Iter end) noexcept
{
	using T = std::iterator_traits<Iter>::value_type;

	size_t n = std::distance(begin, end);
	if (n <= 1)
		return;

	make_clean_vector(_radix_sort_data<T>.auxillary, n);

	auto [pmin, pmax] = std::minmax_element(begin, end);
	auto&& min = *pmin;
	auto&& max = *pmax;
	if (min < 0)
		abort(); //TODO
	
	auto sort_func = _radix_sort_get_based_callee<Iter, T>(max, n);

	std::invoke(sort_func, begin, end);

	if (_radix_sort_data<T>.always_free_auxillary)
		reconstruct(_radix_sort_data<T>.auxillary);
}




#include <array>
#include <chrono>
#include <random>
#include <iostream>

static constexpr size_t N = 100'000'000;
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

	measure(starting_arr, test_radix_sort, "LSD Radix sort (base autopicked)");
	//measure(starting_arr, test_std_sort, "std::sort");
}
