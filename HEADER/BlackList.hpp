#ifndef BLACKLIST_HPP
#define BLACKLIST_HPP

#include <vector>
#include <string>

using std::vector;
using std::string;

class BlackList {
private:
    vector<string> URLs;
public:
    BlackList(const string &path);
    bool isMember(const string &host);
};

extern BlackList blackList;

#endif