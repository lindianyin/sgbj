
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include <boost/lexical_cast.hpp>

#include <map>
#include <iostream>
#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <iostream>  
#include <fstream>
#include "json_spirit.h"
#include <boost/regex.hpp>
#include<boost/tokenizer.hpp>
#include <stdlib.h>

using namespace std;

using namespace json_spirit;

#define INFO(x) cout<<x<<endl

#define CHECK_DB_ERR(q) if (q.GetErrno())\
    {\
         cout<<"sql:"<<q.GetLastQuery()<<endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<endl;\
    }

#define TO_STR(x) boost::lexical_cast<std::string>(x)

#define READ_INT_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_int();\
        }\
    }

#define READ_STR_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::str_type)\
        {\
            m = o[f].get_str();\
        }\
    }

void print_obj(json_spirit::mObject& obj);

void print_value(json_spirit::mValue& value)
{
    int type = value.type();
    cout<<"(type:"<<type<<")";
    switch (type)
    {
        case obj_type:
            print_obj(value.get_obj());
            break;
        case array_type:
            {
                std::vector<json_spirit::mValue> array = value.get_array();
                for (std::vector<json_spirit::mValue>::iterator it = array.begin(); it != array.end(); ++it)
                {
                    print_value(*it);
                }
            }
            break;
        case str_type:
            cout<<value.get_str();
            break;

        case bool_type:
            cout<<value.get_bool();
            break;
        case int_type:
            cout<<value.get_int();
            break;
        case real_type:
            cout<<value.get_real();
            break;
        case null_type:
            cout<<"nil";
            break;
    }
    cout<<endl;
}

void print_obj(json_spirit::mObject& obj)
{
    for (json_spirit::mObject::iterator it = obj.begin(); it != obj.end(); ++it)
    {
        cout<<"{"<<endl;
        cout<<it->first<<":";
        print_value(it->second);
        cout<<"}"<<endl;
    }
}

std::string get_string(const std::string s)
{
    size_t pos1 = s.find("\"");
    string ss = s.substr(pos1 + 1);
    ss.erase(ss.length()-1, 1);
    return ss;
}

void skill_data_field(ofstream& f, const std::string& field1, const std::string& value1)
{
    std::string field = get_string(field1);
    std::string value = get_string(value1);
    
    if (strcmp(field.c_str(), "condition") == 0)
    {
        f<<"<condition>"<<value<<"</condition>";
    }
    else if (strcmp(field.c_str(), "cast") == 0)
    {
        std::string new_value(value.c_str() + 2);
        new_value.erase(new_value.length() - 2, 2);

        cout<<"cast:["<<new_value<<"]"<<endl;
        using namespace boost;
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep("[]},{");
        tokenizer tok(new_value, sep);
        tokenizer::iterator it = tok.begin();

        std::string ck = "";
        while (it != tok.end())
        {
            if (ck == "")
            {
                ck = *it;
            }
            else
            {
                std::string v = *it;
                if (ck == "cd")
                {
                    int cd = atoi(v.c_str());
                    if (cd > 0)
                    {
                        cd = cd * 1000;
                        v = TO_STR(cd);
                    }
                }
                f<<"<"<<ck<<">"<<v<<"</"<<ck<<">";
                ck = "";
            }            
            ++it;
        }
    }
    else if (strcmp(field.c_str(), "shortime") == 0)
    {
        f<<"<shortime>"<<value<<"</shortime>";
    }
    else if (strcmp(field.c_str(), "lastime") == 0)
    {
        if (strcmp(value.c_str(), "[]") == 0)
        {
            f<<"<lastime>0</lastime>";
            f<<"<lastime_type/>";
        }
        else
        {
            if (value[0] == '[' && value[1] == '{')
            {
                std::string new_value(value.c_str() + 2);
                new_value[new_value.length()-2] = 0;
                boost::regex reg(",");  //按/符拆分字符串
                boost::cregex_token_iterator itrBegin = make_regex_token_iterator(new_value.c_str(),reg,-1); //使用-1参数时拆分，使用其它数字时表示取第几个子串，可使用数组取多个串
                boost::cregex_token_iterator itrEnd;

                if (itrBegin != itrEnd)
                {
                    f<<"<lastime>"<<*itrBegin<<"</lastime>";
                    ++itrBegin;
                }
                if (itrBegin != itrEnd)
                {
                    f<<"<lastime_type>"<<*itrBegin<<"</lastime_type>";
                }
                else
                {
                    f<<"<lastime_type/>";
                }
            }
            else
            {
                cout<<"lastime error."<<value<<endl;
            }
        }
    }
    else if (strcmp(field.c_str(), "base_att") == 0)
    {
        f<<"<base_att>"<<value<<"</base_att>";
    }
    else if (strcmp(field.c_str(), "level_desc") == 0)
    {
        f<<"<level_desc>"<<value<<"</level_desc>";
    }
    else
    {
        cout<<"unknow field["<<field<<"]["<<value<<"]"<<endl;
    }
}

void skill_data_detail(ofstream& f, const std::string& detail)
{
    size_t pos = detail.find("{");
    if (pos > 0 && detail[detail.length()-1] == '}')
    {
        std::string new_detail(detail.c_str() + pos);
        new_detail.erase(new_detail.length(), 1);

        boost::regex reg(";");  //按/符拆分字符串
        boost::cregex_token_iterator itrBegin = make_regex_token_iterator(new_detail.c_str(),reg,-1); //使用-1参数时拆分，使用其它数字时表示取第几个子串，可使用数组取多个串
        boost::cregex_token_iterator itrEnd;

        std::string field = "";

        f<<"<lv>";
        for(boost::cregex_token_iterator itr=itrBegin; itr!=itrEnd; ++itr)
        {            
            //cout << *itr << endl;

            if (field == "")
            {
                field = *itr;
            }
            else
            {
                skill_data_field(f, field, *itr);
                field = "";
            }
        }    
        f<<"</lv>";
    }
    else
    {
        cout<<"skill_data_detail().error."<<detail<<endl;
    }
}

void skill_data(ofstream& f, const std::string& data)
{
    if (data[0] == 'a' && data[1] == ':' && data[3] == ':')
    {
        f<<"<lvs>";
        std::string new_data(data.c_str() + 4);
        new_data.erase(new_data.length(), 1);
        boost::regex reg("i:\\d+;a:\\d+");  //按/符拆分字符串
        boost::cregex_token_iterator itrBegin = make_regex_token_iterator(data.c_str(),reg,-1); //使用-1参数时拆分，使用其它数字时表示取第几个子串，可使用数组取多个串
        boost::cregex_token_iterator itrEnd;
        for(boost::cregex_token_iterator itr=itrBegin; itr!=itrEnd; ++itr)
        {            
            cout <<"lv["<<endl<< *itr << endl<<"]lv"<<endl;
            skill_data_detail(f, *itr);
        }
        f<<"</lvs>";
    }
    else
    {
        cout<<"skill_data().error()"<<endl;
    }
}

