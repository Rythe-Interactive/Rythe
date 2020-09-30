#pragma once
#include <algorithm>
#include <cctype>
#include <locale>
#include <vector>
#include <sstream>
#include <iterator>
#include <string>
#include <cstring>

/**
 * @file string_extra.hpp
 */
namespace args::core::common {

    /**@brief Replace each occurrence of [item] in [source] with [value].
     * @return Amount of items replaced.
     */
    inline size_t replace_items(std::string& source, const std::string& item, const std::string& value)
    {
        size_t count = 0;
        size_t n = 0;

        while ((n = source.find(item, n)) != std::string::npos) // While there's items to be found, keep replacing them with value.
        {
            count++;
            source.replace(n, item.size(), value);
            n += value.size();
        }

        return count;
    }

    struct isChars
    {
        isChars(const char* chars) :_chars(chars) {}

        bool operator()(char c) const
        {
            for (auto testChar = _chars; *testChar != 0; ++testChar)
                if (*testChar == c) return true;
            return false;
        }
        const char* _chars;
    };


    template <typename t>
    std::string string_from_data(t data);
    template <typename T>
    T data_from_string_(std::string str);



    template <char token, char... tokens>
    bool str_tokens_helper(std::ctype<char>::mask* rc)
    {
        rc[token] = std::ctype<char>::space;
        if constexpr (sizeof...(tokens) != 0) str_tokens_helper<tokens...>(rc);
        return true;
    }

    //dilems helper classlo
    template <char token, char... tokens>
    struct str_tokens : std::ctype<char>
    {
        str_tokens() : ctype<char>(get_table())
        {
        }

        static mask const* get_table()
        {
            //create const and normal rc
            static const mask* const_rc = classic_table();
            static mask rc[table_size];
            static auto memory = memcpy(rc, const_rc, table_size * sizeof(mask));
            static bool memory2 = str_tokens_helper<token, tokens...>(rc);
            //set spaces

            return rc;
        }
    };

    //split string at any given char via variadic template and insert into vector
    template <char token_1, char... token>
    std::vector<std::string> split_string_at(const std::string& string)
    {
        //copy string into stringstream
        std::stringstream ss(string);

        //set tokens
        ss.imbue(std::locale(std::locale(), new str_tokens<token_1, token...>()));

        //create iterators
        const std::istream_iterator<std::string> begin(ss);
        const std::istream_iterator<std::string> end;

        //copy using iterators
        const std::vector<std::string> vstrings(begin, end);

        return vstrings;
    }

    template <const char* const delim, typename Range, typename Value = typename Range::value_type>
    std::string join_strings_with(Range const& elements) {
        std::ostringstream os;
        auto b = begin(elements), e = end(elements);

        if (b != e) {
            std::copy(b, prev(e), std::ostream_iterator<Value>(os, delim));
            b = prev(e);
        }
        if (b != e) {
            os << *b;
        }

        return os.str();
    }
    template <typename Range, typename Value = typename Range::value_type>
    std::string join_strings_with(Range const& elements, const char* const delim) {
        std::ostringstream os;
        auto b = begin(elements), e = end(elements);

        if (b != e) {
            std::copy(b, prev(e), std::ostream_iterator<Value>(os, delim));
            b = prev(e);
        }
        if (b != e) {
            os << *b;
        }

        return os.str();
    }

    template <const char delim, typename Range, typename Value = typename Range::value_type>
    std::string join_strings_with(Range const& elements) {
        std::ostringstream os;
        auto b = begin(elements), e = end(elements);

        char promoter[2] = { delim };

        if (b != e) {
            std::copy(b, prev(e), std::ostream_iterator<Value>(os, delim));
            b = prev(e);
        }
        if (b != e) {
            os << *b;
        }

        return os.str();
    }
    template <typename Range, typename Value = typename Range::value_type>
    std::string join_strings_with(Range const& elements, char delim) {
        std::ostringstream os;
        auto b = begin(elements), e = end(elements);

        char promoter[2] = { delim };

        if (b != e) {
            std::copy(b, prev(e), std::ostream_iterator<Value>(os, promoter));
            b = prev(e);
        }
        if (b != e) {
            os << *b;
        }

        return os.str();
    }

