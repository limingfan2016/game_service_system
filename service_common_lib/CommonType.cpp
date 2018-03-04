
/* author : limingfan
 * date : 2015.03.20
 * description : 各服务公用的数据类型定义及API
 */

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <unordered_map>

#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/des.h>

#include "CommonType.h"
#include "base/Function.h"
#include "base/CCfg.h"
#include "SrvFrame/IService.h"


namespace NProject
{

// http服务类型
enum EHttpServiceType
{
    Common = 0,                 // 公用的http接入服务
	TX = 1,                     // 腾讯充值操作专用的http服务
	MaxCount,
};

static uint32_t http_service_id[EHttpServiceType::MaxCount];

typedef unordered_map<uint32_t, UIntVector> ServiceTypeToIdMap;    // 服务类型到服务ID信息
static ServiceTypeToIdMap service_id_info;


bool initServiceIDList(NCommon::CCfg *pcfg)
{
	const NCommon::Key2Value *pKey2Value = pcfg->getList("ServiceID");
	while (pKey2Value)
	{
		if (0 == strcmp("MaxServiceId", pKey2Value->key))
		{
			pKey2Value = pKey2Value->pNext;
			continue;
		}

		const unsigned int service_id = atoi(pKey2Value->value);
		service_id_info[service_id / ServiceIdBaseValue].push_back(service_id);

		pKey2Value = pKey2Value->pNext;
	}
	
	memset(http_service_id, 0, sizeof(http_service_id));
	const char* specialServiceIDItem = "SpecialServiceID";
	const char* http_id = pcfg->get(specialServiceIDItem, "common_http_service");
	if (http_id != NULL && atoi(http_id) > 0)
	{
		// 初始化公用的http服务ID
		http_service_id[EHttpServiceType::Common] = atoi(http_id);
        service_id_info[ServiceType::HttpOperationSrv].push_back(http_service_id[EHttpServiceType::Common]);
	}
	
	// 腾讯充值操作专用的http服务ID
	http_id = pcfg->get(specialServiceIDItem, "tx_http_service");
	if (http_id != NULL && atoi(http_id) > 0)
	{
		http_service_id[EHttpServiceType::TX] = atoi(http_id);
	}

	return true;
}

unsigned int getDestServiceID(uint32_t dest_service_type, const char *str, const int str_len)
{
    const UIntVector& srvIds = service_id_info[dest_service_type];
	return !srvIds.empty() ? srvIds[NCommon::strToHashValue(str, str_len) % srvIds.size()] : 0;
}

unsigned int getDestServiceID(uint32_t dest_service_type, const unsigned int index)
{
    const UIntVector& srvIds = service_id_info[dest_service_type];
	return !srvIds.empty() ? srvIds[index % srvIds.size()] : 0;
}

bool getDestServiceID(uint32_t dest_service_type, const char *str, const int str_len, uint32_t &service_id)
{
	service_id = getDestServiceID(dest_service_type, str, str_len);
	return service_id > 0;
}

bool getDestServiceID(uint32_t dest_service_type, const unsigned int index, uint32_t &service_id)
{
	service_id = getDestServiceID(dest_service_type, index);
	return service_id > 0;
}

unsigned int getHttpServiceId(const unsigned int type)
{
	return 0;
}

void getSessionKey(const SessionKeyData& skd, char sessionKey[IdMaxLen])
{
	NCommon::b2str((const char *)&skd, 10, sessionKey, IdMaxLen);  // 将key的前10字节 转为16进制字串
}


int getCurrentYearDay()
{
	const time_t curSecs = time(NULL);
	struct tm curTime;
	localtime_r(&curSecs, &curTime);
	return curTime.tm_yday;
}

// 获取当前日期
const char* getCurrentDate(char* date, unsigned int len, const char* format)
{
	const time_t curSecs = time(NULL);
	struct tm curTime;
	localtime_r(&curSecs, &curTime);
	snprintf(date, len - 1, format, (curTime.tm_year + 1900), (curTime.tm_mon + 1), curTime.tm_mday);
	return date;
}

// 解析后返回物品类型
unsigned int parseGameGoodsType(const unsigned int type)
{
	if (type < EGameGoodsType::EMaxGameGoodsType) return type;
	
	return 0;
}


// 道具物品类型&名称
class CGoodsInfo
{
	typedef std::unordered_map<unsigned int, std::string> GoodsTypeToName;
public:
	CGoodsInfo()
	{
		// m_type2name[EGameGoodsType::EGoodsRMB] = "RMB";
	    // m_type2name[EGameGoodsType::EGoodsGold] = "金币";
		// m_type2name[EGameGoodsType::EGoodsRoomCard] = "房卡";
	}
	
