
#include <array>
#include <chrono>
#include <random>
#include <iostream>

#include "radix_sort.hpp"



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
	ksn::detail::_radix_sort_data<int>.always_free_auxillary = false;
	int arr[] = { 39 ,02 ,70 ,20 ,28 ,41 ,82 ,81 ,12 ,99 ,34 ,96 ,10 ,67 ,85 ,16 };
	ksn::radix_sort(arr + 0, std::end(arr));
}
int main1()
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
		ksn::radix_sort(x.begin(), x.end());
	};

	measure(starting_arr, test_std_sort, "std::sort");
	measure(starting_arr, test_std_stable_sort, "std::stable_sort");
	measure(starting_arr, test_radix_sort, "LSD Radix sort (base autopicked)");

	return 0;
}
