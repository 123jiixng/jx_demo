/*
 * define pdu codec function
 * porting from GSMOperation.cpp of IntegratedMessagePlatform
*/

#include <string.h>

#include "pdu.h"

// 7-bit����
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ����봮ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ����봮����
int gsmEncode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nSrc;        // Դ�ַ����ļ���ֵ
	int nDst;        // Ŀ����봮�ļ���ֵ
	int nChar;       // ��ǰ���ڴ���������ַ��ֽڵ���ţ���Χ��0-7
	unsigned char nLeft=0;    // ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nSrc = 0;
	nDst = 0;

	// ��Դ��ÿ8���ֽڷ�Ϊһ�飬ѹ����7���ֽ�
	// ѭ���ô�����̣�ֱ��Դ����������
	// ������鲻��8�ֽڣ�Ҳ����ȷ����
	while(nSrc<nSrcLength)
	{
		// ȡԴ�ַ����ļ���ֵ�����3λ
		nChar = nSrc & 7;

		// ����Դ����ÿ���ֽ�
		if(nChar == 0)
		{
			// ���ڵ�һ���ֽڣ�ֻ�Ǳ�����������������һ���ֽ�ʱʹ��
			nLeft = *pSrc;
		}

		else
		{
			// ���������ֽڣ������ұ߲��������������ӣ��õ�һ��Ŀ������ֽ�
			*pDst = (*pSrc << (8-nChar)) | nLeft;

			// �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
			nLeft = *pSrc >> nChar;

			// �޸�Ŀ�괮��ָ��ͼ���ֵ
			pDst++;
			nDst++; 
		} 

		// �޸�Դ����ָ��ͼ���ֵ
		pSrc++; nSrc++;
	}

	// ����Ŀ�괮����
	return nDst; 
}

// 7-bit����
// pSrc: Դ���봮ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���봮����
// ����: Ŀ���ַ�������
int gsmDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	int nSrc;        // Դ�ַ����ļ���ֵ
	int nDst;        // Ŀ����봮�ļ���ֵ
	int nByte;       // ��ǰ���ڴ���������ֽڵ���ţ���Χ��0-6
	unsigned char nLeft;    // ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nSrc = 0;
	nDst = 0;

	// �����ֽ���źͲ������ݳ�ʼ��
	nByte = 0;
	nLeft = 0;

	// ��Դ����ÿ7���ֽڷ�Ϊһ�飬��ѹ����8���ֽ�
	// ѭ���ô�����̣�ֱ��Դ���ݱ�������
	// ������鲻��7�ֽڣ�Ҳ����ȷ����
	while(nSrc<nSrcLength)
	{
		// ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
		*pDst = ((*pSrc << nByte) | nLeft) & 0x7f;
		// �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
		nLeft = *pSrc >> (7-nByte);

		// �޸�Ŀ�괮��ָ��ͼ���ֵ
		pDst++;
		nDst++;

		// �޸��ֽڼ���ֵ
		nByte++;

		// ����һ������һ���ֽ�
		if(nByte == 7)
		{
			// ����õ�һ��Ŀ������ֽ�
			*pDst = nLeft;

			// �޸�Ŀ�괮��ָ��ͼ���ֵ
			pDst++;
			nDst++;

			// �����ֽ���źͲ������ݳ�ʼ��
			nByte = 0;
			nLeft = 0;
		}

		// �޸�Դ����ָ��ͼ���ֵ
		pSrc++;
		nSrc++;
	}

	*pDst = 0;

	// ����Ŀ�괮����
	return nDst;
}

//   8bit����
//   ����:   pSrc   -   Դ�ַ���ָ��
//               nSrcLength   -   Դ�ַ�������
//   ���:   pDst   -   Ŀ����봮ָ��
//   ����:   Ŀ����봮����
int gsmEncode8bit(const char* pSrc,unsigned char* pDst,int nSrcLength)
{
	//   �򵥸���
	memcpy(pDst,   pSrc,   nSrcLength);
	return   nSrcLength;
}

//   8bit����
//   ����:   pSrc   -   Դ���봮ָ��
//   nSrcLength   -     Դ���봮����
//   ���:   pDst   -     Ŀ���ַ���ָ��
//   ����:   Ŀ���ַ�������
int gsmDecode8bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	//   �򵥸���
	memcpy(pDst,   pSrc,   nSrcLength);
	//   ����ַ����Ӹ�������
	*pDst   =   '\0';
	return   nSrcLength;
}

