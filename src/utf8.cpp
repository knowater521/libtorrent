/*

Copyright (c) 2012, 2015-2019, Arvid Norberg
Copyright (c) 2016-2017, 2019, Alden Torres
Copyright (c) 2017, Andrei Kurushin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "libtorrent/config.hpp"

#include <iterator>
#include "libtorrent/utf8.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/error_code.hpp"
#include "libtorrent/ConvertUTF.h"
#include "libtorrent/aux_/throw.hpp"
#include "libtorrent/aux_/numeric_cast.hpp"


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-member-function"
#endif

namespace libtorrent {

namespace {

		// ==== utf-8 -> wide ===
		template<int width>
		struct convert_to_wide
		{
			static utf8_errors::error_code_enum convert(UTF8 const** src_start
				, UTF8 const* src_end
				, std::wstring& wide)
			{
				TORRENT_UNUSED(src_start);
				TORRENT_UNUSED(src_end);
				TORRENT_UNUSED(wide);
				return utf8_errors::error_code_enum::source_illegal;
			}
		};

		// ==== utf-8 -> utf-32 ===
		template<>
		struct convert_to_wide<4>
		{
			static utf8_errors::error_code_enum convert(char const** src_start
				, char const* src_end
				, std::wstring& wide)
			{
				wchar_t* dst_start = &wide[0];
				int ret = ConvertUTF8toUTF32(
					reinterpret_cast<UTF8 const**>(src_start)
					, reinterpret_cast<UTF8 const*>(src_end)
					, reinterpret_cast<UTF32**>(&dst_start)
					, reinterpret_cast<UTF32*>(dst_start + wide.size())
					, lenientConversion);
				if (ret == sourceIllegal)
				{
					// assume Latin-1
					wide.clear();
					std::copy(reinterpret_cast<std::uint8_t const*>(*src_start)
						,reinterpret_cast<std::uint8_t const*>(src_end)
						, std::back_inserter(wide));
					return static_cast<utf8_errors::error_code_enum>(ret);
				}
				wide.resize(aux::numeric_cast<std::size_t>(dst_start - wide.c_str()));
				return static_cast<utf8_errors::error_code_enum>(ret);
			}
		};

		// ==== utf-8 -> utf-16 ===
		template<>
		struct convert_to_wide<2>
		{
			static utf8_errors::error_code_enum convert(char const** src_start
				, char const* src_end
				, std::wstring& wide)
			{
				wchar_t* dst_start = &wide[0];
				int ret = ConvertUTF8toUTF16(
					reinterpret_cast<UTF8 const**>(src_start)
					, reinterpret_cast<UTF8 const*>(src_end)
					, reinterpret_cast<UTF16**>(&dst_start)
					, reinterpret_cast<UTF16*>(dst_start + wide.size())
					, lenientConversion);
				if (ret == sourceIllegal)
				{
					// assume Latin-1
					wide.clear();
					std::copy(reinterpret_cast<std::uint8_t const*>(*src_start)
						, reinterpret_cast<std::uint8_t const*>(src_end)
						, std::back_inserter(wide));
					return static_cast<utf8_errors::error_code_enum>(ret);
				}
				wide.resize(aux::numeric_cast<std::size_t>(dst_start - wide.c_str()));
				return static_cast<utf8_errors::error_code_enum>(ret);
			}
		};

		// ==== wide -> utf-8 ===
		template<int width>
		struct convert_from_wide
		{
			static utf8_errors::error_code_enum convert(wchar_t const** src_start
				, wchar_t const* src_end
				, std::string& utf8)
			{
				TORRENT_UNUSED(src_start);
				TORRENT_UNUSED(src_end);
				TORRENT_UNUSED(utf8);
				return utf8_errors::error_code_enum::source_illegal;
			}
		};

		// ==== utf-32 -> utf-8 ===
		template<>
		struct convert_from_wide<4>
		{
			static utf8_errors::error_code_enum convert(wchar_t const** src_start
				, wchar_t const* src_end
				, std::string& utf8)
			{
				char* dst_start = &utf8[0];
				int ret = ConvertUTF32toUTF8(
					reinterpret_cast<UTF32 const**>(src_start)
					, reinterpret_cast<UTF32 const*>(src_end)
					, reinterpret_cast<UTF8**>(&dst_start)
					, reinterpret_cast<UTF8*>(dst_start + utf8.size())
					, lenientConversion);
				utf8.resize(aux::numeric_cast<std::size_t>(dst_start - &utf8[0]));
				return static_cast<utf8_errors::error_code_enum>(ret);
			}
		};

		// ==== utf-16 -> utf-8 ===
		template<>
		struct convert_from_wide<2>
		{
			static utf8_errors::error_code_enum convert(wchar_t const** src_start
				, wchar_t const* src_end
				, std::string& utf8)
			{
				char* dst_start = &utf8[0];
				int ret = ConvertUTF16toUTF8(
					reinterpret_cast<UTF16 const**>(src_start)
					, reinterpret_cast<UTF16 const*>(src_end)
					, reinterpret_cast<UTF8**>(&dst_start)
					, reinterpret_cast<UTF8*>(dst_start + utf8.size())
					, lenientConversion);
				utf8.resize(aux::numeric_cast<std::size_t>(dst_start - &utf8[0]));
				return static_cast<utf8_errors::error_code_enum>(ret);
			}
		};

		struct utf8_error_category final : boost::system::error_category
		{
			const char* name() const BOOST_SYSTEM_NOEXCEPT override
			{
				return "UTF error";
			}

			std::string message(int ev) const override
			{
				static char const* error_messages[] = {
					"ok",
					"source exhausted",
					"target exhausted",
					"source illegal"
				};

				TORRENT_ASSERT(ev >= 0);
				TORRENT_ASSERT(ev < int(sizeof(error_messages)/sizeof(error_messages[0])));
				return error_messages[ev];
			}

			boost::system::error_condition default_error_condition(
				int ev) const BOOST_SYSTEM_NOEXCEPT override
			{ return {ev, *this}; }
		};

	// return the number of bytes in the UTF-8 sequence starting with this
	// character. Returns 0 if the lead by is invalid
	int utf8_sequence_length(char const c)
	{
		auto const b = static_cast<std::uint8_t>(c);
		if (b < 0b10000000) return 1;
		if ((b >> 5) == 0b110) return 2;
		if ((b >> 4) == 0b1110) return 3;
		if ((b >> 3) == 0b11110) return 4;
		// this is an invalid prefix, but we still parse it to skip this many
		// bytes
		if ((b >> 2) == 0b111110) return 5;
		return 0;
	}

} // anonymous namespace

	namespace utf8_errors
	{
		boost::system::error_code make_error_code(utf8_errors::error_code_enum e)
		{
			return {e, utf8_category()};
		}
	} // utf_errors namespace

	boost::system::error_category const& utf8_category()
	{
		static utf8_error_category cat;
		return cat;
	}

	std::wstring utf8_wchar(string_view utf8, error_code& ec)
	{
		// allocate space for worst-case
		std::wstring wide;
		wide.resize(utf8.size());
		char const* src_start = utf8.data();
		utf8_errors::error_code_enum const ret = convert_to_wide<sizeof(wchar_t)>::convert(
			&src_start, src_start + utf8.size(), wide);
		if (ret != utf8_errors::error_code_enum::conversion_ok)
			ec = make_error_code(ret);
		return wide;
	}

	std::wstring utf8_wchar(string_view utf8)
	{
		error_code ec;
		std::wstring ret = utf8_wchar(utf8, ec);
		if (ec) aux::throw_ex<system_error>(ec);
		return ret;
	}

	std::string wchar_utf8(wstring_view wide, error_code& ec)
	{
		// allocate space for worst-case
		std::string utf8;
		utf8.resize(wide.size() * 6);
		if (wide.empty()) return {};

		wchar_t const* src_start = wide.data();
		utf8_errors::error_code_enum const ret = convert_from_wide<sizeof(wchar_t)>::convert(
			&src_start, src_start + wide.size(), utf8);
		if (ret != utf8_errors::error_code_enum::conversion_ok)
			ec = make_error_code(ret);
		return utf8;
	}

	std::string wchar_utf8(wstring_view wide)
	{
		error_code ec;
		std::string ret = wchar_utf8(wide, ec);
		if (ec) aux::throw_ex<system_error>(ec);
		return ret;
	}

	std::int32_t const max_codepoint = 0x10ffff;

	void append_utf8_codepoint(std::string& ret, std::int32_t codepoint)
	{
		std::int32_t const surrogate_start = 0xd800;
		std::int32_t const surrogate_end = 0xdfff;

		if (codepoint >= surrogate_start
			&& codepoint <= surrogate_end)
			codepoint = '_';

		if (codepoint > max_codepoint)
			codepoint = '_';

		int seq_len = 0;
		if (codepoint < 0x80) seq_len = 1;
		else if (codepoint < 0x800) seq_len = 2;
		else if (codepoint < 0x10000) seq_len = 3;
		else seq_len = 4;

		switch (seq_len)
		{
			case 1:
				ret.push_back(static_cast<char>(codepoint));
				break;
			case 2:
				ret.push_back(static_cast<char>(0b11000000 | (codepoint >> 6)));
				break;
			case 3:
				ret.push_back(static_cast<char>(0b11100000 | (codepoint >> 12)));
				break;
			case 4:
				ret.push_back(static_cast<char>(0b11110000 | (codepoint >> 18)));
				break;
		}

		for (int i = seq_len - 2; i >= 0; --i)
			ret.push_back(static_cast<char>(0b10000000 | ((codepoint >> (6 * i)) & 0b111111)));
	}

	// returns the unicode codepoint and the number of bytes of the utf8 sequence
	// that was parsed. The codepoint is -1 if it's invalid
	std::pair<std::int32_t, int> parse_utf8_codepoint(string_view str)
	{
		TORRENT_ASSERT(!str.empty());
		if (str.empty()) return std::make_pair(-1, 0);

		int const sequence_len = utf8_sequence_length(str[0]);

		// this is likely the most common case
		if (sequence_len == 1) return std::make_pair(std::int32_t(str[0]), sequence_len);

		// if we find an invalid sequence length, skip one byte
		if (sequence_len == 0)
			return std::make_pair(-1, 1);

		if (sequence_len > 4)
			return std::make_pair(-1, sequence_len);

		if (sequence_len > int(str.size()))
			return std::make_pair(-1, static_cast<int>(str.size()));

		std::int32_t ch = 0;
		// first byte
		switch (sequence_len)
		{
			case 1:
				ch = str[0] & 0b01111111;
				break;
			case 2:
				ch = str[0] & 0b00011111;
				break;
			case 3:
				ch = str[0] & 0b00001111;
				break;
			case 4:
				ch = str[0] & 0b00000111;
				break;
		};
		for (int i = 1; i < sequence_len; ++i)
		{
			auto const b = static_cast<std::uint8_t>(str[static_cast<std::size_t>(i)]);
			// continuation bytes must start with 10xxxxxx
			if (b > 0b10111111 || b < 0b10000000)
				return std::make_pair(-1, sequence_len);
			ch <<= 6;
			ch += b & 0b111111;
		}

		// check if the sequence is overlong, i.e. whether it has leading
		// (redundant) zeros
		switch (sequence_len)
		{
			case 2:
				if (ch < 0x80) return std::make_pair(-1, sequence_len);
				break;
			case 3:
				if (ch < 0x800) return std::make_pair(-1, sequence_len);
				break;
			case 4:
				if (ch < 0x10000) return std::make_pair(-1, sequence_len);
				break;
		};

		if (ch > max_codepoint)
			return std::make_pair(-1, sequence_len);

		return std::make_pair(static_cast<std::int32_t>(ch), sequence_len);
	}
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