    //remove given word from string at offset and return true if success and false on fail
    inline bool find_and_remove_at(std::string& src, const std::string& search, size_t offset = 0)
    {
        //create temporary
        size_t loc;

        //find given word
        if ((loc = src.find(search, offset)) == std::string::npos) return false;

        //erase word
        src.erase(loc, search.size());

        return true;
    }

    //remove given word from string at offset and return the position
    inline size_t locate_and_delete_at(std::string& src, const std::string& search, size_t offset = 0)
    {
        //create temporary
        size_t loc;

        //find given word
        if ((loc = src.find(search, offset)) == std::string::npos) return std::string::npos;

        //erase word
        src.erase(loc, search.size());

        return loc;
    }

    //find the nearest of given tokens in a string at an offset
    template <char token_1, char... tokens>
    inline size_t nearest_of_any_at(std::string string, size_t offset = 0)
    {
        //create tokens using variadic unfolding
        std::vector<char> tokensVector = { token_1 , tokens... };

        //create locations
        std::vector<size_t> locationsVector(tokensVector.size());

        //find locations
        for (char& token : tokensVector) locationsVector.push_back(string.find(token, offset));

        //find smallest element and return
        return *min_element(begin(locationsVector), end(locationsVector));
    }


    template <>
    inline std::string data_from_string_<std::string>(std::string str)
    {
        return str;
    }

    template <>
    inline std::vector<std::string> data_from_string_<std::vector<std::string>>(std::string str)
    {
        return split_string_at<',', ' '>(str);
    }

    template <typename T>
    inline T data_from_string_(std::string str)
    {
        std::stringstream temp(str);
        T value;
        temp >> value;
        return value;
    }


    template <>
    inline std::string string_from_data<std::string>(std::string data)
    {
        return data;
    }

    template <>
    inline std::string string_from_data<std::vector<std::string>>(std::vector<std::string> data)
    {
        std::string ret;
        for (const std::string& str : data)
            ret += str;
        return ret;
    }


    template <typename t>
    inline std::string string_from_data(t data)
    {
        std::stringstream temp;
        temp << data;
        return temp.str();
    }
    template <char D = '.'>
    constexpr size_t count_delimiter(const char* str)
    {
        return str[0] == '\0' ? 0 : str[0] == D ? count_delimiter<D>(&str[1]) + 1 : count_delimiter<D>(&str[1]);
    }

    constexpr size_t cstrptr_length(const char* str)
    {
        return *str ? 1 + cstrptr_length(str + 1) : 0;
    }


    template <class T, size_t N> constexpr size_t arr_length(T[N])
    {
        return N;
    }

    // trim from start (in place)
    static inline void ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
            }));
    }

    template <class Trimmer>
    static inline void ltrim(std::string& s, Trimmer&& t)
    {
        s.erase(std::find_if(s.begin(), s.end(), [&t](int ch) {
            return !t(ch);
            }), s.end());
    }

    // trim from end (in place)
    static inline void rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
            }).base(), s.end());
    }
    template <class Trimmer>
    static inline void rtrim(std::string& s, Trimmer&& t)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [&t](int ch) {
            return !t(ch);
            }).base(), s.end());
    }


    // trim from both ends (in place)
    static inline void trim(std::string& s) {
        ltrim(s);
        rtrim(s);
    }

    template <class Trimmer>
    static inline void trim(std::string& s, Trimmer&& t)
    {
        ltrim(s, t);
        rtrim(s, t);
    }

    // trim from start (copying)
    static inline std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    static inline std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    static inline std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }
    template<class Trimmer>
    // trim from start (copying)
    static inline std::string ltrim_copy(std::string s, Trimmer&& t) {
        ltrim(s, t);
        return s;
    }
    template<class Trimmer>
    // trim from end (copying)
    static inline std::string rtrim_copy(std::string s, Trimmer&& t) {
        rtrim(s, t);
        return s;
    }
    template<class Trimmer>
    // trim from both ends (copying)
    static inline std::string trim_copy(std::string s, Trimmer&& t) {
        trim(s, t);
        return s;
    }

}