// UCS2����
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ����봮ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ����봮����
int gsmEncodeUcs2(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
#if 0
	int nDstLength;        // UNICODE���ַ���Ŀ
	WCHAR wchar[128];      // UNICODE��������

	// �ַ���-->UNICODE��
	nDstLength = ::MultiByteToWideChar(CP_ACP, 0, pSrc, nSrcLength, wchar, 128);

	// �ߵ��ֽڶԵ������
	for(int i=0; i<nDstLength; i++)
	{
		// �������λ�ֽ�
		*pDst++ = wchar[i] >> 8;
		// �������λ�ֽ�
		*pDst++ = wchar[i] & 0xff;
	}

	// ����Ŀ����봮����
	return nDstLength * 2;
#else
	return 0;
#endif
}

// UCS2����
// pSrc: Դ���봮ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���봮����
// ����: Ŀ���ַ�������
int gsmDecodeUcs2(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
#if 0
	int nDstLength;        // UNICODE���ַ���Ŀ
	WCHAR wchar[128];      // UNICODE��������

	// �ߵ��ֽڶԵ���ƴ��UNICODE
	for(int i=0; i<nSrcLength/2; i++)
	{
		// �ȸ�λ�ֽ�
		wchar[i] = *pSrc++ << 8;

		// ���λ�ֽ�
		wchar[i] |= *pSrc++;
	}

	// UNICODE��-->�ַ���
	nDstLength = ::WideCharToMultiByte(CP_ACP, 0, wchar, nSrcLength/2, pDst, 160, NULL, NULL);

	// ����ַ����Ӹ�������    
	pDst[nDstLength] = '\0';    

	// ����Ŀ���ַ�������
	return nDstLength;
#else
	return 0;
#endif
}

// �ɴ�ӡ�ַ���ת��Ϊ�ֽ�����
// �磺"C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ������ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ�����ݳ���
int gsmString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int i;

	for(i=0; i<nSrcLength; i+=2)
	{
		// �����4λ
		if(*pSrc>='0' && *pSrc<='9')
		{
			*pDst = (*pSrc - '0') << 4;
		}
		else
		{
			*pDst = (*pSrc - 'A' + 10) << 4;
		}

		pSrc++;

		// �����4λ
		if(*pSrc>='0' && *pSrc<='9')
		{
			*pDst |= *pSrc - '0';
		}
		else
		{
			*pDst |= *pSrc - 'A' + 10;
		}

		pSrc++;
		pDst++;
	}

	// ����Ŀ�����ݳ���
	return nSrcLength / 2;
}

// �ֽ�����ת��Ϊ�ɴ�ӡ�ַ���
// �磺{0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01} --> "C8329BFD0E01" 
// pSrc: Դ����ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���ݳ���
// ����: Ŀ���ַ�������
int gsmBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	const char tab[]="0123456789ABCDEF";    // 0x0-0xf���ַ����ұ�
	int i;

	for(i=0; i<nSrcLength; i++)
	{
		// �����4λ
		*pDst++ = tab[*pSrc >> 4];

		// �����4λ
		*pDst++ = tab[*pSrc & 0x0f];

		pSrc++;
	}

	// ����ַ����Ӹ�������
	*pDst = '\0';

	// ����Ŀ���ַ�������
	return nSrcLength * 2;
}

// ����˳����ַ���ת��Ϊ�����ߵ����ַ�����������Ϊ��������'F'�ճ�ż��
// �磺"8613851872468" --> "683158812764F8"
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ���ַ�������
int gsmInvertNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;   // Ŀ���ַ�������
	char ch;          // ���ڱ���һ���ַ�
	int i;

	// ���ƴ�����
	nDstLength = nSrcLength;

	// �����ߵ�
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;        // �����ȳ��ֵ��ַ�
		*pDst++ = *pSrc++;   // ���ƺ���ֵ��ַ�
		*pDst++ = ch;        // �����ȳ��ֵ��ַ�
	}

	// Դ��������������
	if(nSrcLength & 1)
	{
		*(pDst-2) = 'F';     // ��'F'
		nDstLength++;        // Ŀ�괮���ȼ�1
	}

	// ����ַ����Ӹ�������
	*pDst = '\0';

	// ����Ŀ���ַ�������
	return nDstLength;
}

// �����ߵ����ַ���ת��Ϊ����˳����ַ���
// �磺"683158812764F8" --> "8613851872468"
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ���ַ�������
int gsmSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;   // Ŀ���ַ�������
	char ch;          // ���ڱ���һ���ַ�
	int i;

	// ���ƴ�����
	nDstLength = nSrcLength;

	// �����ߵ�
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;        // �����ȳ��ֵ��ַ�
		*pDst++ = *pSrc++;   // ���ƺ���ֵ��ַ�
		*pDst++ = ch;        // �����ȳ��ֵ��ַ�
	}

	// �����ַ���'F'��
	if(*(pDst-1) == 'F')
	{
		pDst--;
		nDstLength--;        // Ŀ���ַ������ȼ�1
	}

	// ����ַ����Ӹ�������
	*pDst = '\0';

	// ����Ŀ���ַ�������
	return nDstLength;
}

