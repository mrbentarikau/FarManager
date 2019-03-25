﻿/*
common.tests.cpp

Tests for common library

*/
/*
Copyright © 2019 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef ENABLE_TESTS

#include "testing.hpp"

//-----------------------------------------------------------------------------
#include "common/bytes_view.hpp"

TEST_CASE("bytes_view")
{
	uint32_t Value = 0;
	const auto BytesRef = bytes::reference(Value);
	std::fill(BytesRef.begin(), BytesRef.end(), 0x42);
	REQUIRE(Value == 0x42424242);

	auto BytesCopy = bytes::copy(Value);
	std::fill(BytesCopy.begin(), BytesCopy.end(), 0x33);
	REQUIRE(Value == 0x42424242);
	const auto NewValue = deserialise<uint32_t>(BytesCopy);
	REQUIRE(NewValue == 0x33333333);

	uint8_t SmallerValue = 1;
	auto SmallerView = bytes_view(SmallerValue);
	auto SmallerRef = bytes::reference(SmallerValue);
	REQUIRE_THROWS_AS(BytesCopy = SmallerView, std::runtime_error);
	REQUIRE_THROWS_AS(SmallerRef = bytes_view(Value), std::runtime_error);
	REQUIRE_NOTHROW(SmallerView = BytesCopy);
}

//-----------------------------------------------------------------------------
#include "common/from_string.hpp"

TEST_CASE("from_string")
{
	{
		size_t Pos = 0;
		REQUIRE(from_string<int>(L"0x42qqq"sv, &Pos, 16) == 66);
		REQUIRE(Pos == 4u);
	}

	REQUIRE(from_string<int>(L"-1"sv) == -1);
	REQUIRE(from_string<unsigned int>(L"4294967295"sv) == 4294967295u);
	REQUIRE(from_string<unsigned long long>(L"18446744073709551615"sv) == 18446744073709551615ull);

	REQUIRE_THROWS_AS(from_string<short>(L"32768"sv), std::out_of_range);
	REQUIRE_THROWS_AS(from_string<unsigned int>(L"4294967296"sv), std::out_of_range);
	REQUIRE_THROWS_AS(from_string<int>(L"fubar"sv), std::invalid_argument);

	{
		int Value;
		REQUIRE(!from_string(L"qqq"sv, Value));
	}
}

//-----------------------------------------------------------------------------
#include "common/enum_tokens.hpp"

TEST_CASE("enum_tokens")
{
	{
		const std::array Baseline = { L"abc"sv, L""sv, L"def"sv, L" q "sv, L"123"sv };
		auto BaselineIterator = Baseline.begin();

		for (const auto& i : enum_tokens(L"abc;,def; q ,123;"sv, L",;"sv))
		{
			REQUIRE(i == *BaselineIterator++);
		}

		REQUIRE(BaselineIterator == Baseline.end());
	}

	{
		const std::array Baseline = { L"abc;"sv, L"de;,f"sv, L"123"sv, L""sv };
		auto BaselineIterator = Baseline.begin();

		for (const auto& i : enum_tokens_with_quotes(L"\"abc;\",\"de;,f\";123;;"sv, L",;"sv))
		{
			REQUIRE(i == *BaselineIterator++);
		}

		REQUIRE(BaselineIterator == Baseline.end());
	}

	{
		const std::array Baseline = { L"abc"sv, L"def"sv, L""sv };
		auto BaselineIterator = Baseline.begin();

		for (const auto& i : enum_tokens_custom_t<with_trim>(L"  abc|   def  |  "sv, L"|"sv))
		{
			REQUIRE(i == *BaselineIterator++);
		}

		REQUIRE(BaselineIterator == Baseline.end());
	}

}

//-----------------------------------------------------------------------------
#include "common/enum_substrings.hpp"

TEST_CASE("enum_substrings")
{
	const std::array Baseline = { L"abc"sv, L"def"sv, L"q"sv };
	auto BaselineIterator = Baseline.begin();

	for (const auto& i : enum_substrings(L"abc\0def\0q\0"sv.data()))
	{
		REQUIRE(i == *BaselineIterator++);
	}

	REQUIRE(BaselineIterator == Baseline.end());
}

//-----------------------------------------------------------------------------
#include "common/chrono.hpp"

TEST_CASE("chrono")
{
	using namespace std::chrono;

	const auto Duration = 17h + 63min + 71s + 3117ms;

	{
		const auto Result = split_duration<hours, minutes, seconds, milliseconds>(Duration);
		REQUIRE(Result.get<hours>() == 18h);
		REQUIRE(Result.get<minutes>() == 4min);
		REQUIRE(Result.get<seconds>() == 14s);
		REQUIRE(Result.get<milliseconds>() == 117ms);
	}

	{
		const auto Result = split_duration<hours>(Duration);
		REQUIRE(Result.get<hours>() == 18h);
	}

	{
		const auto Result = split_duration<seconds>(Duration);
		REQUIRE(Result.get<seconds>() == 65054s);
	}
}

//-----------------------------------------------------------------------------
#include "common/io.hpp"

TEST_CASE("io")
{
	std::stringstream Stream;
	constexpr const auto Str = "12345"sv;
	REQUIRE_NOTHROW(io::write(Stream, Str));

	char Buffer[Str.size()];
	REQUIRE(io::read(Stream, Buffer) == Str.size());
	REQUIRE(!io::read(Stream, Buffer));
}

//-----------------------------------------------------------------------------
#include "common/monitored.hpp"

TEST_CASE("monitored")
{
	monitored<int> a(42);
	REQUIRE(!a.touched());
	a = 33;
	REQUIRE(a.touched());
	REQUIRE(a == 33);
	a.forget();
	REQUIRE(!a.touched());
	REQUIRE(a == 33);
}

//-----------------------------------------------------------------------------
#include "common/movable.hpp"

TEST_CASE("movable")
{
	const auto Value = 42;
	movable<int> m1 = Value;
	REQUIRE(m1 == Value);

	const auto m2 = std::move(m1);
	REQUIRE(m2 == Value);
	REQUIRE(m1 == 0);
}

//-----------------------------------------------------------------------------
#include "common/null_iterator.hpp"

TEST_CASE("null_iterator")
{
	const auto Ptr = L"12345";
	const null_iterator Iterator(Ptr);
	const size_t Count = std::count_if(Iterator, Iterator.end(), [](wchar_t){ return true; });
	REQUIRE(Count == wcslen(Ptr));
}

//-----------------------------------------------------------------------------
TEST_CASE("placement")
{
	struct raii
	{
		explicit raii(int& Value):
			m_Value(Value)
		{
			m_Value = 42;
		}

		~raii()
		{
			m_Value = 33;
		}

		int& m_Value;
	};

	std::aligned_storage_t<sizeof(raii), alignof(raii)> Data;
	auto& Object = reinterpret_cast<raii&>(Data);

	int Value = 0;
	placement::construct(Object, Value);
	REQUIRE(Value == 42);

	placement::destruct(Object);
	REQUIRE(Value == 33);
}

//-----------------------------------------------------------------------------
#include "common/range.hpp"

TEST_CASE("range")
{
	{
		std::array Value{ 1, 2, 3, 4, 5 };
		range Range(Value);
		REQUIRE(Range.size() == Value.size());
		REQUIRE(Range.data() == Value.data());

		Range.pop_front();
		REQUIRE(Range.size() == Value.size() - 1);
		REQUIRE(Range.data() == Value.data() + 1);

		Range.pop_back();
		REQUIRE(Range.size() == Value.size() - 2);
		REQUIRE(Range.data() == Value.data() + 1);
	}

	{
		const auto IRange = make_irange(5);
		auto Iterator = IRange.begin();
		for (int i = 0; i != 5; ++i, ++Iterator)
		{
			REQUIRE(*Iterator == i);
		}
	}

	{
		const std::pair<bool, int> Data[]
		{
			{ true, 42 },
			{ false, 33 },
		};

		const auto Selector = select(Data, [](const auto& i){ return i.second; });
		auto Iterator = Selector.begin();
		REQUIRE(*Iterator == 42);
		++Iterator;
		REQUIRE(*Iterator == 33);
		++Iterator;
		REQUIRE(Iterator == Selector.end());
	}
}

//-----------------------------------------------------------------------------
#include "common/scope_exit.hpp"

template<scope_exit::scope_type type>
static void TestScope(bool Throw, int Expected)
{
	int Result;
	try
	{
		Result = 0;
		const auto Guard = scope_exit::make_scope_guard<type>() << [&](){ Result = 42; };
		REQUIRE(Result == 0);
		Result = 1;
		if (Throw)
			throw 1;
	}
	catch (int) {}
	REQUIRE(Result == Expected);
};

TEST_CASE("scope_exit")
{
	TestScope<scope_exit::scope_type::exit>(false, 42);
	TestScope<scope_exit::scope_type::exit>(true, 42);

	TestScope<scope_exit::scope_type::fail>(false, 1);
	TestScope<scope_exit::scope_type::fail>(true, 42);

	TestScope<scope_exit::scope_type::success>(false, 42);
	TestScope<scope_exit::scope_type::success>(true, 1);
}

//-----------------------------------------------------------------------------
#include "common/string_utils.hpp"

TEST_CASE("string_utils")
{
	REQUIRE(!null_terminated(L"12345"sv.substr(0, 2)).c_str()[2]);

	REQUIRE(concat(L'a', L"bc", L"def"sv, L"1234"s) == L"abcdef1234"sv);

	REQUIRE(cut_left(L"12345"sv, 3) == L"345"sv);
	REQUIRE(cut_left(L"12345"sv, 6) == L"12345"sv);

	REQUIRE(cut_right(L"12345"sv, 3) == L"123"sv);
	REQUIRE(cut_right(L"12345"sv, 6) == L"12345"sv);

	REQUIRE(pad_left(L"123"s, 5) == L"  123"sv);
	REQUIRE(pad_left(L"123"s, 2) == L"123"sv);

	REQUIRE(pad_right(L"123"s, 5) == L"123  "sv);
	REQUIRE(pad_right(L"123"s, 2) == L"123"sv);

	REQUIRE(fit_to_left(L"12345"s, 3) == L"123"sv);
	REQUIRE(fit_to_left(L"12345"s, 5) == L"12345"sv);
	REQUIRE(fit_to_left(L"12345"s, 7) == L"12345  "sv);

	REQUIRE(fit_to_center(L"12345"s, 3) == L"123"sv);
	REQUIRE(fit_to_center(L"12345"s, 5) == L"12345"sv);
	REQUIRE(fit_to_center(L"12345"s, 7) == L" 12345 "sv);

	REQUIRE(fit_to_right(L"12345"s, 3) == L"123"sv);
	REQUIRE(fit_to_right(L"12345"s, 5) == L"12345"sv);
	REQUIRE(fit_to_right(L"12345"s, 7) == L"  12345"sv);

	REQUIRE(erase_all(L"1,2,3,4,5"s, L',') == L"12345"sv);
	REQUIRE(erase_all(L"12345"s, L',') == L"12345"sv);

	REQUIRE(unquote(L"\"12\"345\""s) == L"12345"sv);
	REQUIRE(unquote(L"12345"s) == L"12345"sv);

	REQUIRE(quote(L"12345"s) == L"\"12345\""sv);
	REQUIRE(quote(L"\"12345\""s) == L"\"12345\""sv);

	REQUIRE(quote_unconditional(L"\"12345\""s) == L"\"\"12345\"\""sv);
	REQUIRE(quote_unconditional(L"12345"s) == L"\"12345\""sv);

	REQUIRE(quote_normalise(L"\"12\"345\""s) == L"\"12345\""sv);
	REQUIRE(quote_normalise(L"12345"s) == L"\"12345\""sv);

	REQUIRE(starts_with(L"12345"sv, L"123"sv));
	REQUIRE(starts_with(L"12345"sv, L'1'));
	REQUIRE(!starts_with(L"12345"sv, L"345"sv));
	REQUIRE(!starts_with(L"12345"sv, L'3'));
	REQUIRE(!starts_with(L""sv, L'3'));

	REQUIRE(ends_with(L"12345"sv, L"345"sv));
	REQUIRE(ends_with(L"12345"sv, L'5'));
	REQUIRE(!ends_with(L"12345"sv, L"123"sv));
	REQUIRE(!ends_with(L"12345"sv, L'3'));
	REQUIRE(!ends_with(L""sv, L'3'));

	REQUIRE(trim_left(L"12345"sv) == L"12345"sv);
	REQUIRE(trim_left(L"  12345"sv) == L"12345"sv);
	REQUIRE(trim_left(L"12345  "sv) == L"12345  "sv);
	REQUIRE(trim_left(L"  12345  "sv) == L"12345  "sv);

	REQUIRE(trim_right(L"12345"sv) == L"12345"sv);
	REQUIRE(trim_right(L"  12345"sv) == L"  12345"sv);
	REQUIRE(trim_right(L"12345  "sv) == L"12345"sv);
	REQUIRE(trim_right(L"  12345  "sv) == L"  12345"sv);

	REQUIRE(trim(L"12345"sv) == L"12345"sv);
	REQUIRE(trim(L"  12345"sv) == L"12345"sv);
	REQUIRE(trim(L"12345  "sv) == L"12345"sv);
	REQUIRE(trim(L"  12345  "sv) == L"12345"sv);

	REQUIRE(join(std::array{ L"123"sv, L"45"sv, L"6"sv }, L","sv) == L"123,45,6"sv);

	REQUIRE(contains(L"12345"sv, L"123"sv));
	REQUIRE(contains(L"12345"sv, L'5'));
	REQUIRE(!contains(L"12345"sv, L'R'));

	REQUIRE(L"123"s + L"45"sv == L"12345"sv);
	REQUIRE(L"123"sv + L"45"s == L"12345"sv);
	REQUIRE(L"123"sv + L"45"sv == L"12345"sv);

	const auto Str = L"12345"sv;
	REQUIRE(make_string_view(Str.begin() + 1, Str.end() - 1) == L"234"sv);
}

//-----------------------------------------------------------------------------
#include "common/utility.hpp"

TEMPLATE_TEST_CASE("utility.reserve_exp_noshrink", "", string, std::vector<int>)
{
	TestType Container;
	Container.resize(42);
	const auto InitialCapacity = Container.capacity();
	const auto GrowthFactor = 1.5;

	SECTION("no shrink")
	{
		reserve_exp_noshrink(Container, 1);
		REQUIRE(Container.capacity() >= InitialCapacity);
	}

	SECTION("exponential < factor")
	{
		reserve_exp_noshrink(Container, InitialCapacity * 1.1);
		REQUIRE(Container.capacity() >= InitialCapacity * GrowthFactor);
	}

	SECTION("exponential > factor")
	{
		reserve_exp_noshrink(Container, InitialCapacity * 2);
		REQUIRE(Container.capacity() >= InitialCapacity * 2);
	}
}

TEMPLATE_TEST_CASE("utility.clear_and_shrink", "", string, std::vector<int>)
{
	TestType Container;
	Container.resize(42);
	clear_and_shrink(Container);
	REQUIRE(!Container.size());
	REQUIRE(Container.capacity() == TestType{}.capacity());
}

TEST_CASE("utility")
{
	std::list List{ 1, 2, 3, 4, 5 };
	const auto Ptr1 = &List.front();
	const auto Ptr5 = &List.back();
	node_swap(List, List.begin(), std::prev(List.end()));
	REQUIRE(Ptr1 == &List.back());
	REQUIRE(Ptr5 == &List.front());
	REQUIRE(*Ptr1 == 1);
	REQUIRE(*Ptr5 == 5);
}

//-----------------------------------------------------------------------------
#include "common/zip_view.hpp"

TEST_CASE("zip")
{
	const std::array Source      { 1, 2, 3 };
	      std::array Destination { 9, 8, 7, 6, 5 };
	const std::array Baseline    { 1, 2, 3, 6, 5 };

	for (const auto& [Src, Dst] : zip(Source, Destination))
	{
		Dst = Src;
	}

	REQUIRE(Destination == Baseline);
}
#endif
