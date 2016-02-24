#pragma once

#include <string>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

class Forbid_word_replace
{
public:
    Forbid_word_replace();
    ~Forbid_word_replace();

    void reload();

    void Filter(std::string &input);
    //×Ö·û´®ÊÇ·ñºÏ·¨
    bool isLegal(const std::string &input);
    static Forbid_word_replace* getInstance();
private:
    static Forbid_word_replace* m_handle;
    std::string m_replace_string;
    boost::unordered_map<std::string, unsigned char> m_forbit_word_list;
    uint64_t m_max_word_len;
    uint64_t m_min_word_len;
};