// PDU���룬���ڱ��ơ����Ͷ���Ϣ
// pSrc: ԴPDU����ָ��
// pDst: Ŀ��PDU��ָ��
// ����: Ŀ��PDU������
int gsmEncodePdu(const PDU_PARAM* pSrc, char* pDst)
{
	int nLength;             // �ڲ��õĴ�����
	int nDstLength;          // Ŀ��PDU������
	unsigned char buf[256];  // �ڲ��õĻ�����

	// SMSC��ַ��Ϣ��
	nLength = strlen(pSrc->SCA);    // SMSC��ַ�ַ����ĳ���    
	buf[0] = (char)((nLength & 1) == 0 ? nLength : nLength + 1) / 2 + 1;    // SMSC��ַ��Ϣ����
	buf[1] = 0x91;        // �̶�: �ù��ʸ�ʽ����
	nDstLength = gsmBytes2String(buf, pDst, 2);        // ת��2���ֽڵ�Ŀ��PDU��
	nDstLength += gsmInvertNumbers(pSrc->SCA, &pDst[nDstLength], nLength);    // ת��SMSC��Ŀ��PDU��

	// TPDU�λ���������Ŀ���ַ��
	nLength = strlen(pSrc->TPA);    // TP-DA��ַ�ַ����ĳ���
	buf[0] = 0x11;            // �Ƿ��Ͷ���(TP-MTI=01)��TP-VP����Ը�ʽ(TP-VPF=10)
	buf[1] = 0;               // TP-MR=0
	buf[2] = (char)nLength;   // Ŀ���ַ���ָ���(TP-DA��ַ�ַ�����ʵ����)
	buf[3] = 0x91;            // �̶�: �ù��ʸ�ʽ����
	nDstLength += gsmBytes2String(buf, &pDst[nDstLength], 4);  // ת��4���ֽڵ�Ŀ��PDU��
	nDstLength += gsmInvertNumbers(pSrc->TPA, &pDst[nDstLength], nLength); // ת��TP-DA��Ŀ��PDU��

	// TPDU��Э���ʶ�����뷽ʽ���û���Ϣ��
	nLength = strlen(pSrc->TP_UD);    // �û���Ϣ�ַ����ĳ���
	buf[0] = pSrc->TP_PID;        // Э���ʶ(TP-PID)
	buf[1] = pSrc->TP_DCS;        // �û���Ϣ���뷽ʽ(TP-DCS)
	buf[2] = 0;            // ��Ч��(TP-VP)Ϊ5����
	if(pSrc->TP_DCS == GSM_7BIT)    
	{
		// 7-bit���뷽ʽ
		buf[3] = nLength;            // ����ǰ����
		nLength = gsmEncode7bit(pSrc->TP_UD, &buf[4], nLength+1) + 4;    // ת��TP-DA��Ŀ��PDU��
	}
	else if(pSrc->TP_DCS == GSM_UCS2)
	{
		// UCS2���뷽ʽ
		buf[3] = gsmEncodeUcs2(pSrc->TP_UD, &buf[4], nLength);    // ת��TP-DA��Ŀ��PDU��
		nLength = buf[3] + 4;        // nLength���ڸö����ݳ���
	}
	else
	{
		// 8-bit���뷽ʽ
		buf[3] = gsmEncode8bit(pSrc->TP_UD, &buf[4], nLength);    // ת��TP-DA��Ŀ��PDU��
		nLength = buf[3] + 4;        // nLength���ڸö����ݳ���
	}
	nDstLength += gsmBytes2String(buf, &pDst[nDstLength], nLength);        // ת���ö����ݵ�Ŀ��PDU��

	// ����Ŀ���ַ�������
	return nDstLength;
}