	const char* getName(const unsigned int type)
	{
		GoodsTypeToName::const_iterator it = m_type2name.find(type);
		return (it != m_type2name.end()) ? it->second.c_str() : NULL;
	}
	
private:
	GoodsTypeToName m_type2name;
};

// 根据物品类型获取物品中文名称
const char* getGameGoodsChineseName(const unsigned int type)
{
	static CGoodsInfo goodsInfo;
	return goodsInfo.getName(parseGameGoodsType(type));
}


// MD5加密
void md5Encrypt(const char* data, unsigned int len, char* md5Buff, bool lowercase)
{
	*md5Buff = '\0';
	const char* format = lowercase ? "%2.2x" : "%02X";
	unsigned char md[16] = {'\0'};
	char tmp[3] = {'\0'};
	MD5((const unsigned char*)data, len, md);
	for (int i = 0; i < 16; i++)
	{
		sprintf(tmp, format, md[i]);
		strcat(md5Buff, tmp);
	}
}

// base64编码
bool base64Encode(const char* msg, int len, char* buffer, unsigned int& bufferLen, bool noNL)
{
	if (msg == NULL || len < 1 || buffer == NULL || bufferLen < 1) return false;

	BIO* b64 = BIO_new(BIO_f_base64());
	if (noNL) BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);	

	bool isOk = false;
	BIO* bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	if (BIO_write(b64, msg, len) > 0 && BIO_flush(b64) == 1)
	{
		BUF_MEM* bptr = NULL;
		BIO_get_mem_ptr(b64, &bptr);
		if (bptr != NULL && bptr->length < bufferLen)
		{
			memcpy(buffer, bptr->data, bptr->length);
			buffer[bptr->length] = 0;
			bufferLen = bptr->length;
			isOk = true;
		}
	}
	BIO_free_all(b64);

	return isOk;
}

// base64解码
int base64Decode(const char* str, int len, char* buffer, unsigned int bufferLen, bool noNL)
{
	if (str == NULL || len < 1 || buffer == NULL || bufferLen < 1) return -1;
	
	// 编码的结果必须是4的倍数，不足则补充 '=' 字符
	const unsigned int addLen = len % 4;
	if (addLen > 0)
	{
		if ((len + addLen) > bufferLen) return -1;
		
		memcpy(buffer, str, len);
		for (unsigned int idx = 0; idx < addLen; ++idx) buffer[len++] = '=';
		str = buffer;
	}

	BIO* b64 = BIO_new(BIO_f_base64());
	if (noNL) BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	BIO* bmem = BIO_new_mem_buf((void*)str, len);
	b64 = BIO_push(b64, bmem);
	int rdLen = BIO_read(b64, buffer, bufferLen);
	if (rdLen > 0) buffer[rdLen] = 0;
	BIO_free_all(b64);

	return rdLen;
}

