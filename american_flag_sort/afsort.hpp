
#ifndef _KSN_AFSORT_HPP_
#define _KSN_AFSORT_HPP_

#include <concepts>
#include <string>
#include <array>
#include <utility>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <span>

#include <stdint.h>
#include <limits.h>

#include <ksn/ksn.hpp>

_KSN_BEGIN
namespace detail
{
	static constexpr size_t afsort_max_base_log2 = 4;
	template<class Iter, class ProjFunc, class ExtractFunc, class size_type>
	void afsort_backward_recursive_impl(Iter arr, size_type n, ProjFunc&& projection, ExtractFunc&& digit_extractor, uint32_t iterations, uint8_t log2_of_base, uint64_t shift_value)
	{
		using bucket_array = std::array<size_type, 1 << afsort_max_base_log2>;
		using std::iter_swap;

		if (n <= 1)
			return;

#if _KSN_IS_DEBUG_BUILD && _KSN_CPP_VER >= 202000L
		std::span _debug(arr, arr + n);
#endif

		bucket_array bucket_begins;
		const int base = 1 << log2_of_base;
		constexpr int buckets = (int)std::size(bucket_begins);

		auto get_bucket_number = [&]
		(size_t idx)
		{
			auto result = uint8_t(digit_extractor(projection(arr[idx]), shift_value) & (base - 1));
			return result;
		};
		bucket_array count{};

		for (size_type i = 0; i < n; ++i)
			++count[get_bucket_number(i)];

		std::exclusive_scan(count.begin(), count.begin() + buckets, bucket_begins.begin(), (size_type)0);

		bucket_array& bucket_ends = count;
		for (int i = 0; i < buckets; ++i)
			bucket_ends[i] = bucket_begins[i] + count[i];

		for (int bucket = buckets - 1; bucket >= 0; --bucket)
		{
			if (bucket_ends[bucket] == bucket_begins[bucket])
				continue;

			size_type& i = --bucket_ends[bucket];
			do
			{
				const int desired_bucket = get_bucket_number(i);
				if (desired_bucket != bucket)
					iter_swap(arr + --bucket_ends[desired_bucket], arr + i);
				else
				{
					if (i == bucket_begins[bucket])
						break;
					--i;
				}
			} while (true);
		}

		if (--iterations == 0 || shift_value == 0)
			return;

		size_type current_end = n;
		for (int i = buckets - 1; i >= 0; --i)
		{
			afsort_backward_recursive_impl(arr + bucket_begins[i], current_end - bucket_begins[i],
				projection, digit_extractor, iterations, log2_of_base, shift_value - log2_of_base);
			current_end = bucket_begins[i];
		}
	}



	template<class Iter, class ProjFunc, class ExtractFunc>
	void afsort_recursive(Iter arr, Iter end, ProjFunc&& projection, ExtractFunc&& digit_extractor, uint32_t iterations, uint8_t log2_of_base, uint64_t shift)
	{
		const size_t sz = end - arr;
		auto invoke_impl = [&]<class size_type>
		{
			return afsort_backward_recursive_impl(arr, (size_type)sz, projection, digit_extractor, iterations, log2_of_base, shift);
		};

		return invoke_impl.template operator() < size_t > ();
		if (sz <= 0xFFFFFFFF)
			return invoke_impl.template operator() < uint32_t > ();
		else
			return invoke_impl.template operator() < size_t > ();
	}



	struct non_forwarding_identity
	{
		template<class T>
		const T& operator()(const T& x) const
		{
			return x;
		}
	};

	struct default_length_extractor
	{
		template<std::integral T>
		size_t operator()(T x) const
		{
			//number of binary digits, same as 1 + log2(x) or 1 for x=0
			size_t c = 0;
			do
			{
				c++;
				x /= 2;
			} while (x != 0);
			return c;
		}

		template<class char_t, class traits_t, class alloc_t>
		size_t operator()(const std::basic_string<char_t, traits_t, alloc_t>& x) const
		{
			return x.length();
		}
	};

	struct default_digit_shifter
	{
		template<std::integral T>
		uint8_t operator()(const T& x, uint64_t shift) const
		{
			return uint8_t(x >> shift);
		}

		template<class char_t, class traits_t, class alloc_t>
		uint8_t operator()(const std::basic_string<char_t, traits_t, alloc_t>& str, uint64_t shift) const
		{
			if (shift % CHAR_BIT)
				throw std::runtime_error("afsort: assumption violated: byte-aligned shift for string");
			const uint64_t bytes = shift / CHAR_BIT;
			const uint64_t idx = bytes / sizeof(char_t);
			const uint8_t leftover_bytes = bytes % sizeof(char_t);
			return uint8_t(str[idx] >> (CHAR_BIT * leftover_bytes));
		}
	};

	template<class Test, class To>
	concept same_to_cvref = std::is_same_v<std::remove_cvref_t<Test>, std::remove_cvref_t<To>>;

	struct default_length_comparator
	{
		template<class T, class Extractor>
		bool operator()(T&& a, T&& b, Extractor&& ex)
		{
			if constexpr (same_to_cvref<Extractor, default_length_extractor> && (std::integral<std::remove_cvref_t<T>> || std::floating_point<std::remove_cvref_t<T>>))
				return a < b;
			else
				return ex(a) < ex(b);
		}
	};

	template<
		class Iter,
		class LengthExtractor = default_length_extractor,
		class LengthComparator = default_length_comparator>
	auto find_minmax_length_element(Iter arr, Iter end,
		LengthExtractor&& length_extractor,
		LengthComparator&& length_comparator)
	{
		auto cmp = [&]
		(auto&& a, auto&& b)
		{
			return length_comparator(a, b, length_extractor);
		};
		return std::minmax_element(arr, end, cmp);
	}

	template<class T, class U>
	T divide_and_round_up(T a, U b)
	{
		return a / b + bool(a % b);
	}
}

template<
	class Iter,
	class ProjFunc = detail::non_forwarding_identity,
	class ExtractFunc = detail::default_digit_shifter,
	class LengthExtractor = detail::default_length_extractor,
	class LengthComparator = detail::default_length_comparator>
void afsort(
	Iter arr, Iter end, uint8_t log2_of_base = -1,
	ProjFunc&& projection = {},
	ExtractFunc&& digit_getter = {},
	LengthExtractor&& length_extractor = {},
	LengthComparator&& length_comparator = {}
)
{
	using T = typename std::iterator_traits<Iter>::value_type;

	if (end - arr <= 1)
		return;

	if (log2_of_base == (uint8_t)-1) //TODO: proper autopick
		log2_of_base = detail::afsort_max_base_log2;
	if (log2_of_base > detail::afsort_max_base_log2)
		throw std::runtime_error("afsort: max base exceeded");
	//TODO: enforce byte alignment for strings???

	const auto [pmin, pmax] = detail::find_minmax_length_element(arr, end, length_extractor, length_comparator);
	const auto max_value_length = detail::divide_and_round_up(length_extractor(*pmax), log2_of_base);
	const auto min_value_length = detail::divide_and_round_up(length_extractor(*pmin), log2_of_base);
	const auto value_length_range = max_value_length - std::min<decltype(min_value_length)>(min_value_length, 0);

	const uint64_t iterations = value_length_range + 1;
	if (iterations > UINT32_MAX)
		throw std::runtime_error("afsort: too many iterations");

	const uint64_t shift = (max_value_length - 1) * log2_of_base;

	detail::afsort_recursive(arr, end, projection, digit_getter, (uint32_t)iterations, log2_of_base, shift);
}

_KSN_END

#endif //!_KSN_AFSORT_HPP_
