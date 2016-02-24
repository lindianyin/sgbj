#include "text_filter.h"
#include "utils_all.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <boost/regex.hpp>

Database& GetDb();

Forbid_word_replace* Forbid_word_replace::m_handle = NULL;

Forbid_word_replace* Forbid_word_replace::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new Forbid_word_replace();
        m_handle->reload();
    }
    return m_handle;
}

Forbid_word_replace::Forbid_word_replace()
{
    m_max_word_len = 0;
    m_min_word_len = 0;
    //cout<<"max word len = "<<m_max_word_len<<endl;
}

Forbid_word_replace::~Forbid_word_replace()
{
}

void Forbid_word_replace::reload()
{
    //空字符不允许
    m_forbit_word_list["\xE3\x80\x80"] = 2;
    m_forbit_word_list["\xE2\x80\x8C"] = 2;
    
    m_max_word_len = 0;
    Query q(GetDb());
    int total_nums = 0;
    q.execute("set names utf8");
    q.get_result("select cname from text_filter where 1 ORDER BY LENGTH( cname )");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        ++total_nums;
        std::string key = q.getstr();
        if (key != "")
        {
            //for (unsigned int k = 0; k< key.length(); ++k)
            //{
            //    key[k] = tolower(key[k]);
            //}
            // 1、既不是非关键字也不是关键字的起始 2、最大长度的关键字  3、非最大长度的关键字
            for (unsigned int j = 1; j < key.length(); ++j)
            {
                std::string strKey = key.substr(0, j);
                //原先是最大是关键字(已经是最大长度)
                if (m_forbit_word_list[strKey] == 2)
                {
                    m_forbit_word_list[strKey] = 3;
                }
                else if (m_forbit_word_list[strKey] == 0)
                {
                    m_forbit_word_list[strKey] = 1;
                }
                //cout<<"key:"<<key.substr(0, j)<<",1"<<endl;
            }
            if (m_forbit_word_list[key] == 1)
            {
                m_forbit_word_list[key] = 3;
            }
            else
            {
                m_forbit_word_list[key] = 2;
            }
            //cout<<"key:"<<key<<",2\n";
        }
        else
        {
            continue;
        }
        if (m_max_word_len < key.length())
        {
            m_max_word_len = key.length();
        }
    }
    q.free_result();
    cout<<"total "<<total_nums<<" forbid words!"<<endl;
}

//过滤原则:优先过滤最大长度的关键字，例:关键字有 abc,abcd,abcde,对于字串 abcdef,优先过滤为 **f
void Forbid_word_replace::Filter(std::string &input)
{
    size_t sub3_len = 0;        //用来记录曾今匹配到的关键字长度
    uint64_t index = 0;
    while (index < input.length())
    {
        sub3_len = 0;
        //cout<<"index:"<<index<<endl;
        //记录是否比到2
        bool ifbreak = false;
        for (size_t len = 1; len <= min(input.length() - index, m_max_word_len); ++len)
        {
            std::string sub = input.substr(index, len);
            //for (uint64_t k = 0; k < sub.length(); ++k)
            //{
            //    sub[k] = tolower(sub[k]);
            //}
            //bool ifbreak = false;
            switch (m_forbit_word_list[sub])
            {
                //不是关键字
                //case 0:
                //    ifbreak = true;
                    //判断之前是不是匹配到了关键字
                //    if (sub3_len > 0)
                //    {
                //        input.replace(index, sub3_len, "**");
                //        index += 2;
                //    }
                //    break;
                case 1:
                    break;
                //匹配到是关键字(已经是最大长度)
                case 2:
                    //cout<<"replace:"<<sub<<"\n";
                    input.replace(index, len, "**");
                    index += 2;
                    ifbreak = true;
                    break;
                //匹配到是关键字，但后面可能还能匹配更长的关键字,先记录长度
                case 3:
                    sub3_len = len; //记录长度
                    break;
                //匹配到部分关键字，继续匹配后面的
                default:
                    break;
            }
            if (ifbreak)
            {
                break;
            }
        }
        //循环自然结束并且存在记录点
        if (!ifbreak && sub3_len > 0)
        {
            input.replace(index, sub3_len, "**");
            index += 2;
        }
        ++index;
    }
}

//输入是否合法
bool Forbid_word_replace::isLegal(const std::string &input)
{
    //是否允许在名字中间存在空格,空格不能出现在开始和结束
#ifdef ALLOW_SPACE_IN_NAME
    // 允许数字，字母，汉字，空格，但不能以空格开始和结束
    boost::regex expression("^(?!\\s)(?!.*?\\s$)[a-zA-Z0-9\u4e00-\u9fa5\\s]+$");
    //boost::regex expression("^(?!_)(?!\\s)(?!.*?_$)(?!.*?\\s$)[a-zA-Z0-9\u4e00-\u9fa5\\s]+$");
#else
    // 允许数字，字母，汉字
    boost::regex expression("^[a-zA-Z0-9\u4e00-\u9fa5]+$");
#endif
    if (!boost::regex_match(input, expression))
    {
        return false;
    }
    size_t sub3_len = 0;        //用来记录曾今匹配到的关键字长度
    uint64_t index = 0;
    while (index < input.length())
    {
        sub3_len = 0;
        //cout<<"index:"<<index<<endl;
        //记录是否比到2
        bool ifbreak = false;
        for (size_t len = 1; len <= min(input.length() - index, m_max_word_len); ++len)
        {
            std::string sub = input.substr(index, len);
            switch (m_forbit_word_list[sub])
            {
                case 1:
                    break;
                //匹配到是关键字(已经是最大长度)
                case 2:
                    //cout<<"replace:"<<sub<<"\n";
                    return false;
                //匹配到是关键字，但后面可能还能匹配更长的关键字,先记录长度
                case 3:
                    sub3_len = len; //记录长度
                    break;
                //匹配到部分关键字，继续匹配后面的
                default:
                    break;
            }
            if (ifbreak)
            {
                break;
            }
        }
        //循环自然结束并且存在记录点
        if (!ifbreak && sub3_len > 0)
        {
            return false;
        }
        ++index;
    }
    return true;
}

