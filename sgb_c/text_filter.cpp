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
    //���ַ�������
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
            // 1���Ȳ��Ƿǹؼ���Ҳ���ǹؼ��ֵ���ʼ 2����󳤶ȵĹؼ���  3������󳤶ȵĹؼ���
            for (unsigned int j = 1; j < key.length(); ++j)
            {
                std::string strKey = key.substr(0, j);
                //ԭ��������ǹؼ���(�Ѿ�����󳤶�)
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

//����ԭ��:���ȹ�����󳤶ȵĹؼ��֣���:�ؼ����� abc,abcd,abcde,�����ִ� abcdef,���ȹ���Ϊ **f
void Forbid_word_replace::Filter(std::string &input)
{
    size_t sub3_len = 0;        //������¼����ƥ�䵽�Ĺؼ��ֳ���
    uint64_t index = 0;
    while (index < input.length())
    {
        sub3_len = 0;
        //cout<<"index:"<<index<<endl;
        //��¼�Ƿ�ȵ�2
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
                //���ǹؼ���
                //case 0:
                //    ifbreak = true;
                    //�ж�֮ǰ�ǲ���ƥ�䵽�˹ؼ���
                //    if (sub3_len > 0)
                //    {
                //        input.replace(index, sub3_len, "**");
                //        index += 2;
                //    }
                //    break;
                case 1:
                    break;
                //ƥ�䵽�ǹؼ���(�Ѿ�����󳤶�)
                case 2:
                    //cout<<"replace:"<<sub<<"\n";
                    input.replace(index, len, "**");
                    index += 2;
                    ifbreak = true;
                    break;
                //ƥ�䵽�ǹؼ��֣���������ܻ���ƥ������Ĺؼ���,�ȼ�¼����
                case 3:
                    sub3_len = len; //��¼����
                    break;
                //ƥ�䵽���ֹؼ��֣�����ƥ������
                default:
                    break;
            }
            if (ifbreak)
            {
                break;
            }
        }
        //ѭ����Ȼ�������Ҵ��ڼ�¼��
        if (!ifbreak && sub3_len > 0)
        {
            input.replace(index, sub3_len, "**");
            index += 2;
        }
        ++index;
    }
}

//�����Ƿ�Ϸ�
bool Forbid_word_replace::isLegal(const std::string &input)
{
    //�Ƿ������������м���ڿո�,�ո��ܳ����ڿ�ʼ�ͽ���
#ifdef ALLOW_SPACE_IN_NAME
    // �������֣���ĸ�����֣��ո񣬵������Կո�ʼ�ͽ���
    boost::regex expression("^(?!\\s)(?!.*?\\s$)[a-zA-Z0-9\u4e00-\u9fa5\\s]+$");
    //boost::regex expression("^(?!_)(?!\\s)(?!.*?_$)(?!.*?\\s$)[a-zA-Z0-9\u4e00-\u9fa5\\s]+$");
#else
    // �������֣���ĸ������
    boost::regex expression("^[a-zA-Z0-9\u4e00-\u9fa5]+$");
#endif
    if (!boost::regex_match(input, expression))
    {
        return false;
    }
    size_t sub3_len = 0;        //������¼����ƥ�䵽�Ĺؼ��ֳ���
    uint64_t index = 0;
    while (index < input.length())
    {
        sub3_len = 0;
        //cout<<"index:"<<index<<endl;
        //��¼�Ƿ�ȵ�2
        bool ifbreak = false;
        for (size_t len = 1; len <= min(input.length() - index, m_max_word_len); ++len)
        {
            std::string sub = input.substr(index, len);
            switch (m_forbit_word_list[sub])
            {
                case 1:
                    break;
                //ƥ�䵽�ǹؼ���(�Ѿ�����󳤶�)
                case 2:
                    //cout<<"replace:"<<sub<<"\n";
                    return false;
                //ƥ�䵽�ǹؼ��֣���������ܻ���ƥ������Ĺؼ���,�ȼ�¼����
                case 3:
                    sub3_len = len; //��¼����
                    break;
                //ƥ�䵽���ֹؼ��֣�����ƥ������
                default:
                    break;
            }
            if (ifbreak)
            {
                break;
            }
        }
        //ѭ����Ȼ�������Ҵ��ڼ�¼��
        if (!ifbreak && sub3_len > 0)
        {
            return false;
        }
        ++index;
    }
    return true;
}

