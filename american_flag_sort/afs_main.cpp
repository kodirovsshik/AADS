
#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <ranges>
#include <random>
#include <array>

#include <ksn/ksn.hpp>

_KSN_BEGIN
namespace detail
{
	template<class Iter, class ProjFunc, class ExtractFunc, class size_type>
	void afsort_backward_recursive_impl(Iter arr, size_type n, ProjFunc&& projection, ExtractFunc&& digit_extractor, uint32_t iterations, uint8_t log2_of_base)
	{
		using bucket_array = std::array<size_type, 1 << CHAR_BIT>;
		using std::iter_swap;

		if (n <= 1)
			return;

#if _KSN_IS_DEBUG_BUILD && _KSN_CPP_VER >= 202000L
		std::span _debug(arr, arr + n);
#endif

		bucket_array bucket_begins;
		const int base = 1 << log2_of_base;
		auto shift_value = uint64_t(iterations - 1) * log2_of_base;
		int max_current_bucket;

		auto get_bucket_number = [&]
		(size_t idx)
		{
			auto result = uint8_t(digit_extractor(projection(arr[idx]), shift_value) & (base - 1));
			max_current_bucket = std::max<int>(max_current_bucket, result);
			return result;
		};
		bool single_bucket = true;
		max_current_bucket = 0;
		bucket_array count{};

		++count[get_bucket_number(0)];
		for (size_type i = 1; i < n; ++i)
		{
			auto& current_count = count[get_bucket_number(i)];
			single_bucket &= (current_count != 0); //TODO: check if worth it
			++current_count;
		}

		++max_current_bucket;
		std::exclusive_scan(count.begin(), count.begin() + max_current_bucket, bucket_begins.begin(), (size_type)0);

		if (!single_bucket)
		{
			bucket_array& bucket_ends = count;
			for (int i = 0; i < max_current_bucket; ++i)
				bucket_ends[i] = bucket_begins[i] + count[i];

			for (int bucket = max_current_bucket - 1; bucket >= 0; --bucket)
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
		}

		if (--iterations == 0)
			return;

		size_type current_end = n;
		for (int i = max_current_bucket - 1; i >= 0; --i)
		{
			afsort_backward_recursive_impl(arr + bucket_begins[i], current_end - bucket_begins[i],
				projection, digit_extractor, iterations, log2_of_base);
			current_end = bucket_begins[i];
		}
	}
	

	
	template<class Iter, class ProjFunc, class ExtractFunc>
	void afsort_recursive(Iter arr, Iter end, ProjFunc&& projection, ExtractFunc&& digit_extractor, uint32_t iterations, uint8_t log2_of_base)
	{
		const size_t sz = end - arr;
		auto invoke_impl = [&]<class size_type>
		{
			return afsort_backward_recursive_impl(arr, (size_type)sz, projection, digit_extractor, iterations, log2_of_base);
		};

		if (sz <= 0xFF)
			return invoke_impl.template operator()<uint8_t>();
		else if (sz <= 0xFFFF)
			return invoke_impl.template operator()<uint16_t>();
		else if (sz <= 0xFFFFFFFF)
			return invoke_impl.template operator()<uint32_t>();
		else
			return invoke_impl.template operator()<size_t>();
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
		T operator()(T) const
		{
			return sizeof(T) * CHAR_BIT;
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
			//assume shift is byte-aligned for strings, enforced during preparations 
			const uint64_t bytes = shift / CHAR_BIT;
			const uint64_t idx = bytes / sizeof(char_t);
			const uint8_t leftover_bytes = bytes % sizeof(char_t);
			return uint8_t(str[idx] >> (CHAR_BIT * leftover_bytes));
		}
	};

}

template<
	class Iter,
	class ProjFunc = const detail::non_forwarding_identity&,
	class ExtractFunc = const detail::default_digit_shifter&,
	class LengthExtractor = const detail::default_length_extractor&>
void afsort(
	Iter arr, Iter end, uint8_t log2_of_base = -1,
	ProjFunc&& projection = {},
	ExtractFunc&& digit_getter = {},
	LengthExtractor&& length_extractor = {})
{
	using T = typename std::iterator_traits<Iter>::value_type;

	if (end - arr <= 1)
		return;

	if (log2_of_base == (uint8_t)-1) //TODO: autopick
		log2_of_base = CHAR_BIT;
	if (log2_of_base > CHAR_BIT)
		throw std::runtime_error("afsort: max base is UCHAR_MAX");
	//TODO: enforce byte alignment for strings

	const Iter pmax = std::max_element(arr, end, [&]
	(auto&& a, auto&& b)
	{
		return length_extractor(a) < length_extractor(b);
	});
	const auto max_length = length_extractor(*pmax);
	if (max_length == 0)
		return;

	const uint32_t iterations = 1 + (max_length - 1) / log2_of_base;
	detail::afsort_recursive(arr, end, projection, digit_getter, iterations, log2_of_base);
}

_KSN_END


int main()
{
	std::mt19937_64 rng;
	int a[] = { 0x23, 0x34, 0x35, 0x46, 0x12, 0x21, 0x48, 0x19, 0x0F, 0x5F, 0x47, 0x10, 0x5E, 0x5C, 0x5D };
	std::ranges::shuffle(a, rng);
	ksn::afsort(std::begin(a), std::end(a));

	return 0;
}
