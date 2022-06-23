
import <iterator>;
import <functional>;
import <concepts>;
import <random>;
import <ksn/metapr.hpp>;


template<
	std::forward_iterator It,
	class T = std::iterator_traits<It>::value_type,
	std::strict_weak_order<T, T> Pred = std::less<T>>
It rearrange_by_iter(It begin, It end, It value, Pred pred = {})
{
	return {};
}

template<
	std::forward_iterator It,
	class T = std::iterator_traits<It>::value_type,
	std::strict_weak_order<T, T> Pred>
It rearrange_by_value(It begin, It end, T& value, Pred pred = {})
{
	while (begin != end && pred(*begin, value))
		++begin;

	It ptr = begin;
	while (ptr != end)
	{
		if (pred(*ptr, value))
		{
			std::iter_swap(begin, ptr);
			++begin;
		}
		++ptr;
	}
	return begin;
}

template<class R>
class inverse_relation_t
{
	R ref;
public:
	template<ksn::universal_reference<R> T>
	inverse_relation_t(T obj)
		: ref(std::forward<R>(obj))
	{
	}

	template<class A, class B> requires(std::regular_invocable<R, B&&, A&&>)
	constexpr bool operator()(A&& a, B&& b)
	{
		return ref(b, a);
	}
};

template<
	std::forward_iterator It,
	class T = std::iterator_traits<It>::value_type,
	std::strict_weak_order<T, T> Pred = std::less<T>>
std::pair<It, It> rearrange_by_value_2way(It begin, It end, T value, Pred pred = {})
{
	using inv_pred_t = inverse_relation_t<Pred>;

	It p1 = rearrange_by_value(begin, end, value, pred);
	It p2 = rearrange_by_value(
		std::reverse_iterator<It>(end),
		std::reverse_iterator<It>(p1),
		value, inv_pred_t(pred)).base();
	return { p1, p2 };
}


template<
	std::forward_iterator It,
	class T = std::iterator_traits<It>::value_type,
	std::strict_weak_order<T, T> Pred = std::less<T>>
void selection_sort(It begin, It end, Pred pred = {})
{
	while (begin != end)
	{
		It p = begin, min = p, max = p;
		while (p != end)
		{
			if (pred(*p, *min))
				min = p;
			if (pred(*max, *p))
				max = p;
			++p;
		}
		std::iter_swap(min, begin);
		++begin;
	}
}


template<
	std::random_access_iterator It,
	class T = std::iterator_traits<It>::value_type,
	std::strict_weak_order<T, T> Pred = std::less<T>>
void quick_sort(It begin, It end, Pred pred = {})
{
	const auto n = end - begin;
	if (n <= 1)
		return;
	if (n <= 16) //~10% better
		return selection_sort(begin, end, pred);

	static std::minstd_rand rng;;

	It p1 = begin + rng() % n;
	It p2 = begin + rng() % n;
	It p3 = begin + rng() % n;

	if (pred(*p2, *p1)) std::iter_swap(p2, p1);
	if (pred(*p3, *p2)) std::iter_swap(p3, p2);
	if (pred(*p2, *p1)) std::iter_swap(p2, p1);

	auto [low, high] = rearrange_by_value_2way(begin, end, *p2, pred);
	quick_sort(begin, low, pred);
	quick_sort(high, end, pred);
}




int main()
{
	using namespace std;
	vector<int> arr(100000), v;

	std::generate_n(std::begin(arr), std::size(arr), [] {return (int)rand(); });

	v = arr;
	std::sort(std::begin(v), std::end(v));

	v = arr;
	quick_sort(std::begin(v), std::end(v));

	[] {}();
}