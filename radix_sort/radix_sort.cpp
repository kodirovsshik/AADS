
#include <concepts>
#include <span>
#include <vector>
#include <numeric>
#include <ranges>
#include <utility>
#include <algorithm>



struct _radix_sort_common_data_t
{
	constexpr static int base_log2 = 8;
	constexpr static int base = 1 << base_log2;

	template<class T>
	static size_t iterations_count(T max)
	{
		size_t result = 0;
		do
		{
			max >>= base_log2;
			++result;
		} while (max != 0);
		return result;
	}

	size_t counts[base];
} thread_local _radix_sort_common_data;

template<class T>
struct _radix_sort_data_t
{
	static bool always_free_auxillary;
	static bool always_free_classifications;

	std::vector<T> auxillary;
	std::vector<uint8_t> classifications;
};

template<class T>
bool _radix_sort_data_t<T>::always_free_auxillary = true;
template<class T>
bool _radix_sort_data_t<T>::always_free_classifications = true;

template<class T>
thread_local _radix_sort_data_t<T> _radix_sort_data;



template<std::bidirectional_iterator Iter, bool reuse_classification>
void radix_sort1(Iter begin, Iter end, size_t n) noexcept
{
	using T = std::iterator_traits<Iter>::value_type;
	using Tref = std::iterator_traits<Iter>::reference;
	using RSIter = std::reverse_iterator<typename std::span<T>::iterator>;

	static constexpr int base = _radix_sort_common_data.base;

	auto& counts = _radix_sort_common_data.counts;
	auto& aux = _radix_sort_data<T>.auxillary;

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

	auto get_classification = [&]
	(auto&& x) -> uint8_t
	{
		return static_cast<uint8_t>((x >> shift) % base);
	};

	auto classify = [&]
	(auto&& x, size_t n) -> uint8_t
	{
		uint8_t value = get_classification(x);
		if constexpr (reuse_classification)
			_radix_sort_data<T>.classifications[n] = value;
		return value;
	};

	auto reclassify = [&]
	(auto&& x, size_t n) -> uint8_t
	{
		if constexpr (reuse_classification)
			return _radix_sort_data<T>.classifications[n];
		return get_classification(x);
	};

	size_t iterations = _radix_sort_common_data_t::iterations_count(*std::max_element(begin, end));

	swap_buffers();
	while (true)
	{
		memset(counts, 0, sizeof(counts));

		if constexpr (std::is_trivial_v<T>)
			memset(aux_span.data(), 0, aux_span.size() * sizeof(aux_span.front()));
		else
		{
			for (auto&& x : aux_span)
				x = T{};
		}

		auto main_begin = main_span.begin();
		auto main_end = main_span.end();

		int first = classify(*main_begin, 0);
		++counts[first];

		int min_class = 0;
		int max_class = 255;

		size_t index = 0;
		for (auto p = main_begin;;)
		{
			++p;
			if (p == main_end)
				break;
			int current = classify(*p, ++index);
			++counts[current];
			//if (current < min_class)
				//min_class = current;
			//if (current > max_class)
				//max_class = current;
		}

		std::partial_sum(counts + min_class, counts + max_class + 1, counts + min_class);

		for (auto p = rbegin; p != rend; ++p, --index)
			aux_span[--counts[reclassify(*p, index)]] = std::move(*p);

		swap_buffers();

		if (--iterations == 0)
			break;

		shift += _radix_sort_common_data.base_log2;
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

template<std::bidirectional_iterator Iter>
void radix_sort(Iter begin, Iter end) noexcept
{
	using T = std::iterator_traits<Iter>::value_type;
	size_t n = std::distance(begin, end);

	if (n <= 1)
		return;

	make_clean_vector(_radix_sort_data<T>.auxillary, n);

	try
	{
		make_clean_vector(_radix_sort_data<T>.classifications, n);
		radix_sort1<Iter, true>(begin, end, n);
	}
	catch (const std::bad_alloc&)
	{
		radix_sort1<Iter, false>(begin, end, n);
	}

	if (_radix_sort_data<T>.always_free_auxillary)
		reconstruct(_radix_sort_data<T>.auxillary);
	if (_radix_sort_data<T>.always_free_classifications)
		reconstruct(_radix_sort_data<T>.classifications);
}

int main()
{
	using namespace std;
	int arr[] = { 10'000'000, 1'000'000, 100'000, 10'000, 1'000, 100, 0xFF0000, 1 };

	radix_sort(begin(arr), end(arr));
}
