#ifndef _PDU_H_
#define _PDU_H_

// �û���Ϣ���뷽ʽ
#define GSM_7BIT        0
#define GSM_8BIT        4
#define GSM_UCS2        8

// ����Ϣ�����ṹ������/���빲��
// ���У��ַ�����0��β
typedef struct {
	char SCA[16];       // ����Ϣ�������ĺ���(SMSC��ַ)
	char TPA[16];       // Ŀ������ظ�����(TP-DA��TP-RA)
	char TP_PID;        // �û���ϢЭ���ʶ(TP-PID)
	char TP_DCS;        // �û���Ϣ���뷽ʽ(TP-DCS)
	char TP_SCTS[16];   // ����ʱ����ַ���(TP_SCTS), ����ʱ�õ�
	char TP_UD[161];    // ԭʼ�û���Ϣ(����ǰ�������TP-UD)
	//char index;         // ����Ϣ��ţ��ڶ�ȡʱ�õ�
} PDU_PARAM;

int gsmEncode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength);
int gsmDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength);

int gsmEncode8bit(const char* pSrc,unsigned char* pDst,int nSrcLength);
int gsmDecode8bit(const unsigned char* pSrc, char* pDst, int nSrcLength);

int gsmEncodeUcs2(const char* pSrc, unsigned char* pDst, int nSrcLength);
int gsmDecodeUcs2(const unsigned char* pSrc, char* pDst, int nSrcLength);

int gsmString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength);
int gsmBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength);

int gsmInvertNumbers(const char* pSrc, char* pDst, int nSrcLength);
int gsmSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength);
int gsmEncodePdu(const PDU_PARAM* pSrc, char* pDst);
int gsmDecodePdu(const char* pSrc, PDU_PARAM* pDst);

#if 0
BOOL gsmSendMessage(const PDU_PARAM* pSrc);
int gsmReadMessage(PDU_PARAM* pMsg);
BOOL gsmDeleteMessage(const int index);
BOOL gsmGetCSCA();
BOOL gsmGetMessage(PDU_PARAM* pMsg, int index);

void convertToSmparam(Message msg, PDU_PARAM* pMsg);
void sendMessage(Message msg);
#endif

#endif