// 先对消息做SHA1摘要，接着对摘要做RSA签名
bool sha1RSASign(const char* privateKey, unsigned int privateKeyLen, const unsigned char* msg, unsigned int len, unsigned char* sigret, unsigned int& siglen, bool needSha1)
{
	BIO* memBIO = BIO_new(BIO_s_mem());
	if (memBIO == NULL) return false;
	
	// 读私钥
	if (BIO_write(memBIO, privateKey, privateKeyLen) < 1)
	{
		BIO_vfree(memBIO);
		return false;
	}
	
	RSA* rsa = PEM_read_bio_RSAPrivateKey(memBIO, NULL, NULL, NULL);
	if (rsa == NULL)
	{
		BIO_vfree(memBIO);
		return false;
	}
	
	int rc = -1;
	if ((int)siglen > RSA_size(rsa))
	{
		unsigned char sha1Result[SHA_DIGEST_LENGTH] = {0};
		if (needSha1)
		{
			SHA1(msg, len, sha1Result);  // 做SHA1哈希
			msg = sha1Result;
			len = SHA_DIGEST_LENGTH;
		}
		
		// RSA签名
		rc = RSA_sign(NID_sha1, msg, len, sigret, &siglen, rsa);
		if (rc == 1) sigret[siglen] = 0;
	}
	
	RSA_free(rsa);
	BIO_vfree(memBIO);
	
	return (rc == 1);
}

// SHA1摘要做RSA验证
bool sha1RSAVerify(const char* publicKey, unsigned int publicKeyLen, const unsigned char* msg, unsigned int len, const unsigned char* sigret, unsigned int siglen, bool needSha1)
{
	BIO* memBIO = BIO_new(BIO_s_mem());
	if (memBIO == NULL) return false;
	
	// 读公钥
	if (BIO_write(memBIO, publicKey, publicKeyLen) < 1)
	{
		BIO_vfree(memBIO);
		return false;
	}
	
	RSA* rsa = PEM_read_bio_RSA_PUBKEY(memBIO, NULL, NULL, NULL);
	if (rsa == NULL)
	{
		BIO_vfree(memBIO);
		return false;
	}
	
	unsigned char sha1Result[SHA_DIGEST_LENGTH] = {0};
	if (needSha1)
	{
		SHA1(msg, len, sha1Result);  // 做SHA1哈希
		msg = sha1Result;
		len = SHA_DIGEST_LENGTH;
	}

	// 签名验证
	int verifyResult = RSA_verify(NID_sha1, msg, len, sigret, siglen, rsa);
	RSA_free(rsa);
	BIO_vfree(memBIO);
	
	return (verifyResult == 1);
}

// 哈希1运算 消息认证码
char* HMAC_SHA1(const void* key, unsigned int keyLen, const unsigned char* data, unsigned int dataLen, unsigned char* md, unsigned int& mdLen)
{
	if (mdLen < EVP_MAX_MD_SIZE) return NULL;

	unsigned char* mdResult = HMAC(EVP_sha1(), key, keyLen, data, dataLen, md, &mdLen);
	if (mdResult != NULL) md[mdLen] = 0;
	
	return (char*)mdResult;
}

// 哈希运算消息认证
char* Compute_HMAC(const void* evpMd, const void* key, unsigned int keyLen, const unsigned char* data, unsigned int dataLen, unsigned char* md, unsigned int& mdLen)
{
	if (mdLen < EVP_MAX_MD_SIZE) return NULL;

	unsigned char* mdResult = HMAC((const EVP_MD*)evpMd, key, keyLen, data, dataLen, md, &mdLen);
	if (mdResult != NULL) md[mdLen] = 0;
	
	return (char*)mdResult;
}


// 转换为16进制
unsigned char toHex(unsigned char c)
{ 
	return  (c > 9) ? (c + 55) : (c + 48);
}

// 16进制转换为字符
bool fromHex(unsigned char hex, unsigned char& c)
{
	if (hex >= 'A' && hex <= 'Z') c = hex - 'A' + 10;
	else if (hex >= 'a' && hex <= 'z') c = hex - 'a' + 10;
	else if (hex >= '0' && hex <= '9') c = hex - '0';
	else return false;

	return true;
}