std::string hexstr(unsigned char *buf, int len)
{
    const char *set = "0123456789abcdef";
    char str[65], *tmp;
    unsigned char *end; 
    if (len > 32)
        len = 32; 

    end = buf + len;
    tmp = &str[0]; 
    while (buf < end)
    {
        *tmp++ = set[ (*buf) >> 4 ];
        *tmp++ = set[ (*buf) & 0xF ];
        buf ++;
    }
    *tmp = 0; 
    return std::string(str);
}

bool IsValidateStr16(const char *str);  
int StrToNumber16(const char *str);  
int Char16ToInt(char c); 

bool IsValidateStr16(const char *str)  
{  
    int len,i;  
    if (NULL == str)  
    {  
        return false;  
    }  
    len = strlen(str);  
    for (i = 0; i < len; i++)  
    {  
        if (!((str[i] >= '0' && str[i] <= '9') || (str[i] >= 'A' && str[i] <= 'F')|| (str[i] >= 'a' && str[i] <= 'f')))  
           //满足条件之一0~9或者a~z或者A~Z都是合法的十六进制字符  
           return false;          
    }  
    return true;  
}  
  
int StrToNumber16(const char *str)  
{  
    int len,i,num;  
    num = 0;//使用数据必须初始化否则产生不确定值  
    len = strlen(str);  
    for (i = 0; i < len; i++)  
    {  
      num = num*16 + Char16ToInt(str[i]);/*十六进制字符串与10进制的对应数据*/   
    }  
    return num;  
  
}  
  
int Char16ToInt(char c)  
{  
    int num;  
    num = 0;//  
    switch (c)  
    {  
    case '0':  
            num = 0;  
            break;  
    case '1':  
            num = 1;  
            break;  
    case '2':  
            num = 2;  
            break;  
    case '3':  
            num = 3;  
            break;  
    case '4':  
            num = 4;  
            break;  
    case '5':  
            num = 5;  
            break;  
    case '6':  
            num = 6;  
            break;  
    case '7':  
            num = 7;  
            break;  
    case '8':  
            num = 8;  
            break;  
    case '9':  
            num = 9;  
            break;  
    case 'a':  
    case 'A':  
            num = 10;  
            break;  
    case 'b':  
    case 'B':  
            num = 11;  
            break;  
    case 'c':  
    case 'C':  
            num = 12;  
            break;  
    case 'd':  
    case 'D':  
            num = 13;  
            break;  
    case 'e':  
    case 'E':  
            num = 14;  
            break;  
    case 'f':  
    case 'F':  
            num = 15;  
            break;  
    default:  
        break;  
    }  
    return num;  
} 

std::string enc_unicode_to_utf8_one(unsigned long unic)
{
    char pOutput[7];
    memset(pOutput, 0, 7);
    if ( unic <= 0x0000007F )
    {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *pOutput     = (unic & 0x7F);
    }
    else if ( unic >= 0x00000080 && unic <= 0x000007FF )
    {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        *(pOutput+1) = (unic & 0x3F) | 0x80;
        *pOutput     = ((unic >> 6) & 0x1F) | 0xC0;
    }
    else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )
    {
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
        *(pOutput+2) = (unic & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 12) & 0x0F) | 0xE0;
    }
    else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )
    {
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+3) = (unic & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 18) & 0x07) | 0xF0;
    }
    else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )
    {
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+4) = (unic & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 24) & 0x03) | 0xF8;
    }
    else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )
    {
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+5) = (unic & 0x3F) | 0x80;
        *(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 30) & 0x01) | 0xFC;
    }
    std::string out = pOutput;
    return out;
}

std::string unicode_to_utf8(const std::string& instr)  
{
    const uint8_t* in = (const uint8_t*)instr.c_str();
    std::string out = "";
    for (size_t i = 0; i < instr.length(); )  
    {
        if (*(in+i) == '\\' && (i+1) != instr.length() && *(in+i+1) == 'u' && (i+5) != instr.length())
        {
            std::string tmp_str = instr.substr(i+2,4);            
            unsigned long unicode = StrToNumber16(tmp_str.c_str()); 
            cout<<std::hex<<tmp_str<<"->"<<unicode<<endl;
            
            out += enc_unicode_to_utf8_one(unicode);
            i += 6;
        }
        else
        {
            ++i;
            //cout<<std::oct;
            //cout<<"error at "<<i<<"=>"<<instr<<endl;
            //return "";
        }
    }  
    cout<<std::oct;
    return out;  
}  

std::string utf8_to_unicode(uint8_t *in)
{  
    uint8_t *p = in;
    std::string out = "";
    while(*p)
    {  
        if (*p >= 0x00 && *p <= 0x7f)  
        {
            out += "\\u00";
            out += hexstr(p, 1);            
        }  
        else if ((*p & (0xff << 5))== 0xc0)  
        {  
            uint8_t t1 = 0;  
            uint8_t t2 = 0;  
            t1 = *p & (0xff >> 3);  
            p++;  
            t2 = *p & (0xff >> 2);  
            uint8_t tt1 = t2 | ((t1 & (0xff >> 6)) << 6);//t1 >> 2;  
            uint8_t tt2 = t1 >> 2;//t2 | ((t1 & (0xff >> 6)) << 6);  
              out += "\\u";
            out += hexstr(&tt2, 1);
            out += hexstr(&tt1, 1);
        }  
        else if ((*p & (0xff << 4))== 0xe0)  
        {  
            uint8_t t1 = 0;  
            uint8_t t2 = 0;  
            uint8_t t3 = 0;

            t1 = *p & (0xff >> 3);  
            p++;  
            t2 = *p & (0xff >> 2);  
            p++;  
            t3 = *p & (0xff >> 2);  
  
            //Little Endian  
            
            uint8_t tt1 = ((t2 & (0xff >> 6)) << 6) | t3;//(t1 << 4) | (t2 >> 2);    
            uint8_t tt2 = (t1 << 4) | (t2 >> 2);//((t2 & (0xff >> 6)) << 6) | t3;    
              out += "\\u";
            out += hexstr(&tt2, 1);
            out += hexstr(&tt1, 1);
            
        }
        p++;  
    }      
    return out;
}  
  
void dump_utf8(uint8_t *utf8)  
{  
    uint8_t *p = utf8;  
  
    while(*p)  
    {  
        printf("%02X", *p);  
        p++;  
    }  
    putchar('\n');  
}  
  
void dump_unicode(uint16_t *utf16, int size)  
{  
    uint8_t *p = (uint8_t *)utf16;  
    int i = 0;  
  
    for (i = 0; i < size; i++)  
    {  
        printf("%02X", *p);  
        p++;  
    }  
    putchar('\n');  
}  


