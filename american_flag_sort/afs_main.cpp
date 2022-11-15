
#include "afsort.hpp"
#include "../radix_sort/radix_sort.hpp"

#include <iostream>
#include <fstream>
#include <semaphore>
#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <ranges>
#include <random>
#include <array>
#include <thread>

import libksn.multithreading;


template<class T, class RNG>
T mydist(RNG&& engine)
{
	//std::uniform_int_distribution<T> dist(0, std::numeric_limits<T>::max());
	//return dist(engine);
	std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
	uint32_t val = dist(engine);
	return (T)std::pow((double)std::numeric_limits<T>::max(), double(val) / UINT32_MAX);
}

std::ofstream fails("fails.txt");
std::binary_semaphore output_semaphore(1);


struct noop { constexpr void operator()(){} };

template<class Callable, class Callable2 = noop>
uint64_t measure(Callable&& f, size_t times = 1, Callable2&& prep = {})
{
	using namespace std;
	using namespace chrono;
	auto clock_f = &steady_clock::now;
	uint64_t dt = UINT64_MAX;
	for (size_t i = 0; i < times; ++i)
	{
		(*std::launder(std::addressof(prep)))();
		auto t1 = clock_f();
		(*std::launder(std::addressof(f)))();
		auto t2 = clock_f();
		dt = std::min<uint64_t>(dt, duration_cast<nanoseconds>(t2 - t1).count());
	}
	return dt;
}

int main()
{
	const size_t N = 150000;
	std::vector<int> v(N);
	std::vector<int> v1(N);
	int* const arr = v.data();
	int* const arr1 = v1.data();

	std::mt19937_64 rng;
	std::uniform_int_distribution<int> dist(0, INT_MAX);

	std::ofstream fout("bench");

	for (size_t n = 1; true; ++n)
	{
		const size_t len = n * 1000;
		if (len > N)
			break;

		rng.seed(n);
		for (size_t i = 0; i < len; ++i)
			arr[i] = mydist<int>(rng);

		auto refill = [&] { memcpy(arr1, arr, sizeof(*arr) * len); };

		constexpr size_t measures = 2;
		auto dt1 = measure([&] { std::sort(v1.data(), v1.data() + len); }, measures, refill) / 1000;
		auto dt2 = measure([&] { ksn::afsort(v1.data(), v1.data() + len); }, measures, refill) / 1000;
		auto dt3 = measure([&] { ksn::radix_sort(v1.data(), v1.data() + len); }, measures, refill) / 1000;

		fout << len << 
			", " << dt1 << 
			", " << dt2 << 
			", " << dt3 << 
			'\n';
	}

	return 0;
}