// URL编码
bool urlEncode(const char* str, unsigned int len, char* enCode, unsigned int& codeLen)
{
	unsigned int cLen = 0;
	for (unsigned int i = 0; i < len && cLen < codeLen; i++)
	{
		if (isalnum(str[i]) || (str[i] == '-') || (str[i] == '_') || (str[i] == '.') || (str[i] == '~'))
			enCode[cLen++] = str[i];
		else if (str[i] == ' ')
			enCode[cLen++] = '+';
		else
		{
			enCode[cLen++] = '%';
			enCode[cLen++] = toHex((unsigned char)str[i] >> 4);
			enCode[cLen++] = toHex((unsigned char)str[i] % 16);
		}
	}
	
	if (cLen < codeLen)
	{
		enCode[cLen] = 0;
		codeLen = cLen;
		return true;
	}
	
	return false;
}

// URL解码
bool urlDecode(const char* enCode, unsigned int codeLen, char* str, unsigned int& len)
{
	unsigned int sLen = 0;
	for (unsigned int i = 0; i < codeLen && sLen < len; i++)
	{
		if (enCode[i] == '+')
			str[sLen++] = ' ';
		else if (enCode[i] == '%')
		{
			unsigned char high;
			unsigned char low;
			if ((i + 2 >= codeLen) || !fromHex((unsigned char)enCode[++i], high) || !fromHex((unsigned char)enCode[++i], low)) return false;

			str[sLen++] = high * 16 + low;
		}
		else
			str[sLen++] = enCode[i];
	}
	
	if (sLen < len)
	{
		str[sLen] = 0;
		len = sLen;
		return true;
	}

	return false;
}

//3DES ECB解码
string dESEcb3Encrypt(const string &ciphertext, const string &key)
{
	string strClearText;
	DES_cblock ke1, ke2, ke3;
	memset(ke1, 0, 8);
	memset(ke2, 0, 8);
	memset(ke2, 0, 8);

	if (key.length() >= 24)
	{
	    memcpy(ke1, key.c_str(), 8);
	    memcpy(ke2, key.c_str() + 8, 8);
	    memcpy(ke3, key.c_str() + 16, 8);
	}
	else if (key.length() >= 16)
	{
		memcpy(ke1, key.c_str(), 8);
		memcpy(ke2, key.c_str() + 8, 8);
		memcpy(ke3, key.c_str() + 16, key.length() - 16);
	}
	else if (key.length() >= 8)
	{
		memcpy(ke1, key.c_str(), 8);
		memcpy(ke2, key.c_str() + 8, key.length() - 8);
		memcpy(ke3, key.c_str(), 8);
	}
	else
	{
		memcpy(ke1, key.c_str(), key.length());
		memcpy(ke2, key.c_str(), key.length());
		memcpy(ke3, key.c_str(), key.length());
	}

	DES_key_schedule ks1, ks2, ks3;
	DES_set_key_unchecked(&ke1, &ks1);
	DES_set_key_unchecked(&ke2, &ks2);
	DES_set_key_unchecked(&ke3, &ks3);

	const_DES_cblock inputText;
	DES_cblock outputText;
	vector<unsigned char> vecCleartext;
	unsigned char tmp[8];

	for (int i = 0; i < (int)ciphertext.length() / 8; i++)
	{
		memcpy(inputText, ciphertext.c_str() + i * 8, 8);
		DES_ecb3_encrypt(&inputText, &outputText, &ks1, &ks2, &ks3, DES_DECRYPT);
		memcpy(tmp, outputText, 8);

		for (int j = 0; j < 8; j++) vecCleartext.push_back(tmp[j]);
	}

	if (ciphertext.length() % 8 != 0)
	{
		int tmp1 = ciphertext.length() / 8 * 8;
		int tmp2 = ciphertext.length() - tmp1;
		memset(inputText, 0, 8);
		memcpy(inputText, ciphertext.c_str() + tmp1, tmp2);

		DES_ecb3_encrypt(&inputText, &outputText, &ks1, &ks2, &ks3, DES_DECRYPT);
		memcpy(tmp, outputText, 8);

		for (int j = 0; j < 8; j++) vecCleartext.push_back(tmp[j]);
	}

	strClearText.clear();
	strClearText.assign(vecCleartext.begin(), vecCleartext.end());

	return strClearText;
}


}

