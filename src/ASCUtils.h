#ifndef ASCUtils

 #define ASCUtils
#include <string>
#include <vector>
#include <regex>

template <typename OutputIterator>
void string_split(std::string const &s, char sep, OutputIterator output, bool skip_empty = true)
{
	std::regex rxSplit(std::string("\\") + sep + (skip_empty ? "+" : ""));

	std::copy(std::sregex_token_iterator(std::begin(s), std::end(s), rxSplit, -1),
						std::sregex_token_iterator(), output);
};

std::vector<String> splitString(String s, char sep,bool se = true) {
  std::vector<std::string> params;
  string_split(s.c_str(), sep, std::back_inserter(params),se);
  std::vector<String> retval;
  for (std::string vs : params) {
    retval.push_back(String(vs.c_str()));
  }
  return retval;
}
#endif // !ASCUtils
