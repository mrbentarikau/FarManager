﻿/*
datetime.cpp

Функции для работы с датой и временем
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
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

// Self:
#include "datetime.hpp"

// Internal:
#include "config.hpp"
#include "strmix.hpp"
#include "global.hpp"
#include "imports.hpp"
#include "locale.hpp"
#include "encoding.hpp"

// Platform:

// Common:
#include "common/chrono.hpp"
#include "common/from_string.hpp"

// External:
#include "format.hpp"

//----------------------------------------------------------------------------

DWORD ConvertYearToFull(DWORD ShortYear)
{
	DWORD UpperBoundary = 0;
	if(!GetCalendarInfo(LOCALE_USER_DEFAULT, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX|CAL_RETURN_NUMBER, nullptr, 0, &UpperBoundary))
	{
		UpperBoundary = 2029; // Magic, current default value.
	}
	return (UpperBoundary/100-(ShortYear<UpperBoundary%100?0:1))*100+ShortYear;
}

static string st_time(const tm* tmPtr, const locale_names& Names, bool const is_dd_mmm_yyyy)
{
	const auto DateSeparator = locale.date_separator();

	if (is_dd_mmm_yyyy)
	{
		return format(FSTR(L"{0:2}-{1:3.3}-{2:4}"),
			tmPtr->tm_mday,
			upper(Names.Months[tmPtr->tm_mon].Short),
			tmPtr->tm_year + 1900);
	}

	const auto Format = [&](const auto FormatString)
	{
		return format(FormatString, DateSeparator, tmPtr->tm_mday, tmPtr->tm_mon + 1, tmPtr->tm_year + 1900);
	};

	switch(locale.date_format())
	{
	case 0:  return Format(FSTR(L"{2:02}{0}{1:02}{0}{3:4}"));
	case 1:  return Format(FSTR(L"{1:02}{0}{2:02}{0}{3:4}"));
	default: return Format(FSTR(L"{3:4}{0}{2:02}{0}{1:02}"));
	}
}

string StrFTime(string_view const Format, const tm* Time)
{
	bool IsLocal = false;

	string Result;

	for (auto Iterator = Format.cbegin(); Iterator != Format.cend(); ++Iterator)
	{
		if (*Iterator != L'%')
		{
			Result.push_back(*Iterator);
			continue;
		}

		++Iterator;
		if (Iterator == Format.cend())
			break;

		switch (*Iterator)
		{
		case L'%':
			Result.push_back(L'%');
			break;

		case L'L':
			IsLocal = !IsLocal;
			continue;

		// Краткое имя дня недели (Sun,Mon,Tue,Wed,Thu,Fri,Sat)
		// abbreviated weekday name
		case L'a':
			Result += locale.Names(IsLocal).Weekdays[Time->tm_wday].Short;
			break;

		// Полное имя дня недели
		// full weekday name
		case L'A':
			Result += locale.Names(IsLocal).Weekdays[Time->tm_wday].Full;
			break;

		// Краткое имя месяца (Jan,Feb,...)
		// abbreviated month name
		case L'h':
		case L'b':
			Result += locale.Names(IsLocal).Months[Time->tm_mon].Short;
			break;

		// Полное имя месяца
		// full month name
		case L'B':
			Result += locale.Names(IsLocal).Months[Time->tm_mon].Full;
			break;

		//Дата и время в формате WDay Mnt  Day HH:MM:SS yyyy
		//appropriate date and time representation
		case L'c':
			// Thu Oct 07 12:37:32 1999
			Result += format(FSTR(L"{0} {1} {2:02} {3:02}:{4:02}:{5:02} {6:4}"),
				locale.Names(IsLocal).Weekdays[Time->tm_wday].Short,
				locale.Names(IsLocal).Months[Time->tm_mon].Short,
				Time->tm_mday, Time->tm_hour, Time->tm_min, Time->tm_sec, Time->tm_year + 1900);
			break;

		// Столетие как десятичное число (00 - 99). Например, 1992 => 19
		case L'C':
			Result += format(FSTR(L"{0:02}"), (Time->tm_year + 1900) / 100);
			break;

		// day of month, blank padded
		case L'e':
		// Две цифры дня месяца (01 - 31)
		// day of the month, 01 - 31
		case L'd':
			Result += *Iterator == L'e'?
				format(FSTR(L"{0:2}"), Time->tm_mday) :
				format(FSTR(L"{0:02}"), Time->tm_mday);
			break;

		// hour, 24-hour clock, blank pad
		case L'k':
		// Две цифры часа (00 - 23)
		// hour, 24-hour clock, 00 - 23
		case L'H':
			Result += *Iterator == L'k'?
				format(FSTR(L"{0:2}"), Time->tm_hour) :
				format(FSTR(L"{0:02}"), Time->tm_hour);
			break;

		// hour, 12-hour clock, 1 - 12, blank pad
		case L'l':
		// Две цифры часа (01 - 12)
		// hour, 12-hour clock, 01 - 12
		case L'I':
		{
			int I = Time->tm_hour % 12;

			if (!I)
				I=12;

			Result += *Iterator == L'l'?
				format(FSTR(L"{0:2}"), I) :
				format(FSTR(L"{0:02}"), I);
			break;
		}

		// Три цифры дня в году (001 - 366)
		// day of the year, 001 - 366
		case L'j':
			Result += format(FSTR(L"{0:03}"), Time->tm_yday+1);
			break;

		// Две цифры месяца, как десятичное число (1 - 12)
		// month, 01 - 12
		case L'm':
			++Iterator;
			if (Iterator == Format.cend())
				break;

			switch(*Iterator)
			{
			// %mh - Hex month digit
			case L'h':
				Result += format(FSTR(L"{0:X}"), Time->tm_mon + 1);
				break;

			// %m0 - ведущий 0
			case L'0':
				Result += format(FSTR(L"{0:02}"), Time->tm_mon + 1);
				break;

			default:
				--Iterator;
				Result += format(FSTR(L"{0}"), Time->tm_mon + 1);
				break;
			}
			break;

		// Две цифры минут (00 - 59)
		// minute, 00 - 59
		case L'M':
			Result += format(FSTR(L"{0:02}"), Time->tm_min);
			break;

		// AM или PM
		// am or pm based on 12-hour clock
		case L'p':
			Result += Time->tm_hour / 12? L"PM"sv : L"AM"sv;
			break;

		// Две цифры секунд (00 - 59)
		// second, 00 - 59
		case L'S':
			Result += format(FSTR(L"{0:02}"), Time->tm_sec);
			break;

		// День недели где 0 - Воскресенье (Sunday) (0 - 6)
		// weekday, Sunday == 0, 0 - 6
		case L'w':
			Result.push_back(L'0' + Time->tm_wday);
			break;

		// Две цифры номера недели, где Воскресенье (Sunday) является первым днем недели (00 - 53)
		// week of year, Sunday is first day of week
		case L'U':
		// Две цифры номера недели, где Понедельник (Monday) является первым днем недели (00 - 53)
		// week of year, Monday is first day of week
		case L'W':
		{
			int I = Time->tm_wday - (Time->tm_yday % 7);

			//I = (chr == 'W'?(!WeekFirst?((t->tm_wday+6)%7):(t->tm_wday? t->tm_wday-1:6)):(t->tm_wday)) - (t->tm_yday % 7);
			if (I<0)
				I+=7;

			Result += format(FSTR(L"{0:02}"), (Time->tm_yday + I - (*Iterator == L'W')) / 7);
			break;
		}

		// date as dd-bbb-YYYY
		case L'v':
		// Дата в формате mm.dd.yyyy
		// appropriate date representation
		case L'D':
		case L'x':
			Result += st_time(Time, locale.Names(IsLocal), *Iterator == L'v');
			break;

		// Время в формате HH:MM:SS
		// appropriate time representation
		case L'T':
		case L'X':
			Result += format(FSTR(L"{1:02}{0}{2:02}{0}{3:02}"), locale.time_separator(), Time->tm_hour, Time->tm_min, Time->tm_sec);
			break;

		// Две цифры года без столетия (00 to 99)
		// year without a century, 00 - 99
		case L'y':
			Result += format(FSTR(L"{0:02}"), Time->tm_year % 100);
			break;

		// Год со столетием (19yy-20yy)
		// year with century
		case L'Y':
			Result += str(1900 + Time->tm_year);
			break;

		// ISO 8601 offset from UTC in timezone
		case L'z':
			{
				using namespace std::chrono;
				const auto Offset = split_duration<hours, minutes>(-seconds(_timezone + (Time->tm_isdst? _dstbias : 0)));
				Result += format(FSTR(L"{0:+05}"), Offset.get<hours>().count() * 100 + Offset.get<minutes>().count());
			}
			break;

		// Timezone name or abbreviation
		case L'Z':
			Result += encoding::ansi::get_chars(_tzname[Time->tm_isdst]);
			break;

		// same as \n
		case L'n':
			Result.push_back(L'\n');
			break;

		// same as \t
		case L't':
			Result.push_back(L'\t');
			break;

		// time as %I:%M:%S %p
		case L'r':
			Result += StrFTime(L"%I:%M:%S %p"sv, Time);
			break;

		// time as %H:%M
		case L'R':
			Result += StrFTime(L"%H:%M"sv, Time);
			break;

		// week of year according to ISO 8601
		case L'V':
			{
				// [01,53]
				wchar_t Buffer[3];
				std::wcsftime(Buffer, std::size(Buffer), L"%V", Time);
				Result += Buffer;
			}
			break;
		}

		if (Iterator == Format.cend())
			break;
	}

	return Result;
}

string MkStrFTime(string_view const Format)
{
	const auto Time = os::chrono::nt_clock::to_time_t(os::chrono::nt_clock::now());

	_tzset();
	return StrFTime(Format.empty()? Global->Opt->Macro.strDateFormat : Format, std::localtime(&Time));
}

void ParseTimeComponents(string_view const Src, span<const std::pair<size_t, size_t>> const Ranges, span<time_component> const Dst, time_component const Default)
{
	assert(Dst.size() == Ranges.size());
	std::transform(ALL_CONST_RANGE(Ranges), Dst.begin(), [Src, Default](const auto& i)
	{
		const auto Part = trim(Src.substr(i.first, i.second));
		return Part.empty()? Default : from_string<time_component>(Part);
	});
}

os::chrono::time_point ParseTimePoint(const string& Date, const string& Time, int DateFormat, const date_ranges& DateRanges, const time_ranges& TimeRanges)
{
	time_component DateN[3];
	ParseTimeComponents(Date, DateRanges, DateN);
	time_component TimeN[5];
	ParseTimeComponents(Time, TimeRanges, TimeN, 0);

	if (DateN[0] == time_none || DateN[1] == time_none || DateN[2] == time_none)
	{
		// Пользователь оставил дату пустой, значит обнулим дату и время.
		return {};
	}

	SYSTEMTIME st{};

	switch (DateFormat)
	{
	case 0:
		st.wMonth = DateN[0];
		st.wDay   = DateN[1];
		st.wYear  = DateN[2];
		break;

	case 1:
		st.wDay   = DateN[0];
		st.wMonth = DateN[1];
		st.wYear  = DateN[2];
		break;

	default:
		st.wYear  = DateN[0];
		st.wMonth = DateN[1];
		st.wDay   = DateN[2];
		break;
	}

	if (st.wYear < 100)
	{
		st.wYear = static_cast<WORD>(ConvertYearToFull(st.wYear));
	}

	st.wHour         = TimeN[0];
	st.wMinute       = TimeN[1];
	st.wSecond       = TimeN[2];
	st.wMilliseconds = TimeN[3];

	os::chrono::time_point Point;
	local_to_utc(st, Point);
	return Point + os::chrono::duration(TimeN[4]);
}

os::chrono::duration ParseDuration(const string& Date, const string& Time, const time_ranges& TimeRanges)
{
	time_component DateN[1];
	const std::pair<size_t, size_t> DateRange[]{ { 0, Date.size() } };
	ParseTimeComponents(Date, DateRange, DateN, 0);

	time_component TimeN[5];
	ParseTimeComponents(Time, TimeRanges, TimeN, 0);

	using namespace std::chrono;
	return chrono::days(DateN[0]) + hours(TimeN[0]) + minutes(TimeN[1]) + seconds(TimeN[2]) + milliseconds(TimeN[3]) + os::chrono::duration(TimeN[4]);
}

void ConvertDate(os::chrono::time_point const Point, string& strDateText, string& strTimeText, int const TimeLength, int const FullYear, bool const Brief, bool const TextMonth)
{
	if (Point == os::chrono::time_point{})
	{
		strDateText.clear();
		strTimeText.clear();
		return;
	}

	const auto DateFormat = locale.date_format();
	const auto DateSeparator = locale.date_separator();
	const auto TimeSeparator = locale.time_separator();
	const auto DecimalSeparator = locale.decimal_separator();

	const auto CurDateFormat = Brief && DateFormat == 2? 0 : DateFormat;

	SYSTEMTIME st;
	if (!utc_to_local(Point, st))
		return;

	auto Letter = L""sv;

	if (TimeLength == 6)
	{
		Letter = st.wHour < 12 ? L"a"sv : L"p"sv;

		if (st.wHour > 12)
			st.wHour -= 12;

		if (!st.wHour)
			st.wHour = 12;
	}

	if (TimeLength < 7)
	{
		strTimeText = format(FSTR(L"{0:02}{1}{2:02}{3}"), st.wHour, TimeSeparator, st.wMinute, Letter);
	}
	else
	{
		strTimeText = cut_right(
			format(
				FSTR(L"{0:02}{1}{2:02}{1}{3:02}{4}{5:03}+{6:04}"),
				st.wHour,
				TimeSeparator,
				st.wMinute,
				st.wSecond,
				DecimalSeparator,
				st.wMilliseconds,
				(Point.time_since_epoch() % 1ms).count()
			),
			TimeLength);
	}

	const auto Year = FullYear? st.wYear : st.wYear % 100;

	if (TextMonth)
	{
		const auto Format = [&](const auto FormatString)
		{
			strDateText = format(FormatString, st.wDay, locale.LocalNames().Months[st.wMonth - 1].Short, Year);
		};

		switch (CurDateFormat)
		{
		case 0:  Format(FSTR(L"{1:3.3} {0:2} {2:02}")); break;
		case 1:  Format(FSTR(L"{0:2} {1:3.3} {2:02}")); break;
		default: Format(FSTR(L"{2:02} {1:3.3} {0:2}")); break;
		}

	}
	else
	{
		int p1, p2, p3 = Year;
		int w1 = 2, w2 = 2, w3 = 2;
		wchar_t f1 = L'0', f2 = L'0', f3 = FullYear == 2? L' ' : L'0';
		switch (CurDateFormat)
		{
		case 0:
			p1 = st.wMonth;
			p2 = st.wDay;
			break;
		case 1:
			p1 = st.wDay;
			p2 = st.wMonth;
			break;
		default:
			p1 = Year;
			w1 = FullYear == 2?5:2;
			using std::swap;
			swap(f1, f3);
			p2 = st.wMonth;
			p3 = st.wDay;
			break;
		}

		// Library doesn't support dynamic fill currently. TODO: fix it?
		wchar_t Format[] = L"{0: >{1}}{6}{2: >{3}}{6}{4: >{5}}";
		Format[3] = f1;
		Format[15] = f2;
		Format[27] = f3;
		strDateText = format(Format, p1, w1, p2, w2, p3, w3, DateSeparator);
	}

	if (Brief)
	{
		strDateText.resize(TextMonth? 6 : 5);

		SYSTEMTIME Now;
		if (utc_to_local(os::chrono::nt_clock::now(), Now) && Now.wYear != st.wYear)
			strTimeText = format(FSTR(L"{0:5}"), st.wYear);
	}
}

std::tuple<string, string> ConvertDuration(os::chrono::duration Duration)
{
	using namespace std::chrono;

	const auto Result = split_duration<chrono::days, hours, minutes, seconds, milliseconds>(Duration);

	return
	{
		str(Result.get<chrono::days>().count()),
		format(FSTR(L"{0:02}{5}{1:02}{5}{2:02}{6}{3:03}+{4:04}"),
			Result.get<hours>().count(),
			Result.get<minutes>().count(),
			Result.get<seconds>().count(),
			Result.get<milliseconds>().count(),
			(Duration % 1ms).count(),
			locale.time_separator(),
			locale.decimal_separator()
		)
	};
}

string ConvertDurationToHMS(os::chrono::duration Duration)
{
	using namespace std::chrono;

	const auto Result = split_duration<hours, minutes, seconds, milliseconds>(Duration);

	return format(FSTR(L"{0:02}{3}{1:02}{3}{2:02}"),
		Result.get<hours>().count(),
		Result.get<minutes>().count(),
		Result.get<seconds>().count(),
		locale.time_separator()
	);
}

bool utc_to_local(os::chrono::time_point UtcTime, SYSTEMTIME& LocalTime)
{
	const auto FileTime = os::chrono::nt_clock::to_filetime(UtcTime);
	SYSTEMTIME SystemTime;
	return FileTimeToSystemTime(&FileTime, &SystemTime) && SystemTimeToTzSpecificLocalTime(nullptr, &SystemTime, &LocalTime);
}

static bool local_to_utc(const SYSTEMTIME &lst, SYSTEMTIME &ust)
{
	if (imports.TzSpecificLocalTimeToSystemTime && imports.TzSpecificLocalTimeToSystemTime(nullptr, &lst, &ust))
		return true;

	TIME_ZONE_INFORMATION Tz;
	if (GetTimeZoneInformation(&Tz) != TIME_ZONE_ID_INVALID)
	{
		Tz.Bias = -Tz.Bias;
		Tz.StandardBias = -Tz.StandardBias;
		Tz.DaylightBias = -Tz.DaylightBias;
		if (SystemTimeToTzSpecificLocalTime(&Tz, &lst, &ust))
			return true;
	}

	std::tm ltm
	{
		lst.wSecond,
		lst.wMinute,
		lst.wHour,
		lst.wDay,
		lst.wMonth - 1,
		lst.wYear - 1900,
		lst.wDayOfWeek,
		-1,
		-1
	};

	if (const auto gtim = std::mktime(&ltm); gtim != static_cast<time_t>(-1))
	{
		if (const auto ptm = std::gmtime(&gtim))
		{
			ust.wYear = ptm->tm_year + 1900;
			ust.wMonth = ptm->tm_mon + 1;
			ust.wDay = ptm->tm_mday;
			ust.wHour = ptm->tm_hour;
			ust.wMinute = ptm->tm_min;
			ust.wSecond = ptm->tm_sec;
			ust.wDayOfWeek = ptm->tm_wday;
			ust.wMilliseconds = lst.wMilliseconds;
			return true;
		}
	}

	FILETIME lft, uft;
	return SystemTimeToFileTime(&lst, &lft) && LocalFileTimeToFileTime(&lft, &uft) && FileTimeToSystemTime(&uft, &ust);
}

bool local_to_utc(const SYSTEMTIME& LocalTime, os::chrono::time_point& UtcTime)
{
	SYSTEMTIME SystemUtcTime;
	if (!local_to_utc(LocalTime, SystemUtcTime))
		return false;

	FILETIME FileUtcTime;
	if (!SystemTimeToFileTime(&SystemUtcTime, &FileUtcTime))
		return false;

	UtcTime = os::chrono::nt_clock::from_filetime(FileUtcTime);
	return true;
}
