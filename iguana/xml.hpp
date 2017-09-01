#pragma once
#include <string.h>
#include <algorithm>
#include <functional>
#include <cctype>
#include <boost/lexical_cast.hpp>
#include "reflection.hpp"

#define IGUANA_XML_READER_CHECK_FORWARD if (l > length) return 0; work_ptr += l; length -= l
#define IGUANA_XML_READER_CHECK_FORWARD_CHAR if(length < 1) return 0; work_ptr += 1; length -= 1
#define IGUANA_XML_HEADER "<?xml version = \"1.0\" encoding=\"UTF-8\">"

namespace iguana { namespace xml 
{
	namespace detail
	{
		struct char_const
		{
			static constexpr char angle_bracket = 0x3c;				// '<'
			static constexpr char anti_angle_bracket = 0x3e;		// '>'
			static constexpr char slash = 0x2f;					// '/'
			static constexpr char space = 0x20;					// ' '
			static constexpr char horizental_tab = 0x09;			// '/t'
			static constexpr char line_feed = 0x0a;					// '/n'
			static constexpr char enter = 0x0d;					// '/r'
			static constexpr char quote = 0x22;					// '"'
			static constexpr char underline = 0x5f;					// '_'
			static constexpr char question_mark = 0x3f;				// '?'
		};

		inline bool expected_char(char const* str, char character)
		{
			if (*str == character)
				return true;
			return false;
		}

		template <typename Pred>
		inline size_t forward(char const* begin, size_t length, Pred p)
		{
			auto work_ptr = begin;
			for (size_t traversed = 0; traversed < length; traversed++)
			{
				char work = *work_ptr;
				if (!p(work))
				{
					++work_ptr;
				}
				else
				{
					return traversed;
				}
			}

			return length;
		}

		inline size_t ignore_blank_ctrl(char const* begin, size_t length)
		{
			return forward(begin, length, [](auto c) { return !std::isblank(c) && !std::iscntrl(c); });
		}

		inline size_t get_token(char const* str, size_t length)
		{
			if (!(std::isalpha(*str) || expected_char(str++, char_const::underline)))
				return 0;

			return forward(str, length - 1, [](auto c) { return !std::isalnum(c) && char_const::underline != c; });
		}

		inline bool expected_token(char const* str, size_t length, char const* expected, size_t expected_length)
		{
			if (expected_length != length)
				return false;

			if (std::strncmp(str, expected, length) == 0)
				return true;
			return false;
		}

		inline size_t forward_until(char const* str, size_t length, char until_c)
		{
			return forward(str, length, [until_c](auto c) { return until_c == c; });
		}

		inline size_t forward_after(char const* str, size_t length, char after_c)
		{
			auto l = forward_until(str, length, after_c);
			if (l < length)
				l++;
			return l;
		}

		template <typename T>
		inline auto get_value(char const* str, size_t length, T& value)
			-> std::enable_if_t<std::is_arithmetic<T>::value>
		{
			value = boost::lexical_cast<T>(str, length);
		}

		inline void get_value(char const* str, size_t length, std::string& value)
		{
			value.assign(str, length);
		}

		template <typename T>
		struct array_size
		{
			static constexpr size_t value = 0;
		};

		template <typename T, size_t Size>
		struct array_size<T const(&)[Size]>
		{
			static constexpr size_t value = Size;
		};
	}

	class xml_reader_t
	{
		enum object_status
		{
			EMPTY = -1,
			ILLEGAL = 0,
			NORMAL = 1,
		};

	public:
		xml_reader_t(char const* buffer, size_t length)
			: buffer_(buffer)
			, length_(length)
		{
		}

		bool get_root()
		{
			auto work_ptr = buffer_;
			auto length = length_;

			auto l = detail::ignore_blank_ctrl(work_ptr, length);
			IGUANA_XML_READER_CHECK_FORWARD;

			if (!detail::expected_char(work_ptr, detail::char_const::angle_bracket))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD_CHAR;

			if (!detail::expected_char(work_ptr, detail::char_const::question_mark))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD_CHAR;

			l = detail::get_token(work_ptr, length);
			if (!detail::expected_token(work_ptr, l, "xml", 3))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD;

			l = detail::forward_after(work_ptr, length, detail::char_const::anti_angle_bracket);
			IGUANA_XML_READER_CHECK_FORWARD;

			buffer_ = work_ptr;
			length_ = length;

			return true;
		}

		int begin_object(char const* expected)
		{
			auto work_ptr = buffer_;
			auto length = length_;

			auto l = detail::ignore_blank_ctrl(work_ptr, length);
			IGUANA_XML_READER_CHECK_FORWARD;

			if (!detail::expected_char(work_ptr, detail::char_const::angle_bracket))
				return object_status::ILLEGAL;
			IGUANA_XML_READER_CHECK_FORWARD_CHAR;

			l = detail::get_token(work_ptr, length);
			if (!detail::expected_token(work_ptr, l, expected, std::strlen(expected)))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD;

			l = detail::forward_after(work_ptr, length, detail::char_const::anti_angle_bracket);
			IGUANA_XML_READER_CHECK_FORWARD;

			buffer_ = work_ptr;
			length_ = length;

			if (detail::expected_char(work_ptr - 1, detail::char_const::slash))
				return object_status::EMPTY;
			return object_status::NORMAL;
		}