// PDU���룬���ڽ��ա��Ķ�����Ϣ
// pSrc: ԴPDU��ָ��
// pDst: Ŀ��PDU����ָ��
// ����: �û���Ϣ������
int gsmDecodePdu(const char* pSrc, PDU_PARAM* pDst)
{
	int nDstLength;          // Ŀ��PDU������
	unsigned char tmp;       // �ڲ��õ���ʱ�ֽڱ���
	unsigned char buf[256];  // �ڲ��õĻ�����

	// SMSC��ַ��Ϣ��
	gsmString2Bytes(pSrc, &tmp, 2);    // ȡ����
	tmp = (tmp - 1) * 2;    // SMSC���봮����
	pSrc += 4;              // ָ�����
	gsmSerializeNumbers(pSrc, pDst->SCA, tmp);    // ת��SMSC���뵽Ŀ��PDU��
	pSrc += tmp;        // ָ�����

	// TPDU�λ����������ظ���ַ��
	gsmString2Bytes(pSrc, &tmp, 2);    // ȡ��������
	pSrc += 2;        // ָ�����
		
	// �����ظ���ַ��ȡ�ظ���ַ��Ϣ
	gsmString2Bytes(pSrc, &tmp, 2);    // ȡ����
	if(tmp & 1) tmp += 1;    // ������ż��
	pSrc += 4;          // ָ�����
	gsmSerializeNumbers(pSrc, pDst->TPA, tmp);    // ȡTP-RA����
	pSrc += tmp;        // ָ�����

	// TPDU��Э���ʶ�����뷽ʽ���û���Ϣ��
	gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_PID, 2);    // ȡЭ���ʶ(TP-PID)
	pSrc += 2;        // ָ�����
	gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_DCS, 2);    // ȡ���뷽ʽ(TP-DCS)
	pSrc += 2;        // ָ�����
	gsmSerializeNumbers(pSrc, pDst->TP_SCTS, 14);        // ����ʱ����ַ���(TP_SCTS) 
	pSrc += 14;       // ָ�����
	gsmString2Bytes(pSrc, &tmp, 2);    // �û���Ϣ����(TP-UDL)
	pSrc += 2;        // ָ�����
	if(pDst->TP_DCS == GSM_7BIT)    
	{
		// 7-bit����
		nDstLength = gsmString2Bytes(pSrc, buf, tmp & 7 ? (int)tmp * 7 / 4 + 2 : (int)tmp * 7 / 4);  // ��ʽת��
		gsmDecode7bit(buf, pDst->TP_UD, nDstLength);    // ת����TP-DU
		nDstLength = tmp;
	}
	else if(pDst->TP_DCS == GSM_UCS2)
	{
		// UCS2����
		nDstLength = gsmString2Bytes(pSrc, buf, tmp * 2);        // ��ʽת��
		nDstLength = gsmDecodeUcs2(buf, pDst->TP_UD, nDstLength);    // ת����TP-DU
	}
	else
	{
		// 8-bit����
		nDstLength = gsmString2Bytes(pSrc, buf, tmp * 2);        // ��ʽת��
		nDstLength = gsmDecode8bit(buf, pDst->TP_UD, nDstLength);    // ת����TP-DU
	}

	// ����Ŀ���ַ�������
	return nDstLength;
}

