#ifndef IVANP_ARG_MATCH_HH
#define IVANP_ARG_MATCH_HH

#ifdef ARGS_PARSER_STD_REGEX
#include <regex>
#elif defined(ARGS_PARSER_BOOST_REGEX)
#include <boost/regex.hpp>
#endif

namespace ivanp { namespace args {
namespace detail {

// Matchers ---------------------------------------------------------
// These represent rules for matching program arguments with argument
// definitions

struct arg_match_base {
  virtual bool operator()(const char* arg) const noexcept = 0;
  virtual ~arg_match_base() { }
};

template <typename T>
class arg_match final : public arg_match_base {
  T m; // matching rule
public:
  template <typename... Args>
  arg_match(Args&&... args): m(std::forward<Args>(args)...) { }
  inline bool operator()(const char* arg) const noexcept { return m(arg); }
};

template <>
inline bool arg_match<char>::operator()(const char* arg) const noexcept {
  return ( arg[1]==m && arg[2]=='\0' );
}
template <>
bool arg_match<const char*>::operator()(const char* arg) const noexcept;
template <>
inline bool arg_match<std::string>::operator()(const char* arg) const noexcept {
  return m == arg;
}
#ifdef _GLIBCXX_REGEX
template <>
inline bool arg_match<std::regex>::operator()(const char* arg) const noexcept {
  return std::regex_match(arg,m);
}
#endif
#ifdef BOOST_RE_REGEX_HPP
template <>
inline bool arg_match<boost::regex>::operator()(const char* arg) const noexcept {
  return boost::regex_match(arg,m);
}
#endif

// Argument type ----------------------------------------------------

enum arg_type { long_arg, short_arg, context_arg };

arg_type find_arg_type(const char* arg) noexcept;
inline arg_type find_arg_type(const std::string& arg) noexcept {
  return find_arg_type(arg.c_str());
}

// Matcher factories ------------------------------------------------

using arg_match_type = std::pair<const arg_match_base*,arg_type>;
template <typename T> struct arg_match_tag { };

template <typename T>
inline arg_match_type make_arg_match(T&& x) noexcept {
  return make_arg_match_impl(std::forward<T>(x),
    arg_match_tag<std::decay_t<T>>{});
}

template <typename T, typename Tag>
arg_match_type make_arg_match_impl(T&& x, Tag) noexcept {
  return { new arg_match<T>( std::forward<T>(x) ), context_arg };
}
template <typename T>
arg_match_type make_arg_match_impl(T&& x, arg_match_tag<char>) noexcept {
  return { new arg_match<char>( x ), short_arg };
}
template <typename T, typename TagT>
std::enable_if_t<std::is_convertible<TagT,std::string>::value,arg_match_type>
make_arg_match_impl(T&& x, arg_match_tag<TagT>) noexcept {
  const arg_type t = find_arg_type(x);
#if defined(ARGS_PARSER_STD_REGEX) || defined(ARGS_PARSER_BOOST_REGEX)
  using regex_t =
# ifdef ARGS_PARSER_STD_REGEX
    std::regex;
# else
    boost::regex;
# endif
  if (t==context_arg)
    return { new arg_match<regex_t>( std::forward<T>(x) ), t };
  else
#endif
    return { new arg_match<TagT>( std::forward<T>(x) ), t };
}

}
}}

#endif
