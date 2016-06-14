#include "profile.h"
#include "util.h"
using namespace std;

namespace bcm2dump {

string trim(string str)
{
	if (str.empty()) {
		return str;
	}

	auto i = str.find_last_not_of(" \x0d\n\t");
	if (i != string::npos) {
		str.erase(i + 1);
	}

	i = str.find_first_not_of(" \x0d\n\t");
	if (i == string::npos) {
		return "";
	}

	return str.substr(i);
}

string to_hex(const std::string& buffer)
{
	string ret;

	for (char c : buffer) {
		ret += to_hex(c);
	}

	return ret;
}

ofstream logger::s_bucket;
logger::severity logger::s_loglevel = logger::INFO;

ostream& logger::log(severity s)
{
	if (s < s_loglevel) {
		return s_bucket;
	} else if (s >= WARN) {
		return cerr;
	} else {
		return cout;
	}

}



}