#if 0
// ���Ͷ���Ϣ
// pSrc: ԴPDU����ָ��
BOOL gsmSendMessage(const PDU_PARAM* pSrc)
{
	int nPduLength;        // PDU������
	unsigned char nSmscLength;    // SMSC������
	int nLength;           // �����յ������ݳ���
	char cmd[16];          // ���
	char pdu[512];         // PDU��
	char ans[128];         // Ӧ��

	nPduLength = gsmEncodePdu(pSrc, pdu);    // ����PDU����������PDU��
	strcat(pdu, "\x01a");        // ��Ctrl-Z����

	gsmString2Bytes(pdu, &nSmscLength, 2);    // ȡPDU���е�SMSC��Ϣ����
	nSmscLength++;        // ���ϳ����ֽڱ���

	// �����еĳ��ȣ�������SMSC��Ϣ���ȣ��������ֽڼ�
	sprintf(cmd, "AT+CMGS=%d\r", nPduLength / 2 - nSmscLength);    // ��������

	WriteComm2(cmd, strlen(cmd));    // ��������
	Sleep(1000);
	nLength = ReadComm2(ans, 128);   // ��Ӧ������

	// �����ܷ��ҵ�"\r\n> "�����ɹ����
	if(nLength != 0 && strstr(ans, ">") != NULL)
	{
		WriteComm2(pdu, strlen(pdu));        // �õ��϶��ش𣬼������PDU��
		Sleep(1000);
		char ans1[128];
		nLength = ReadComm2(ans1, 128);       // ��Ӧ������

		// �����ܷ��ҵ�"+CMS ERROR"�����ɹ����
		if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// ��ȡ����Ϣ
// ��+CMGL����+CMGR����һ���Զ���ȫ������Ϣ
// pMsg: ����Ϣ�������������㹻��
// ����: ����Ϣ����
int gsmReadMessage(PDU_PARAM* pMsg)
{
	int nLength;        // �����յ������ݳ���
	int nMsg;           // ����Ϣ����ֵ
	char* ptr;          // �ڲ��õ�����ָ��
	char cmd[16];       // ���
	char ans[1024];     // Ӧ��

	nMsg = 0;
	ptr = ans;

	sprintf(cmd, "AT+CMGL\r");    // ��������

	WriteComm1(cmd, strlen(cmd));    // ������
	Sleep(10000);
	nLength = ReadComm1(ans, 1024);    // ��Ӧ������
	ans[nLength]='\0';
	printf("ans: %s\n", ans);
	// �����ܷ��ҵ�"+CMS ERROR"�����ɹ����
	if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
	{
		// ѭ����ȡÿһ������Ϣ, ��"+CMGL:"��ͷ
		while((ptr = strstr(ptr, "+CMGL:")) != NULL)
		{
			ptr += 6;        // ����"+CMGL:"
			sscanf(ptr, "%d", &pMsg->index);    // ��ȡ���
//			TRACE("  index=%d\n",pMsg->index);

			ptr = strstr(ptr, "\r\n");    // ����һ��
			ptr += 2;        // ����"\r\n"

			gsmDecodePdu(ptr, pMsg);    // PDU������
			pMsg++;        // ׼������һ������Ϣ
			nMsg++;        // ����Ϣ������1
		}
	}

	return nMsg;
}

// ɾ������Ϣ
// index: ����Ϣ��ţ���1��ʼ
BOOL gsmDeleteMessage(const int index)
{
	int nLength;          // �����յ������ݳ���
	char cmd[16];         // ���
	char ans[128];        // Ӧ��

	sprintf(cmd, "AT+CMGD=%d\r", index);    // ��������

	// ������
	WriteComm1(cmd, strlen(cmd));

	// ��Ӧ������
	nLength = ReadComm1(ans, 128);

	// �����ܷ��ҵ�"+CMS ERROR"�����ɹ����
	if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL gsmGetCSCA()
{
	int nLength;
	char cmd[16];
	char ans[128];
	sprintf(cmd, "AT+CSCA?\r");    // ��������
	WriteComm1(cmd, strlen(cmd));
	nLength = ReadComm1(ans, 128);   // ��Ӧ������
	return FALSE;
}

BOOL gsmGetMessage(PDU_PARAM* pMsg, int index){
	int nLength;        // �����յ������ݳ���
	char* ptr;          // �ڲ��õ�����ָ��
	char cmd[16];       // ���
	char ans[1024];     // Ӧ��

	ptr = ans;

	sprintf(cmd, "AT+CMGR=%d\r", index);    // ��������

	WriteComm1(cmd, strlen(cmd));    // ������
	Sleep(500);
	nLength = ReadComm1(ans, 1024);    // ��Ӧ������
	ans[nLength]='\0';

	// �����ܷ��ҵ�"+CMS ERROR"�����ɹ����
	if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
	{
		ptr = strstr(ptr, "+CMGR:");
//		printf("%s\n", ptr);
		ptr += 6;        // ����"+CMGL:"

		char tmp = *(ptr+1);
		if (tmp == '1')
			return false;

		sscanf(ptr, "%d", &pMsg->index);    // ��ȡ���
//			TRACE("  index=%d\n",pMsg->index);

		ptr = strstr(ptr, "\r\n");    // ����һ��
		ptr += 2;        // ����"\r\n"

		gsmDecodePdu(ptr, pMsg);    // PDU������
		return true;
	}
	return false;
}

void convertToSmparam(Message msg, PDU_PARAM* pMsg){
	strcpy(pMsg->SCA, "8613800210500");
	pMsg->TP_DCS = GSM_UCS2;
	strcpy(pMsg->TP_UD , msg.data.c_str());
	pMsg->TP_PID = 0;
	char m_cellphone[20];
//	strcpy(pMsg->TPA, 
	strcpy(m_cellphone, "86");
	strcat(m_cellphone, msg.get_cellphone().c_str());
	strcpy(pMsg->TPA, m_cellphone);
}

void sendMessage(Message msg){
	PDU_PARAM *pMsg = new PDU_PARAM();
	convertToSmparam(msg, pMsg);
	if(gsmSendMessage(pMsg)){
		printf("���ŷ��ͳɹ���\n");
		printf("-------------------------------------------------\n");
	}
	else{
		printf("���ŷ���ʧ�ܣ�\n");
		printf("-------------------------------------------------\n");
	}
}
#endif
