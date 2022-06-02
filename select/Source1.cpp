
import <concepts>;
import <iostream>;
import <utility>;
import <functional>;
import <random>;


template<
	std::forward_iterator It,
	class T = std::iterator_traits<It>::value_type,
	std::strict_weak_order<T, T> Pred = std::less<T>>
It rearrange(It begin, It end, It value, Pred pred = {})
{
	It result = end;
	while (begin != end)
	{
		if (!pred(*value, *begin))
			++begin;
		else
		{
			while (true)
			{
				--end;
				if (begin == end)
					break;
				else if (!pred(*value, *end))
				{
					std::iter_swap(begin, end);
					if (value == begin)
						value = end;
					else if (value == end)
						value = begin;
					--result;
					++begin;
					break;
				}
			}
		}
	}
	return begin;
}

void assert(bool cond)
{
	if (!cond)
		__debugbreak();
}

template<std::random_access_iterator It, class T = std::iterator_traits<It>::value_type, std::strict_weak_order<T, T> Pred = std::less<void>>
void nth_element(It begin, It nth, It end, Pred pred = {})
{
	static std::minstd_rand rng;

	while (true)
	{
		const size_t N = end - begin;
		if (N <= 1)
			return;

		It p1 = begin + rng() % N;
		It p2 = begin + rng() % N;
		It p3 = begin + rng() % N;

		if (pred(*p2, *p1)) std::swap(p1, p2);
		if (pred(*p3, *p2)) std::swap(p3, p2);
		if (pred(*p2, *p1)) std::swap(p1, p2);

		It div = rearrange(begin, end, p2, pred);
		//TODO: handle arrays of same element
		if (div <= nth)
			begin = div;
		else
			end = div;
	}
}
//it's not quick bruh
template<std::random_access_iterator It, class T = std::iterator_traits<It>::value_type, std::strict_weak_order<T, T> Pred = std::less<void>>
void quick_sort(It begin, It end, Pred pred = {})
{
	const size_t n = end - begin;
	if (n <= 1)
		return;

	auto nth = begin + n / 2;
	nth_element(begin, nth, end, pred);
	quick_sort(begin, nth, pred);
	quick_sort(nth, end, pred);
}

int main()
{
	std::mt19937_64 rng;
	std::vector<int> v;
	
	
	
	static constexpr size_t N = 2000;
	for (size_t i = 0; i < N; ++i)
	{
		std::generate_n(std::back_inserter(v), N, [&] { return (int)rng(); });

		quick_sort(v.begin(), v.end());

		if (!std::ranges::is_sorted(v))
			std::cout << "Error on test " << i << std::endl;
		if (i % 100 == 0)
			std::cout << i << std::endl;
		v.clear();
	}
	return 0;
}