		template <typename T>
		bool get_value(T& t)
		{
			auto work_ptr = buffer_;
			auto length = length_;

			auto l = detail::ignore_blank_ctrl(work_ptr, length);
			IGUANA_XML_READER_CHECK_FORWARD;

			l = detail::forward_until(work_ptr, length, detail::char_const::angle_bracket);
			try
			{
				detail::get_value(work_ptr, l, t);
			}
			catch (...)
			{
				return false;
			}
			IGUANA_XML_READER_CHECK_FORWARD;

			buffer_ = work_ptr;
			length_ = length;

			return true;
		}

		bool end_object(char const* expected)
		{
			auto work_ptr = buffer_;
			auto length = length_;

			auto l = detail::ignore_blank_ctrl(work_ptr, length);
			IGUANA_XML_READER_CHECK_FORWARD;

			if (!detail::expected_char(work_ptr, detail::char_const::angle_bracket))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD_CHAR;

			if (!detail::expected_char(work_ptr, detail::char_const::slash))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD_CHAR;

			l = detail::get_token(work_ptr, length);
			if (!detail::expected_token(work_ptr, l, expected, std::strlen(expected)))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD;

			if (!detail::expected_char(work_ptr, detail::char_const::anti_angle_bracket))
				return false;
			IGUANA_XML_READER_CHECK_FORWARD_CHAR;

			buffer_ = work_ptr;
			length_ = length;
			return true;
		}

		static const size_t xml_header_length = detail::array_size<decltype(IGUANA_XML_HEADER)>::value;

	private:
		char const*	buffer_;
		size_t		length_;
	};

	//to xml
	template<typename Stream, typename T>
	inline std::enable_if_t<!std::is_floating_point<T>::value && (std::is_integral<T>::value || std::is_unsigned<T>::value || std::is_signed<T>::value)>
		render_xml_value(Stream& ss, T value)
	{
		char temp[20];
		auto p = itoa_fwd(value, temp);
		ss.write(temp, p - temp);
	}

	template<typename Stream, typename T>
	inline std::enable_if_t<std::is_floating_point<T>::value> render_xml_value(Stream& ss, T value)
	{
		char temp[20];
		sprintf(temp, "%f", value);
		ss.write(temp);
	}

	template<typename Stream>
	inline void render_xml_value(Stream& ss, const std::string &s)
	{
		ss.write(s.c_str(), s.size());
	}

	template<typename Stream>
	inline void render_xml_value(Stream& ss, const char* s)
	{
		ss.write(s, strlen(s));
	}

	template<typename Stream, typename T>
	inline std::enable_if_t<std::is_arithmetic<T>::value> render_key(Stream& ss, T t) {
		ss.put('<');
		render_xml_value(ss, t);
		ss.put('>');
	}

	template<typename Stream>
	inline void render_key(Stream& ss, const std::string &s) {
		render_xml_value(ss, s);
	}

	template<typename Stream>
	inline void render_key(Stream& ss, const char* s) {
		render_xml_value(ss, s);
	}

	template<typename Stream>
	inline void render_tail(Stream& ss, const char* s)
	{
		ss.put('<');
		ss.put('/');
		ss.write(s, strlen(s));
		ss.put('>');
	}

	template<typename Stream>
	inline void render_head(Stream& ss, const char* s)
	{
		ss.put('<');
		ss.write(s, strlen(s));
		ss.put('>');
	}

	template<typename Stream, typename T, typename = std::enable_if_t<is_reflection<T>::value>>
	inline void to_xml_impl(Stream& s, T &&t) {
		for_each(std::forward<T>(t), [&s](const auto& v, size_t I, bool is_last) { //magic for_each struct std::forward<T>(t)
			render_head(s, get_name<T>(I));
			render_xml_value(s, v);
			render_tail(s, get_name<T>(I));
		}, [&s](const auto& o, size_t I, bool is_last)
		{
			render_head(s, get_name<T>(I));
			to_xml_impl(s, o);
			render_tail(s, get_name<T>(I));
		});
	}

	template<typename Stream, typename T, typename = std::enable_if_t<is_reflection<T>::value>>
	inline void to_xml(Stream& s, T &&t) {
		//render_head(s, "xml");
		s.write(IGUANA_XML_HEADER, xml_reader_t::xml_header_length);
		to_xml_impl(s, std::forward<T>(t));
	}

	template<typename T, typename = std::enable_if_t<is_reflection<T>::value>>
	inline void do_read(xml_reader_t &rd, T &&t)
	{
		for_each(std::forward<T>(t),
			[&rd](auto& value, size_t I, bool is_last) 
		{
			if (rd.begin_object(get_name<T>(I)) == 1)
			{
				// read value
				rd.get_value(value);
				rd.end_object(get_name<T>(I));
			}
		},
			[&rd](auto &obj, size_t I, bool is_last) 
		{
			if (rd.begin_object(get_name<T>(I)) == 1)
			{
				do_read(rd, obj);
				rd.end_object(get_name<T>(I));
			}
		});
	}

	template<typename T, typename = std::enable_if_t<is_reflection<T>::value>>
	inline void from_xml(T &&t, const char *buf, size_t len = -1)
	{
		xml_reader_t rd(buf, len);
		if (rd.get_root())
		{
			do_read(rd, t);
		}
	}
} }