size_t fix_s_len(std::string& s, size_t start_pos)
{
    size_t pos = s.find("\"level_desc\";s:", start_pos);
    if (pos != std::string::npos)
    {
        //cout<<"pos:"<<pos<<endl;
        pos += 15;
        std::string pres = s.substr(0, pos);
        std::string ends = s.substr(pos);


        size_t pos1 = ends.find("\"");
        size_t pos2 = ends.find("\";");

        if (pos1 != std::string::npos && pos2 != std::string::npos)
        {
            std::string fix = ends.substr(pos1+1, pos2-pos1-1);
            //cout<<"fix:"<<fix<<endl;
            std::string left = ends.substr(pos2 + 2);
            //cout<<"left:"<<left<<endl;

            s = pres + TO_STR(fix.length()) + ":\"" + fix + "\";";
            start_pos = s.length();
            s += left;
            return start_pos;
        }
        else
        {
            cout<<"fix_s_len:error,"<<pos1<<","<<pos2<<"["<<s<<"]"<<endl;
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

struct scene_data
{
    int id;
    std::string str_elem;
    std::string str_npc;
    std::string str_mon;
};


int main(int argc, char* argv[])
{
    Database db("localhost", "c_user", "23rf234", argv[1]);//, CLIENT_INTERACTIVE);
    Query q(db);

    if (argc < 2)
    {
        cout<<"xxx db_name"<<endl;
    }
    std::string db_name = argv[1];
    std::string dowhat = argv[2];
    if (argc == 2 || (dowhat == "goods"))
    {
        ofstream goods_xml("goods.xml", ios::out|ios::trunc);
        if (goods_xml.is_open())
        {
            std::string goods_fields[] = 
            {
                "goods_id",
                "goods_name",
                "intro",
                "type",
                "subtype",
                "bind",
                "price_type",
                "price",
                "trade",
                "sell_price",
                "sell",
                "isdrop",
                "level",
                "career",
                "sex",
                "hp",
                "mp",
                "forza",
                "wit",
                "agile",
                "max_attack",
                "min_attack",
                "def",
                "hit",
                "dodge",
                "crit",
                "anti_wind",
                "anti_fire",
                "anti_water",
                "anti_thunder",
                "anti_soil",
                "anti_rift",
                "speed",
                "attrition",
                "suit_id",
                "max_hole",
                "step",
                "color",
                "other_data",""
            };
            goods_xml<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?><root>";

            std::string sql = "select ";
            for (int i = 0; goods_fields[i] != ""; ++i)
            {
                if (i > 0)
                {
                    sql += ",";
                }
                sql += goods_fields[i];
            }
            sql += " from " + db_name + ".base_goods where 1 order by goods_id desc";

            q.get_result(sql);
            while (q.fetch_row())
            {
                goods_xml<<"<goods>";
                for (int i = 0; goods_fields[i] != ""; ++i)
                {
                    std::string temp = q.getstr();
                    if (temp != "" && temp != "0")
                    {
                        goods_xml<<"<"<<goods_fields[i]<<">"<<temp<<"</"<<goods_fields[i]<<">";
                    }
                }
                goods_xml<<"</goods>";
            }
            q.free_result();
            
            goods_xml<<"</root>";
            goods_xml.close();

            ofstream skill_xml("data_skill.xml", ios::out|ios::trunc);
            if (!skill_xml.is_open())
            {
                cout<<"open skill xml fail"<<endl;
                return 0;
            }
            skill_xml<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?><skill>";
            q.get_result("select `id`,`name`,`desc`,`career`,`mod`,`type`,`obj`,`area`,`area_obj`,`level_effect`,`place`,`assist_type`,`limit_action`,`hate`,`data` from " + db_name + ".base_skill where 1 order by id");
            while (q.fetch_row())
            {
                int id = q.getval();
                //if (id == 10000 || id == 90008)
                {
                    skill_xml<<"<item>";            
                    std::string name = q.getstr();
                    std::string desc = q.getstr();
                    int career = q.getval();
                    int mod = q.getval();
                    int type = q.getval();
                    int obj = q.getval();
                    int area = q.getval();
                    int area_obj = q.getval();
                    std::string level_effect = q.getstr();
                    std::string place = q.getstr();
                    int assist_type = q.getval();
                    int limit_action = q.getval();
                    std::string hate = q.getstr();

                    skill_xml<<"<id>"<<id<<"</id>";
                    skill_xml<<"<name>"<<name<<"</name>";
                    skill_xml<<"<desc>"<<desc<<"</desc>";
                    skill_xml<<"<career>"<<career<<"</career>";
                    skill_xml<<"<mod>"<<mod<<"</mod>";
                    skill_xml<<"<type>"<<type<<"</type>";
                    skill_xml<<"<obj>"<<obj<<"</obj>";
                    skill_xml<<"<area>"<<obj<<"</area>";
                    skill_xml<<"<area_obj>"<<obj<<"</area_obj>";
                    skill_xml<<"<level_effect>"<<level_effect<<"</level_effect>";
                    if (place == "")
                    {
                        skill_xml<<"<place/>";
                    }
                    else
                    {
                        skill_xml<<"<place>"<<place<<"</place>";
                    }
                    skill_xml<<"<assist_type>"<<assist_type<<"</assist_type>";
                    skill_xml<<"<limit_action>"<<limit_action<<"</limit_action>";
                    skill_xml<<"<hate>"<<hate<<"</hate>";
                    std::string data = q.getstr();

                    //json_spirit::mValue value;
                    //json_spirit::read(data, value);

                    //cout<<"data:"<<data<<endl;
                    //print_value(value);
                    skill_data(skill_xml, data);
                    skill_xml<<"</item>";
                }
            }
            q.free_result();
            skill_xml<<"</skill>";
            skill_xml.close();
        }
    }

    else if (dowhat == "skill")
    {
        std::list<std::string> sqls;
        //fix "level_desc", length
        cout<<"fix..."<<endl;
        q.get_result("select `id`,`data` from " + db_name + ".base_skill where 1 order by id");
        while (q.fetch_row())
        {
            int id = q.getval();
            std::string old_data = q.getstr();

            std::string data = old_data;
            size_t start_s = 0;

            //if (id == 10000 || id == 90008)
            {
                int loop_times = 0;
                do
                {
                    start_s = fix_s_len(data, start_s);
                    ++loop_times;
                }
                while (start_s > 0 && loop_times < 10);

                if (data != old_data)
                {
                    //q.free_result();
                    //cout<<"fix id "<<id<<endl;
                    //q.execute("update " + db_name + ".base_skill set data='" + db.safestr(data) + "' where id=" + TO_STR(id));
                    //CHECK_DB_ERR(q);
                    //break;
                    sqls.push_back("update " + db_name + ".base_skill set data='" + db.safestr(data) + "' where id=" + TO_STR(id));
                }
            }
        }
        q.free_result();        

        cout<<"fix "<<sqls.size()<<endl;
        for (std::list<std::string>::iterator it = sqls.begin(); it != sqls.end(); ++it)
        {            
            q.execute(*it);
            CHECK_DB_ERR(q);
        }
    }
    else if ("scene" == dowhat)
    {
        cout<<"fix base_scene..."<<endl;
#ifdef DEBUG        
        ofstream scene_txt("scene.txt", ios::out|ios::trunc);
        if (!scene_txt.is_open())
        {
            cout<<"open scene.txt fail"<<endl;
            return 0;
        }
#endif
        std::map<std::string, std::string> npc_map;
        std::map<std::string, std::string> mon_map;
        std::map<std::string, std::string> scene_map;

#ifdef DEBUG
        scene_txt<<"--------base_npc--------"<<endl;
#endif
        q.get_result("select nid,name from base_npc where 1");
        while (q.fetch_row())
        {
            std::string id = q.getstr();
            std::string name = q.getstr();
            std::string newname = utf8_to_unicode((uint8_t*)name.c_str());
            if (newname != "")
            {
                npc_map[id] = newname;
#ifdef DEBUG
                scene_txt<<id<<":"<<name<<"->"<<newname<<endl;
#endif
            }
        }
        q.free_result();

#ifdef DEBUG
        scene_txt<<"--------base_mon--------"<<endl;
#endif
        q.get_result("select mid,name from base_mon where 1");
        while (q.fetch_row())
        {
            std::string id = q.getstr();
            std::string name = q.getstr();
            std::string newname = utf8_to_unicode((uint8_t*)name.c_str());
            if (newname != "")
            {
                mon_map[id] = newname;
#ifdef DEBUG
                scene_txt<<id<<":"<<name<<"->"<<newname<<endl;
#endif
            }
        }
        q.free_result();

#ifdef DEBUG
        scene_txt<<"--------base_scene--------"<<endl;
        q.get_result("select sid,name from base_scene where 1");
#endif
        while (q.fetch_row())
        {
            std::string id = q.getstr();
            std::string name = q.getstr();
            std::string newname = utf8_to_unicode((uint8_t*)name.c_str());
            if (newname != "")
            {
                scene_map[id] = newname;
#ifdef DEBUG
                scene_txt<<id<<":"<<name<<"->"<<newname<<endl;
#endif
            }
        }
        q.free_result();

        std::list<scene_data> scene_datas;
        q.get_result("select sid,elem,npc,mon from base_scene where 1");
        while (q.fetch_row())
        {
            scene_data s;
            s.id = q.getval();
            s.str_elem = q.getstr();
            s.str_npc = q.getstr();
            s.str_mon = q.getstr();

            json_spirit::mValue value_elem;
            json_spirit::read(s.str_elem, value_elem);
            if (value_elem.type() == array_type)
            {
                s.str_elem = "[";
                mArray& mArr = value_elem.get_array();
                for (mArray::iterator it = mArr.begin(); it != mArr.end(); ++it)
                {
                    mObject& mobj = it->get_obj();
                    std::string id = "";
                    READ_STR_FROM_MOBJ(id,mobj,"id");
                    int x = 0, y = 0;
                    READ_INT_FROM_MOBJ(x,mobj,"x");
                    READ_INT_FROM_MOBJ(y,mobj,"y");

                    if (it != mArr.begin())
                    {
                        s.str_elem += ",";
                    }
                    s.str_elem += "{\"name\":\"" + scene_map[id] + "\",";
                    s.str_elem += "\"x\":" + TO_STR(x) + ",";
                    s.str_elem += "\"id\":\"" + TO_STR(id) + "\",";
                    s.str_elem += "\"y\":" + TO_STR(y) + "}";
                }
                s.str_elem += "]";
            }
            else
            {
                cout<<"sid "<<s.id<<" elem not array,"<<s.str_elem<<endl;
            }

            json_spirit::mValue value_npc;
            json_spirit::read(s.str_npc, value_npc);
            if (value_npc.type() == array_type)
            {
                s.str_npc = "[";
                mArray& mArr = value_npc.get_array();
                for (mArray::iterator it = mArr.begin(); it != mArr.end(); ++it)
                {
                    mObject& mobj = it->get_obj();
                    std::string id = "";
                    READ_STR_FROM_MOBJ(id,mobj,"id");
                    int x = 0, y = 0;
                    READ_INT_FROM_MOBJ(x,mobj,"x");
                    READ_INT_FROM_MOBJ(y,mobj,"y");
                    if (it != mArr.begin())
                    {
                        s.str_npc += ",";
                    }
                    s.str_npc += "{\"id\":\"" + TO_STR(id) + "\",";
                    s.str_npc += "\"x\":" + TO_STR(x) + ",";
                    s.str_npc += "\"y\":" + TO_STR(y) + ",";
                    s.str_npc += "\"name\":\"" + npc_map[id] + "\"}";
                }
                s.str_npc += "]";
            }
            else
            {
                cout<<"sid "<<s.id<<" npc not array,"<<s.str_npc<<endl;
            }

            json_spirit::mValue value_mon;
            json_spirit::read(s.str_mon, value_mon);
            if (value_mon.type() == array_type)
            {
                s.str_mon = "[";
                mArray& mArr = value_mon.get_array();
                for (mArray::iterator it = mArr.begin(); it != mArr.end(); ++it)
                {
                    mObject& mobj = it->get_obj();
                    std::string id = "";
                    READ_STR_FROM_MOBJ(id,mobj,"id");
                    int x = 0, y = 0, lv = 1;
                    READ_INT_FROM_MOBJ(lv,mobj,"lv");
                    READ_INT_FROM_MOBJ(x,mobj,"x");
                    READ_INT_FROM_MOBJ(y,mobj,"y");

                    if (it != mArr.begin())
                    {
                        s.str_mon += ",";
                    }
                    s.str_mon += "{\"lv\":" + TO_STR(lv) + ",";
                    s.str_mon += "\"id\":\"" + TO_STR(id) + "\",";
                    s.str_mon += "\"x\":" + TO_STR(x) + ",";
                    s.str_mon += "\"y\":" + TO_STR(y) + ",";
                    s.str_mon += "\"name\":\"" + mon_map[id] + "\"}";
                }
                s.str_mon += "]";
            }
            else
            {
                cout<<"sid "<<s.id<<" mon not array,"<<s.str_mon<<endl;
            }

            scene_datas.push_back(s);
        }
        q.free_result();

#ifdef DEBUG
        scene_txt<<"-------fix base_scene-------"<<endl;
#endif
        for (std::list<scene_data>::iterator it = scene_datas.begin(); it != scene_datas.end(); ++it)
        {
            std::string sql = "update base_scene set elem='" + db.safestr(it->str_elem) + "',npc='" + db.safestr(it->str_npc) + "',mon='" + db.safestr(it->str_mon) + "' where sid=" + TO_STR(it->id);
            q.execute(sql);

#ifdef DEBUG
            scene_txt<<sql<<endl;
            if (q.GetErrno())
            {
                scene_txt<<"sql:"<<q.GetLastQuery()<<endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<endl;
            }
#else
            if (q.GetErrno())
            {
                cout<<"sql:"<<q.GetLastQuery()<<endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<endl;
            }
#endif
        }
        cout<<"fix base_scene, done."<<endl;
#ifdef DEBUG
        scene_txt.close();
#endif
    }
    else if ("base" == dowhat)
    {
        ofstream base_npc("data_npc.erl", ios::out|ios::trunc);
        ofstream base_mon("data_mon.erl", ios::out|ios::trunc);
        ofstream base_talk("data_talk.erl", ios::out|ios::trunc);
        if (!base_npc.is_open())
        {
            cout<<"open base_npc.erl fail."<<endl;
            return 0;
        }
        if (!base_mon.is_open())
        {
            cout<<"open base_mon.erl fail."<<endl;
            return 0;
        }
        if (!base_talk.is_open())
        {
            cout<<"open base_talk.erl fail."<<endl;
            return 0;
        }

        std::string str_now = "0000-00-00 00:00:00";
        q.get_result("select now()");
        if (q.fetch_row())
        {
            str_now = q.getstr();
        }
        cout<<"now => "<<str_now<<endl;
        q.free_result();

        base_npc<<endl;
        base_npc<<"\%\%\%---------------------------------------"<<endl;
        base_npc<<"\%\%\% @Module  : data_npc"<<endl;
        base_npc<<"\%\%\% @Author  : auto"<<endl;
        base_npc<<"\%\%\% @Created : "<<str_now<<endl;
        base_npc<<"\%\%\% @Description:  自动生成"<<endl;
        base_npc<<"\%\%\%---------------------------------------"<<endl;
        base_npc<<"-module(data_npc)."<<endl;
        base_npc<<"-export([get/1])."<<endl;
        base_npc<<"-include(\"record.hrl\")."<<endl<<endl;
        
        q.get_result("select nid,icon,name,talk from base_npc where 1");
        while (q.fetch_row())
        {
            int id = q.getval();
            int icon = q.getval();
            std::string name = q.getstr();
            int talk = q.getval();

            base_npc<<"get("<<id<<") ->"<<endl<<endl;
            base_npc<<"\t\t\t\t\t#ets_npc{"<<endl;
            base_npc<<"\t\t\t\t\t\tnid="<<id<<","<<endl;
            base_npc<<"\t\t\t\t\t\ticon="<<icon<<","<<endl;
            base_npc<<"\t\t\t\t\t\tname = <<\""<<name<<"\">>,"<<endl;
            base_npc<<"\t\t\t\t\t\ttalk = "<<talk<<endl;
            base_npc<<"\t\t\t\t\t};"<<endl<<endl<<endl;
        }
        q.free_result();
        base_npc<<"get(_Id) ->"<<endl;
        base_npc<<"    []."<<endl;
        base_npc.close();

        cout<<"data_npc.erl done."<<endl;

        base_mon<<endl;
        base_mon<<"\%\%\%---------------------------------------"<<endl;
        base_mon<<"\%\%\% @Module  : data_mon"<<endl;
        base_mon<<"\%\%\% @Author  : auto"<<endl;
        base_mon<<"\%\%\% @Created : "<<str_now<<endl;
        base_mon<<"\%\%\% @Description:  自动生成"<<endl;
        base_mon<<"\%\%\%---------------------------------------"<<endl;
        base_mon<<"-module(data_mon)."<<endl;
        base_mon<<"-export([get/1])."<<endl;
        base_mon<<"-include(\"record.hrl\")."<<endl<<endl;

        q.get_result("select mid,icon,name,def,lv,hp,hp_lim,mp,mp_lim,att_area,trace_area,exp,hit,dodge,crit,speed,att_speed,skill,retime,guard_area,anti_wind,anti_fire,anti_water,anti_thunder,att_type,max_attack,min_attack from base_mon where 1");
        while (q.fetch_row())
        {
            int mid = q.getval();
            base_mon<<"get("<<mid<<") ->"<<endl;
            base_mon<<"\t\t\t\t\t#ets_mon{"<<endl;
            base_mon<<"\t\t\t\t\t\tmid="<<mid<<","<<endl;
            base_mon<<"\t\t\t\t\t\ticon="<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tname = <<\""<<q.getstr()<<"\">>,"<<endl;
            base_mon<<"\t\t\t\t\t\tdef = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tlv = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\thp = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\thp_lim = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tmp = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tmp_lim = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tatt_area = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\ttrace_area = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\texp = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\thit = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tdodge = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tcrit = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tspeed = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tatt_speed = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tskill = "<<q.getstr()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tretime = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tguard_area = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tanti_wind = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tanti_fire = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tanti_water = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tanti_thunder = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tatt_type = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tmax_attack = "<<q.getval()<<","<<endl;
            base_mon<<"\t\t\t\t\t\tmin_attack = "<<q.getval()<<""<<endl;
            base_mon<<"\t\t\t\t\t};"<<endl<<endl<<endl;
        }
        q.free_result();

        base_mon<<"get(_MonId) ->"<<endl;
        base_mon<<"    []."<<endl;
        base_mon.close();
        cout<<"data_mon.erl done."<<endl;

        std::string test = unicode_to_utf8("\\u5deb\\u533b\\u4e4b\\u672f\\u867d\\u4e0e\\u4e2d\\u539f\\u533b\\u672f\\u5927\\u76f8\\u5f84\\u5ead\\uff0c\\u4f46\\u672c\\u8d28\\u90fd\\u4e3a\\u6cbb\\u75c5\\u6551\\u4eba\\uff01");
        ofstream unicode_test("unicode_test.txt", ios::out|ios::trunc);
        if (!unicode_test.is_open())
        {
            cout<<"open unicode_test.txt fail."<<endl;
            return 0;
        }
        unicode_test<<test<<endl;
        unicode_test.close();

        base_talk<<endl;
        base_talk<<"\%\%\%---------------------------------------"<<endl;
        base_talk<<"\%\%\% @Module  : data_talk"<<endl;
        base_talk<<"\%\%\% @Author  : auto"<<endl;
        base_talk<<"\%\%\% @Created : 2010-11-03 21:42:39"<<endl;
        base_talk<<"\%\%\% @Description:  自动生成"<<endl;
        base_talk<<"\%\%\%---------------------------------------"<<endl;
        base_talk<<"-module(data_talk)."<<endl;
        base_talk<<"-compile(export_all)."<<endl;
        base_talk<<"-include(\"common.hrl\")."<<endl;
        base_talk<<"\%\% 0    npc    NPC对话内容"<<endl;
        base_talk<<"\%\% 1    role    角色对话内容"<<endl;
        base_talk<<"\%\% 2    yes    确定"<<endl;
        base_talk<<"\%\% 3    no    取消"<<endl;
        base_talk<<"\%\% 4    fight    发生战斗"<<endl;
        base_talk<<"\%\% 5    trigger    触发任务"<<endl;
        base_talk<<"\%\% 6    finish    完成任务"<<endl;
        base_talk<<"\%\% 7    trigger_and_finish    触发并完成任务"<<endl;
        base_talk<<"\%\% 8    talk_event    触发对话事件"<<endl;
        base_talk<<"\%\% 9    build_guild    建立氏族"<<endl;
        base_talk<<"\%\% 10    apply_join_guild    申请加入氏族"<<endl;
        base_talk<<"\%\% 11    guild_store    氏族仓库"<<endl;
        base_talk<<"\%\% 12    guild_task    氏族任务"<<endl;
        base_talk<<"\%\% 13    learn_skill    技能学习"<<endl;
        base_talk<<"\%\% 14    personal_store    个人仓库"<<endl;
        base_talk<<"\%\% 15    buy    买物品"<<endl;
        base_talk<<"\%\% 16    sell    卖物品"<<endl;
        base_talk<<"\%\% 17    mixture    合成"<<endl;
        base_talk<<"\%\% 18    embed    镶嵌"<<endl;
        base_talk<<"\%\% 19    identify    装备鉴定"<<endl;
        base_talk<<"\%\% 20    mix    法宝熔合"<<endl;
        base_talk<<"\%\% 21    strenghten    装备强化"<<endl;
        base_talk<<"\%\% 22    petshop    仙宠购买  "<<endl;
        base_talk<<"\%\% 23    train_equip    法宝修炼"<<endl;
        base_talk<<"\%\% 24    drill    装备打孔"<<endl;
        base_talk<<"\%\% 25    fixed    装备修理"<<endl;
        base_talk<<"\%\% 26    fix_all    全部修理"<<endl;
        base_talk<<"\%\% 27    ablate    摘除宝石"<<endl;
        base_talk<<"\%\% 28    alchemy    炼丹"<<endl;
        base_talk<<"\%\% 29    back_to_guild_scene    回到帮会领地"<<endl;
        base_talk<<"\%\% 30    leave_guild_scene    离开帮会领地"<<endl;
        base_talk<<"\%\% 31    join_guild_war    参加帮会战"<<endl;
        base_talk<<"\%\% 32    watch_guild_war    观看帮会战"<<endl;
        base_talk<<"\%\% 33    enabled_xxd    开启玄虚鼎"<<endl;
        base_talk<<"\%\% 34    chushi    出师"<<endl;
        base_talk<<"\%\% 35    bole_billboard    伯乐榜"<<endl;
        base_talk<<"\%\% 36    soul_binding    灵魂绑定"<<endl;
        base_talk<<"\%\% 37    coin_binding    铜钱绑定"<<endl;
        base_talk<<"\%\% 38    task_award    任务奖励"<<endl;
        base_talk<<"\%\% 39    jobber    交易行"<<endl;
        base_talk<<"\%\% 40    open_favorite    弹出收藏框"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%43 convoy_city主城传送"<<endl;
        base_talk<<"\%\%44 bind_stone绑定回城石"<<endl;
        base_talk<<"\%\%45 convoy_center九霄传送"<<endl;
        base_talk<<"\%\%46 novice_card 领取新手卡"<<endl;
        base_talk<<"\%\%47 arena_sign 战场报名"<<endl;
        base_talk<<"\%\%48 arena_explain 战场说明"<<endl;
        base_talk<<"\%\%49 integral_shop 积分商场"<<endl;
        base_talk<<"\%\%50 all_fight 查看总战绩"<<endl;
        base_talk<<"\%\%51 last_fight 上一场战绩"<<endl;
        base_talk<<"\%\%52 honour_shop 荣誉商店"<<endl;
        base_talk<<"\%\%53 fst_nine 封神台九层"<<endl;
        base_talk<<"\%\%54 fst_eighteen 封神台十八层"<<endl;
        base_talk<<"\%\%55 honour_twenty_seven 封神台二十七层"<<endl;
        base_talk<<"\%\%56 arena_out 离开战场"<<endl;
        base_talk<<"\%\%57 consign 委托大厅"<<endl;
        base_talk<<"\%\%58 arena_in 进入战场"<<endl;
        base_talk<<"\%\%59 quit_box_scene 离开秘境"<<endl;
        base_talk<<"\%\%60 enter_box_scene 进入秘境"<<endl;
        base_talk<<"\%\%61 fst_one 进入封神台"<<endl;
        base_talk<<"\%\%62 guild_boss_one 召唤氏族boss1"<<endl;
        base_talk<<"\%\%63 guild_boss_two 召唤氏族boss2"<<endl;
        base_talk<<"\%\%64 guild_boss_three 召唤氏族boss3"<<endl;
        base_talk<<"\%\%65 apply_skyrush 神岛空战报名"<<endl;
        base_talk<<"\%\%66 enter_sky 进入神岛"<<endl;
        base_talk<<"\%\%67 skyrush_show 神岛空战说明"<<endl;
        base_talk<<"\%\%68 applied_guilds 当前已报名氏族"<<endl;
        base_talk<<"\%\%69 feats_rank 积分排行"<<endl;
        base_talk<<"\%\%70 feats_shop 功勋商店"<<endl;
        base_talk<<"\%\%71 quit_sky 离开神岛"<<endl;
        base_talk<<"\%\%72 get_faward 领取奖励"<<endl;
        base_talk<<"\%\%73 one_enter_demon 单人进入镇妖台"<<endl;
        base_talk<<"\%\%74 many_enter_demon 多人进入镇妖台"<<endl;
        base_talk<<"\%\%75 demon_fshop 镇妖台功勋商店"<<endl;
        base_talk<<"\%\%76 fashion_shop 时装商店"<<endl;
        base_talk<<"\%\%77 send_to_300 传至九霄"<<endl;
        base_talk<<"\%\%78 send_to_30 传至荷园(30级)"<<endl;
        base_talk<<"\%\%79 send_to_40 传至芷园(40级)"<<endl;
        base_talk<<"\%\%80 send_to_50 传至梅园(50级)"<<endl;
        base_talk<<"\%\%81 send_to_60 传至竹园(60级)"<<endl;
        base_talk<<"\%\%82 ring_bless 紫戒指祝福"<<endl;
        base_talk<<"\%\%83 enter_train_practice 进入试炼"<<endl;
        base_talk<<"\%\%84 exit_train_practice 离开试炼"<<endl;
        base_talk<<"\%\%85 enter_zxt  进入诛仙台"<<endl;
        base_talk<<"\%\%86 enter_hotspring_vip 进入皇家温泉(VIP专用)"<<endl;
        base_talk<<"\%\%87 enter_hotspring_normal 进入大众温泉"<<endl;
        base_talk<<"\%\%88 leave_hotspring 离开温泉"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%89 zxt_shop诛仙台荣誉商店"<<endl;
        base_talk<<"\%\%90 send_to_70 传至菊园（70级）"<<endl;
        base_talk<<"\%\%91 sex_change 变性"<<endl;
        base_talk<<"\%\%92 enter_war 进入封神大会"<<endl;
        base_talk<<"\%\%93 exit_war 退出封神大会"<<endl;
        base_talk<<"\%\%94 vip_drug 领取vip药品"<<endl;
        base_talk<<"\%\%95 war_info 封神大会"<<endl;
        base_talk<<"\%\%96 member_gwish 打开氏族祝福其他成员运势界面"<<endl;
        base_talk<<"\%\%97 enter_pet_zoon进入灵兽圣园"<<endl;
        base_talk<<"\%\%98 exit_pet_zoon 退出灵兽圣园"<<endl;
        base_talk<<"\%\%99 war_gold_shop 封神大会元宝商店"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%101 join_castle_rush 九霄城战报名"<<endl;
        base_talk<<"\%\%102 enter_castle_rush 进入城战"<<endl;
        base_talk<<"\%\%103 castle_rush_intro 九霄城战说明"<<endl;
        base_talk<<"\%\%104 castle_rush_join_list 当前已报名氏族"<<endl;
        base_talk<<"\%\%105 castle_rush_score_rank 积分排行"<<endl;
        base_talk<<"\%\%106 castle_rush_tax 领取税收"<<endl;
        base_talk<<"\%\%107 leave_castle_rush 退出攻城战"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%108 war_shop 声望商店"<<endl;
        base_talk<<"\%\%109 enter_warfare    进入神魔乱斗"<<endl;
        base_talk<<"\%\%110 leave_warfare 离开神魔乱斗"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%111 propose 提亲"<<endl;
        base_talk<<"\%\%112 book_wedding 预订婚宴"<<endl;
        base_talk<<"\%\%113 mass 群发喜帖"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%114 begin_weddings 开始拜堂"<<endl;
        base_talk<<"\%\%115 wedding_gift 赠送贺礼"<<endl;
        base_talk<<"\%\%116 leave_wedding 离开婚宴"<<endl;
        base_talk<<"\%\%117 divorce 离婚"<<endl;
        base_talk<<"\%\%118 cancel_wedding 取消婚期"<<endl;
        base_talk<<"\%\%119 view_wedding 查看婚宴信息"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%120 wish_tree 许愿树"<<endl;
        base_talk<<"\%\%121 fst_shop 神秘商店"<<endl;
        base_talk<<endl;
        base_talk<<"\%\%122 法宝销毁weapion_destory"<<endl;
        base_talk<<"type_to_int(Key)->"<<endl;
        base_talk<<"\tL = ["<<endl;
        base_talk<<"\t\t{npc,0},"<<endl;
        base_talk<<"\t\t{role,1},"<<endl;
        base_talk<<"\t\t{yes,2},"<<endl;
        base_talk<<"\t\t{no,3},"<<endl;
        base_talk<<"\t\t{fight,4},"<<endl;
        base_talk<<"\t\t{trigger,5},"<<endl;
        base_talk<<"\t\t{finish,6},"<<endl;
        base_talk<<"\t\t{trigger_and_finish,7},"<<endl;
        base_talk<<"\t\t{talk_event,8},"<<endl;
        base_talk<<"\t\t{build_guild,9},"<<endl;
        base_talk<<"\t\t{apply_join_guild,10},"<<endl;
        base_talk<<"\t\t{guild_store,11},"<<endl;
        base_talk<<"\t\t{guild_task,12},"<<endl;
        base_talk<<"\t\t{learn_skill,13},"<<endl;
        base_talk<<"\t\t{personal_store,14},"<<endl;
        base_talk<<"\t\t{buy,15},"<<endl;
        base_talk<<"\t\t{sell,16},"<<endl;
        base_talk<<"\t\t{mixture,17},"<<endl;
        base_talk<<"\t\t{embed,18},"<<endl;
        base_talk<<"\t\t{identify,19},"<<endl;
        base_talk<<"\t\t{mix,20},"<<endl;
        base_talk<<"\t\t{strenghten,21},"<<endl;
        base_talk<<"\t\t{petshop,22},"<<endl;
        base_talk<<"\t\t{train_equip,23},"<<endl;
        base_talk<<"\t\t{drill,24},"<<endl;
        base_talk<<"\t\t{fixed,25},"<<endl;
        base_talk<<"\t\t{fix_all,26},"<<endl;
        base_talk<<"\t\t{ablate,27},"<<endl;
        base_talk<<"\t\t{alchemy,28},"<<endl;
        base_talk<<"\t\t{back_to_guild_scene,29},"<<endl;
        base_talk<<"\t\t{leave_guild_scene,30},"<<endl;
        base_talk<<"\t\t{join_guild_war,31},"<<endl;
        base_talk<<"\t\t{watch_guild_war,32},"<<endl;
        base_talk<<"\t\t{enabled_xxd,33},"<<endl;
        base_talk<<"\t\t{chushi,34},"<<endl;
        base_talk<<"\t\t{bole_billboard,35},"<<endl;
        base_talk<<"\t\t{soul_binding,36},"<<endl;
        base_talk<<"\t\t{coin_binding,37},"<<endl;
        base_talk<<"\t\t{task_award,38},"<<endl;
        base_talk<<"\t\t{jobber,39},"<<endl;
        base_talk<<"\t\t{open_favorite,40},"<<endl;
        base_talk<<"\t\t{fb_list,41},"<<endl;
        base_talk<<"\t\t{fb_out,42},"<<endl;
        base_talk<<"\t\t{convoy_city,43},"<<endl;
        base_talk<<"\t\t{bind_stone,44},"<<endl;
        base_talk<<"\t\t{convoy_center,45},"<<endl;
        base_talk<<"\t\t{novice_card,46},"<<endl;
        base_talk<<"\t\t{arena_sign,47},"<<endl;
        base_talk<<"\t\t{arena_explain,48},"<<endl;
        base_talk<<"\t\t{integral_shop,49},"<<endl;
        base_talk<<"\t\t{all_fight,50},"<<endl;
        base_talk<<"\t\t{last_fight,51},"<<endl;
        base_talk<<"\t\t{honour_shop, 52},"<<endl;
        base_talk<<"\t\t{fst_nine, 53},"<<endl;
        base_talk<<"\t\t{fst_eighteen, 54},"<<endl;
        base_talk<<"\t\t{honour_twenty_seven, 55},"<<endl;
        base_talk<<"\t\t{arena_out,56},"<<endl;
        base_talk<<"\t\t{consign,57},"<<endl;
        base_talk<<"\t\t{arena_in,58},"<<endl;
        base_talk<<"\t\t{quit_box_scene, 59},"<<endl;
        base_talk<<"\t\t{enter_box_scene, 60},"<<endl;
        base_talk<<"\t\t{fst_one, 61},"<<endl;
        base_talk<<"\t\t{guild_boss_one, 62},"<<endl;
        base_talk<<"\t\t{guild_boss_two, 63},"<<endl;
        base_talk<<"\t\t{guild_boss_three, 64},"<<endl;
        base_talk<<"\t\t{apply_skyrush, 65},"<<endl;
        base_talk<<"\t\t{enter_sky, 66},"<<endl;
        base_talk<<"\t\t{skyrush_show, 67},"<<endl;
        base_talk<<"\t\t{applied_guilds, 68},"<<endl;
        base_talk<<"\t\t{feats_rank, 69},"<<endl;
        base_talk<<"\t\t{feats_shop, 70},"<<endl;
        base_talk<<"\t\t{quit_sky, 71},"<<endl;
        base_talk<<"\t\t{get_faward, 72},"<<endl;
        base_talk<<"\t\t{one_enter_demon, 73},"<<endl;
        base_talk<<"\t\t{many_enter_demon, 74},"<<endl;
        base_talk<<"\t\t{demon_fshop, 75},"<<endl;
        base_talk<<"\t\t{fashion_shop, 76},"<<endl;
        base_talk<<"\t\t{send_to_300,77},"<<endl;
        base_talk<<"\t\t{send_to_30,78},"<<endl;
        base_talk<<"\t\t{send_to_40,79},"<<endl;
        base_talk<<"\t\t{send_to_50,80},"<<endl;
        base_talk<<"\t\t{send_to_60,81},"<<endl;
        base_talk<<"\t\t{ring_bless,82},"<<endl;
        base_talk<<"\t\t{enter_train_practice,83},"<<endl;
        base_talk<<"\t\t{exit_train_practice,84},"<<endl;
        base_talk<<"\t\t{enter_zxt,85},"<<endl;
        base_talk<<"\t\t{enter_hotspring_vip, 86},"<<endl;
        base_talk<<"\t\t{enter_hotspring_normal, 87},"<<endl;
        base_talk<<"\t\t{leave_hotspring, 88},"<<endl;
        base_talk<<"\t\t{zxt_shop,89},"<<endl;
        base_talk<<"\t\t{send_to_70,90},"<<endl;
        base_talk<<"\t\t{sex_change,91},"<<endl;
        base_talk<<"\t\t{enter_war,92},"<<endl;
        base_talk<<"\t\t{exit_war,93},"<<endl;
        base_talk<<"\t\t{vip_drug,94},"<<endl;
        base_talk<<"\t\t{war_info,95},"<<endl;
        base_talk<<"\t\t{member_gwish,96},"<<endl;
        base_talk<<"\t\t{enter_pet_zoon,97},"<<endl;
        base_talk<<"\t\t{exit_pet_zoon,98},"<<endl;
        base_talk<<"\t\t{war_gold_shop,99},"<<endl;
        base_talk<<"\t\t{join_castle_rush, 101},"<<endl;
        base_talk<<"\t\t{enter_castle_rush, 102},"<<endl;
        base_talk<<"\t\t{castle_rush_intro, 103},"<<endl;
        base_talk<<"\t\t{castle_rush_join_list, 104},"<<endl;
        base_talk<<"\t\t{castle_rush_score_rank, 105},"<<endl;
        base_talk<<"\t\t{castle_rush_tax, 106},"<<endl;
        base_talk<<"\t\t{leave_castle_rush, 107},"<<endl;
        base_talk<<"\t\t{war_shop,108},"<<endl;
        base_talk<<"\t\t{enter_warfare, 109},"<<endl;
        base_talk<<"\t\t{leave_warfare, 110},"<<endl;
        base_talk<<"\t\t{propose,111},"<<endl;
        base_talk<<"\t\t{book_wedding,112},"<<endl;
        base_talk<<"\t\t{mass,113},"<<endl;
        base_talk<<"\t\t{begin_weddings,114},"<<endl;
        base_talk<<"\t\t{wedding_gift,115},"<<endl;
        base_talk<<"\t\t{leave_wedding,116},"<<endl;
        base_talk<<"\t\t{divorce,117},"<<endl;
        base_talk<<"\t\t{cancel_wedding,118},"<<endl;
        base_talk<<"\t\t{view_wedding,119},"<<endl;
        base_talk<<"\t\t{wish_tree, 120},"<<endl;
        base_talk<<"\t\t{fst_shop, 121},"<<endl;
        base_talk<<"\t\t{weapon_destory,122},"<<endl;
        base_talk<<"\t\t{arena_award, 123}"<<endl;
        base_talk<<"\t],"<<endl;
        base_talk<<"\tproplists:get_value(Key, L, 0)."<<endl;
        base_talk<<endl;

        q.get_result("select id,content from base_talk where 1");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int id = q.getval();
            std::string con = q.getstr();
            cout<<"task_talk-"<<id<<endl;
            json_spirit::mValue value;
            json_spirit::read(con, value);
            if (value.type() == array_type)
            {
                base_talk<<"get("<<id<<")->"<<endl;
                base_talk<<"[";
                mArray& mArr = value.get_array();
                for (mArray::iterator it = mArr.begin(); it != mArr.end(); ++it)
                {
                    mValue& v = *it;
                    if (v.type() == array_type)
                    {
                        base_talk<<"[";
                        mArray& a = v.get_array();
                        for (mArray::iterator it2 = a.begin(); it2 != a.end(); ++it2)
                        {
                            base_talk<<"{";
                            mValue& vv = *it2;
                            if (vv.type() == array_type)
                            {
                                mArray& aa = vv.get_array();
                                if (aa.size() < 2)
                                {
                                    cout<<"2. size not 2 or 3, is "<<aa.size()<<endl;
                                    return 0;
                                }
                                else
                                {
                                    mArray::iterator it3 = aa.begin();
                                    std::string field = it3->get_str();
                                    ++it3;
                                    std::string value = it3->get_str();
                                    std::string newvalue = unicode_to_utf8(value);
                                    ++it3;
                                    std::string extra = "[]";
                                    if (it3 != aa.end())
                                    {
                                        extra = "[" + it3->get_str() + "]";
                                    }
                                    base_talk<<field<<","<<"<<"<<newvalue<<">>,"<<extra;
                                    if (it2 != a.end())
                                    {
                                        base_talk<<",";
                                    }
                                }
                            }
                            else
                            {
                                cout<<"2. not array type."<<endl;
                                return 0;
                            }
                            base_talk<<"}";
                        }
                        base_talk<<"[";
                    }
                    else
                    {
                        cout<<"2 not array type."<<endl;
                        return 0;
                    }
                }    
                base_talk<<"];"<<endl;
            }
            else
            {
                cout<<"1 not array type."<<endl;
                return 0;
            }
        }
        q.free_result();

        base_talk<<"get(_Id) ->"<<endl;
        base_talk<<"    []."<<endl;

        base_talk.close();
        
        cout<<"gen base data done."<<endl;
    }
    else
    {
        cout<<"unknow "<<dowhat<<endl;
    }

    return 0;
}

