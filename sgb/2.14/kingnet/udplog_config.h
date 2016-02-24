 

#ifndef _UDPLOG_CONFIG_H_
#define _UDPLOG_CONFIG_H_


#include "udplog_typedef.h"
#include "tinyxml.h"
#include "udplog_singleton.h"

#include <map>
#include <vector>
#include <string>

struct stHost
{
	char szIP[MAX_IPADDR_LENGTH];
	int16_t nPort;
};

class CUdpLogConfig
{
public:
	CUdpLogConfig();
	virtual ~CUdpLogConfig();

public:
	int32_t Init(const char* szFileName = DEFAULT_CONFIGFILENAME);
	
	//���ȡһ��host���أ�����ֵ < 0 ��ʾ��ȡʧ��
	int32_t GetHostRandomly(stHost& host);
	
	//�����ļ����ֻ�ȡ��Ӧ��ID������ֵ < 0 ��ʾ��ȡʧ��
	int32_t GetLogFileID(const char* typeinfo);

private:

	int32_t InitServerInfo(TiXmlElement* pRoot);

	int32_t InitFileTypeInfo(TiXmlElement* pRoot);

private:
	std::map<std::string, int32_t>	m_mLogFileTypeInfo;
	std::vector<stHost>				m_vHosts;
};

#define	CREATE_UDPLOGCONFIG_INSTANCE	CSingleton<CUdpLogConfig>::CreateInstance
#define	GET_UDPLOGCONFIG_INSTANCE		CSingleton<CUdpLogConfig>::GetInstance
#define	DESTROY_UDPLOGCONFIG_INSTANCE	CSingleton<CUdpLogConfig>::DestroyInstance

#define _config GET_UDPLOGCONFIG_INSTANCE()

#endif
