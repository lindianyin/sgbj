 
#include "udplog_config.h"
#include <cstdio>
#include <ctime>
#include <iostream>

#define INFO(x) std::cout<<x<<std::endl
#define ERR() std::cout<<"!!!!!!!!!!!!!!!!!error!!!!!!!!!!!!!!!!:"<<__FILE__<<" at line "<<__LINE__<<std::endl<<std::flush


#define TAGNAME_UDPLOG_SERVER                "udplog_server"
#define TAGNAME_LOGFILE_TYPE                "log_file_type"
#define TAGNAME_ITEM                        "item"

#define ATTRIBUTE_ID                        "id"
#define ATTRIBUTE_IP                        "ip"
#define ATTRIBUTE_PORT                        "port"
#define ATTRIBUTE_TYPEINFO                    "typeinfo"



CUdpLogConfig::CUdpLogConfig()
{
}

CUdpLogConfig::~CUdpLogConfig()
{

}

int32_t CUdpLogConfig::InitServerInfo(TiXmlElement* pRoot)
{
    if(NULL == pRoot)
    {
        return E_FAIL;
    }
    TiXmlElement *pServer = pRoot->FirstChildElement(TAGNAME_UDPLOG_SERVER);
    if(NULL == pServer)
    {
        return E_FAIL;
    }
    
    m_vHosts.clear();
    
    //test updatelog
    INFO("CUdpLogConfig::InitServerInfo**************");
    //end

    const char* pszValue = NULL;
    TiXmlElement *pItem = pServer->FirstChildElement(TAGNAME_ITEM);
    while (NULL != pItem)
    {
        stHost host;
        pszValue = pItem->Attribute(ATTRIBUTE_IP);
        if (NULL == pszValue)
        {
            return E_FAIL;
        }
#ifndef WIN32
        strncpy(host.szIP, pszValue, MAX_IPADDR_LENGTH-1);
#else
        strncpy_s(host.szIP, sizeof(host.szIP), pszValue, strlen(pszValue));
#endif
        int32_t port ;
        pszValue = pItem->Attribute(ATTRIBUTE_PORT,&port);
        if (NULL == pszValue)
        {
            return E_FAIL;
        }
        host.nPort = (int16_t)port;
        
        m_vHosts.push_back(host);
        
        //test updatelog
        INFO("ip_addr="<< host.szIP <<",port=" << host.nPort <<" push_back**************");
        //end

        pItem = pItem->NextSiblingElement();
    }
    return S_OK;
}

int32_t CUdpLogConfig::InitFileTypeInfo(TiXmlElement* pRoot)
{
    if(NULL == pRoot)
    {
        return E_FAIL;
    }

    TiXmlElement *pServer = pRoot->FirstChildElement(TAGNAME_LOGFILE_TYPE);
    if(NULL == pServer)
    {
        return E_FAIL;
    }
    
    m_mLogFileTypeInfo.clear();

    //test updatelog
    INFO("CUdpLogConfig::InitFileTypeInfo**************");
    //end

    const char* pszValue = NULL;
    TiXmlElement *pItem = pServer->FirstChildElement(TAGNAME_ITEM);
    while (NULL != pItem)
    {
        pszValue = pItem->Attribute(ATTRIBUTE_TYPEINFO);
        if (NULL == pszValue)
        {
            return E_FAIL;
        }
        std::string file = pszValue;
        
        int32_t id;

        pszValue = pItem->Attribute(ATTRIBUTE_ID,&id);
        if (NULL == pszValue)
        {
            return E_FAIL;
        }
        m_mLogFileTypeInfo[file] = id;

        //test updatelog
        //INFO("file="<< file <<",id=" << id <<" push_back**************");
        //end

        pItem = pItem->NextSiblingElement();
    }
    return S_OK;
}


int32_t CUdpLogConfig::Init(const char* szFileName/* = DEFAULT_CONFIGFILENAME*/)
{
    //test updatelog
    INFO("****************CUdpLogConfig::Init is call**************");
    INFO("path=" << szFileName);
    //end
    srand((uint32_t)time(NULL));

    TiXmlDocument doc(szFileName); 
    if (!doc.LoadFile(TIXML_ENCODING_UTF8))
    {
        ERR();
        return E_FAIL;
    }

    //获取根节点
    TiXmlElement *pRoot = doc.RootElement();
    if (NULL == pRoot)
    {
        ERR();
        return E_FAIL;
    }
    int32_t ret = InitServerInfo(pRoot);
    if(ret < 0)
    {
        ERR();
        return ret;
    }
    ret = InitFileTypeInfo(pRoot);
    if(ret < 0)
    {
        ERR();
        return ret;
    }

    return S_OK;
}

int32_t CUdpLogConfig::GetHostRandomly(stHost& host)
{
    int32_t size = (int32_t)m_vHosts.size();
    if(0 == size)
    {
        return E_FAIL;
    }
    
    int32_t index = rand()%size;
    
    host = m_vHosts[index];

    return S_OK;
}

int32_t CUdpLogConfig::GetLogFileID(const char* typeinfo)
{
    if(NULL == typeinfo)
    {
        return E_FAIL;
    }
    std::map<std::string, int32_t>::iterator it = m_mLogFileTypeInfo.find(typeinfo);
    if(it != m_mLogFileTypeInfo.end())
    {
        return it->second;
    }
    return E_FAIL;
}

