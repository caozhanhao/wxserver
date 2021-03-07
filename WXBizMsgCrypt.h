#pragma once

#include <string>
#include <stdint.h>
#include "tinyxml2.h"

namespace Tencent {

static const unsigned int kAesKeySize = 32;
static const unsigned int kAesIVSize = 16;
static const unsigned int kEncodingKeySize = 43;
static const unsigned int kRandEncryptStrLen = 16;
static const unsigned int kMsgLen = 4;
static const unsigned int kMaxBase64Size = 1000000000;
enum  WXBizMsgCryptErrorCode
{
    WXBizMsgCrypt_OK = 0,
    WXBizMsgCrypt_ValidateSignature_Error = -40001,
    WXBizMsgCrypt_ParseXml_Error = -40002,
    WXBizMsgCrypt_ComputeSignature_Error = -40003,
    WXBizMsgCrypt_IllegalAesKey = -40004,
    WXBizMsgCrypt_ValidateCorpid_Error = -40005,
    WXBizMsgCrypt_EncryptAES_Error = -40006,
    WXBizMsgCrypt_DecryptAES_Error = -40007,
    WXBizMsgCrypt_IllegalBuffer = -40008,
    WXBizMsgCrypt_EncodeBase64_Error = -40009,
    WXBizMsgCrypt_DecodeBase64_Error = -40010,
    WXBizMsgCrypt_GenReturnXml_Error = -40011,
};

class WXBizMsgCrypt
{
public:
    //构造函数
    // @param sToken: 企业微信后台，开发者设置的Token
    // @param sEncodingAESKey: 企业微信后台，开发者设置的EncodingAESKey
    // @param sReceiveId: 不同场景含义不同，详见文档

    WXBizMsgCrypt(const std::string &sToken, 
                    const std::string &sEncodingAESKey, 
                    const std::string &sReceiveId)
                    :m_sToken(sToken), m_sEncodingAESKey(sEncodingAESKey),m_sReceiveId(sReceiveId)
                    {   }
    
        //靠靠靠gettoken靠靠靠靠靠靠
    
	
    //验证URL
	// @param sMsgSignature: 签名串，对应URL参数的msg_signature
	// @param sTimeStamp: 时间戳，对应URL参数的timestamp
	// @param sNonce: 随机串，对应URL参数的nonce
	// @param sEchoStr: 随机串，对应URL参数的echostr
	// @param sReplyEchoStr: 解密之后的echostr，当return返回0时有效
	// @return：成功0，失败返回对应的错误码
	int VerifyURL(const std::string& sMsgSignature,
					const std::string& sTimeStamp,
					const std::string& sNonce,
					const std::string& sEchoStr,
					std::string& sReplyEchoStr);
    
    
    // 检验消息的真实性，并且获取解密后的明文
    // @param sMsgSignature: 签名串，对应URL参数的msg_signature
    // @param sTimeStamp: 时间戳，对应URL参数的timestamp
    // @param sNonce: 随机串，对应URL参数的nonce
    // @param sPostData: 密文，对应POST请求的数据
    // @param sMsg: 解密后的原文，当return返回0时有效
    // @return: 成功0，失败返回对应的错误码
    int DecryptMsg(const std::string &sMsgSignature,
                    const std::string &sTimeStamp,
                    const std::string &sNonce,
                    const std::string &sPostData,
                    std::string &sMsg);
            
            
    //将企业微信回复用户的消息加密打包
    // @param sReplyMsg:企业微信待回复用户的消息，xml格式的字符串
    // @param sTimeStamp: 时间戳，可以自己生成，也可以用URL参数的timestamp
    // @param sNonce: 随机串，可以自己生成，也可以用URL参数的nonce
    // @param sEncryptMsg: 加密后的可以直接回复用户的密文，包括msg_signature, timestamp, nonce, encrypt的xml格式的字符串,
    //                      当return返回0时有效
    // return：成功0，失败返回对应的错误码
    int EncryptMsg(const std::string &sReplyMsg,
                    const std::string &sTimeStamp,
                    const std::string &sNonce,
                    std::string &sEncryptMsg);
					
	int GetXmlField(const std::string & sPostData, const std::string & sField,std::					string &sEncryptMsg);
private:
    std::string m_sToken;
    std::string m_sEncodingAESKey;
    std::string m_sReceiveId;


private:

    // AES CBC
    int AES_CBCEncrypt( const char * sSource, const uint32_t iSize,
            const char * sKey, unsigned int iKeySize, std::string * poResult );
    
    int AES_CBCEncrypt( const std::string & objSource,
            const std::string & objKey, std::string * poResult );
    
    int AES_CBCDecrypt( const char * sSource, const uint32_t iSize,
            const char * sKey, uint32_t iKeySize, std::string * poResult );
    
    int AES_CBCDecrypt( const std::string & objSource,
            const std::string & objKey, std::string * poResult );
    
    //base64
    int EncodeBase64(const std::string sSrc, std::string & sTarget);
    
    int DecodeBase64(const std::string sSrc, std::string & sTarget);
    
    //genkey
    int GenAesKeyFromEncodingKey( const std::string & sEncodingKey, std::string & sAesKey);
    
    //signature
    int ComputeSignature(const std::string sToken, const std::string sTimeStamp, const std::string & sNonce,
        const std::string & sMessage, std::string & sSignature);
    
    int ValidateSignature(const std::string &sMsgSignature, const std::string &sTimeStamp, 
        const std::string &sNonce, const std::string & sEncryptMsg);  

    //get , set data
    void GenRandStr(std::string & sRandStr, uint32_t len);

    void GenNeedEncryptData(const std::string &sReplyMsg,std::string & sNeedEncrypt );



    int SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName, 
        const std::string & value, bool bIsCdata);

    int GenReturnXml(const std::string & sEncryptMsg, const std::string & sSignature, const std::string & sTimeStamp, 
        const std::string & sNonce, std::string & sResult);


};

}

