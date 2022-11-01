
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
	struct default_digit_shifter
	{
		template<class T>
		uint8_t operator()(const T& x, uint32_t shift, uint8_t mask) const
		{
			return uint8_t(x >> shift) & mask;
		}
	};

	struct shifting_data
	{
		uint16_t value;
		uint8_t delta;
	};

	template<class Iter, class ProjFunc, class size_type>
	void afsort_backward_recursive_impl(Iter arr, size_type n, ProjFunc&& digit_extractor, shifting_data shift)
	{
		using bucket_array = std::array<size_type, 256>;
		using std::iter_swap;

		if (n <= 1)
			return;

#if _KSN_IS_DEBUG_BUILD && _KSN_CPP_VER >= 201700L
		std::span _debug(arr, arr + n);
#endif

		bucket_array bucket_begins;
		const int base = 1 << shift.delta;
		const int bucket_count = base;

		do
		{
			bucket_array counts{};
			for (size_type i = 0; i < n; ++i)
				++counts[(uint8_t)digit_extractor(arr[i], shift.value, base - 1)];

			std::exclusive_scan(counts.begin(), counts.begin() + bucket_count, bucket_begins.begin(), 0);
			bucket_array& bucket_ends = counts;

		} while (shift.value > 0)
	}

	template<class Iter, class ProjFunc, class size_type>
	void afsort_forward_recursive_impl(Iter arr, size_type n, ProjFunc&& digit_extractor, shifting_data shift)
	{
		using bucket_array = std::array<size_type, 256>;
		using std::iter_swap;

		if (n <= 1)
			return;

#if _KSN_IS_DEBUG_BUILD && _KSN_CPP_VER >= 201700L
		std::span _debug(arr, arr + n);
#endif
			
		bucket_array bucket_ends{};
		const int base = 1 << shift.delta;
		const int bucket_count = base;

		{
			bucket_array count{};
			for (size_type i = 0; i < n; ++i)
				++count[(uint8_t)digit_extractor(arr[i], shift.value, base - 1)];

			std::inclusive_scan(count.begin(), count.begin() + bucket_count, bucket_ends.begin());
			//std::exclusive_scan(count.begin(), count.begin() + bucket_count, count.begin(), (size_type)0);
			//^^^ shall give up the same array computed by std::inclusive_scan shifted by 1 element ^^^
			//So copy it instead of recomputing
			//memcpy optimization pretty please?
			count[0] = 0;
			for (int i = 0; i < bucket_count - 1; ++i)
				count[i + 1] = bucket_ends[i];

			bucket_array& bucket_indexes = count;

			for (int bucket = 0; bucket < bucket_count; ++bucket)
			{
				size_type i = bucket_indexes[bucket];
				while(i < bucket_ends[bucket])
				{
					const int desired_bucket = digit_extractor(arr[i], shift.value, base - 1);
					if (desired_bucket == bucket)
					{
						++i;
						continue;
					}
					iter_swap(arr + i, arr + bucket_indexes[desired_bucket]++);
				}
			}
		}

		if (shift.value == 0)
			return;
			
		shift.value -= shift.delta;
		size_type current_bucket_start = 0;
		for (int i = 0; i < bucket_count; ++i) //TODO: check if overflow occures for size_type < size_t
		{
			const size_type sz = bucket_ends[i] - current_bucket_start;
			afsort_forward_recursive_impl(arr + current_bucket_start, sz, std::forward<ProjFunc>(digit_extractor), shift);
			current_bucket_start = bucket_ends[i];
		}
	}

	template<class Iter, class ProjFunc>
	void afsort_recursive(Iter arr, Iter end, ProjFunc&& digit_extractor, shifting_data shift)
	{
		const size_t sz = end - arr;
		auto invoke_impl = [&]<class size_type>
		{
			return afsort_forward_recursive_impl(arr, (size_type)sz, std::forward<ProjFunc>(digit_extractor), shift);
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

	template<class T>
	uint8_t afsort_iterations(T value, uint8_t shift)
	{
		uint8_t iterations = 0;
		do
		{
			value >>= shift;
			++iterations;
		} while (value > 0);
		return iterations;
	}
}

template<class Iter, class ProjFunc = const detail::default_digit_shifter&>
void afsort(Iter arr, Iter end, uint8_t log2_of_base = -1, ProjFunc&& digit_getter = {})
{
	using T = typename std::iterator_traits<Iter>::value_type;

	if (end - arr <= 1)
		return;

	if (log2_of_base == (uint8_t)-1) //TODO: autopick
		log2_of_base = 8;

	auto [pmin, pmax] = std::minmax_element(arr, end);
	if (*pmin < 0)
		throw std::runtime_error("not implemented");

	uint8_t iterations = detail::afsort_iterations<T>(*pmax, log2_of_base);
	detail::afsort_recursive(arr, end, std::forward<ProjFunc>(digit_getter), { uint16_t(log2_of_base * (iterations - 1)), log2_of_base });
}
_KSN_END

int main()
{

	std::mt19937_64 rng;
	int a_rray[] = { 0x23, 0x34, 0x35, 0x46, 0x12, 0x21, 0x48, 0x19, 0x0F, 0x5F, 0x47, 0x10, 0x5E, 0x5C, 0x5D };
	//std::ranges::shuffle(a_rray, rng);
	ksn::afsort(a_rray, std::end(a_rray), 4);

	return 0;
}
