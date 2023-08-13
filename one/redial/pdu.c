/*
 * define pdu codec function
 * porting from GSMOperation.cpp of IntegratedMessagePlatform
*/

#include <string.h>

#include "pdu.h"

// 7-bit编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
int gsmEncode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nSrc;        // 源字符串的计数值
	int nDst;        // 目标编码串的计数值
	int nChar;       // 当前正在处理的组内字符字节的序号，范围是0-7
	unsigned char nLeft=0;    // 上一字节残余的数据

	// 计数值初始化
	nSrc = 0;
	nDst = 0;

	// 将源串每8个字节分为一组，压缩成7个字节
	// 循环该处理过程，直至源串被处理完
	// 如果分组不到8字节，也能正确处理
	while(nSrc<nSrcLength)
	{
		// 取源字符串的计数值的最低3位
		nChar = nSrc & 7;

		// 处理源串的每个字节
		if(nChar == 0)
		{
			// 组内第一个字节，只是保存起来，待处理下一个字节时使用
			nLeft = *pSrc;
		}

		else
		{
			// 组内其它字节，将其右边部分与残余数据相加，得到一个目标编码字节
			*pDst = (*pSrc << (8-nChar)) | nLeft;

			// 将该字节剩下的左边部分，作为残余数据保存起来
			nLeft = *pSrc >> nChar;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++; 
		} 

		// 修改源串的指针和计数值
		pSrc++; nSrc++;
	}

	// 返回目标串长度
	return nDst; 
}

// 7-bit解码
// pSrc: 源编码串指针
// pDst: 目标字符串指针
// nSrcLength: 源编码串长度
// 返回: 目标字符串长度
int gsmDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	int nSrc;        // 源字符串的计数值
	int nDst;        // 目标解码串的计数值
	int nByte;       // 当前正在处理的组内字节的序号，范围是0-6
	unsigned char nLeft;    // 上一字节残余的数据

	// 计数值初始化
	nSrc = 0;
	nDst = 0;

	// 组内字节序号和残余数据初始化
	nByte = 0;
	nLeft = 0;

	// 将源数据每7个字节分为一组，解压缩成8个字节
	// 循环该处理过程，直至源数据被处理完
	// 如果分组不到7字节，也能正确处理
	while(nSrc<nSrcLength)
	{
		// 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
		*pDst = ((*pSrc << nByte) | nLeft) & 0x7f;
		// 将该字节剩下的左边部分，作为残余数据保存起来
		nLeft = *pSrc >> (7-nByte);

		// 修改目标串的指针和计数值
		pDst++;
		nDst++;

		// 修改字节计数值
		nByte++;

		// 到了一组的最后一个字节
		if(nByte == 7)
		{
			// 额外得到一个目标解码字节
			*pDst = nLeft;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++;

			// 组内字节序号和残余数据初始化
			nByte = 0;
			nLeft = 0;
		}

		// 修改源串的指针和计数值
		pSrc++;
		nSrc++;
	}

	*pDst = 0;

	// 返回目标串长度
	return nDst;
}

//   8bit编码
//   输入:   pSrc   -   源字符串指针
//               nSrcLength   -   源字符串长度
//   输出:   pDst   -   目标编码串指针
//   返回:   目标编码串长度
int gsmEncode8bit(const char* pSrc,unsigned char* pDst,int nSrcLength)
{
	//   简单复制
	memcpy(pDst,   pSrc,   nSrcLength);
	return   nSrcLength;
}

//   8bit解码
//   输入:   pSrc   -   源编码串指针
//   nSrcLength   -     源编码串长度
//   输出:   pDst   -     目标字符串指针
//   返回:   目标字符串长度
int gsmDecode8bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	//   简单复制
	memcpy(pDst,   pSrc,   nSrcLength);
	//   输出字符串加个结束符
	*pDst   =   '\0';
	return   nSrcLength;
}

// UCS2编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
int gsmEncodeUcs2(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
#if 0
	int nDstLength;        // UNICODE宽字符数目
	WCHAR wchar[128];      // UNICODE串缓冲区

	// 字符串-->UNICODE串
	nDstLength = ::MultiByteToWideChar(CP_ACP, 0, pSrc, nSrcLength, wchar, 128);

	// 高低字节对调，输出
	for(int i=0; i<nDstLength; i++)
	{
		// 先输出高位字节
		*pDst++ = wchar[i] >> 8;
		// 后输出低位字节
		*pDst++ = wchar[i] & 0xff;
	}

	// 返回目标编码串长度
	return nDstLength * 2;
#else
	return 0;
#endif
}

// UCS2解码
// pSrc: 源编码串指针
// pDst: 目标字符串指针
// nSrcLength: 源编码串长度
// 返回: 目标字符串长度
int gsmDecodeUcs2(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
#if 0
	int nDstLength;        // UNICODE宽字符数目
	WCHAR wchar[128];      // UNICODE串缓冲区

	// 高低字节对调，拼成UNICODE
	for(int i=0; i<nSrcLength/2; i++)
	{
		// 先高位字节
		wchar[i] = *pSrc++ << 8;

		// 后低位字节
		wchar[i] |= *pSrc++;
	}

	// UNICODE串-->字符串
	nDstLength = ::WideCharToMultiByte(CP_ACP, 0, wchar, nSrcLength/2, pDst, 160, NULL, NULL);

	// 输出字符串加个结束符    
	pDst[nDstLength] = '\0';    

	// 返回目标字符串长度
	return nDstLength;
#else
	return 0;
#endif
}

// 可打印字符串转换为字节数据
// 如："C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// pSrc: 源字符串指针
// pDst: 目标数据指针
// nSrcLength: 源字符串长度
// 返回: 目标数据长度
int gsmString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int i;

	for(i=0; i<nSrcLength; i+=2)
	{
		// 输出高4位
		if(*pSrc>='0' && *pSrc<='9')
		{
			*pDst = (*pSrc - '0') << 4;
		}
		else
		{
			*pDst = (*pSrc - 'A' + 10) << 4;
		}

		pSrc++;

		// 输出低4位
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

	// 返回目标数据长度
	return nSrcLength / 2;
}

// 字节数据转换为可打印字符串
// 如：{0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01} --> "C8329BFD0E01" 
// pSrc: 源数据指针
// pDst: 目标字符串指针
// nSrcLength: 源数据长度
// 返回: 目标字符串长度
int gsmBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	const char tab[]="0123456789ABCDEF";    // 0x0-0xf的字符查找表
	int i;

	for(i=0; i<nSrcLength; i++)
	{
		// 输出低4位
		*pDst++ = tab[*pSrc >> 4];

		// 输出高4位
		*pDst++ = tab[*pSrc & 0x0f];

		pSrc++;
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nSrcLength * 2;
}

// 正常顺序的字符串转换为两两颠倒的字符串，若长度为奇数，补'F'凑成偶数
// 如："8613851872468" --> "683158812764F8"
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gsmInvertNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;   // 目标字符串长度
	char ch;          // 用于保存一个字符
	int i;

	// 复制串长度
	nDstLength = nSrcLength;

	// 两两颠倒
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;        // 保存先出现的字符
		*pDst++ = *pSrc++;   // 复制后出现的字符
		*pDst++ = ch;        // 复制先出现的字符
	}

	// 源串长度是奇数吗？
	if(nSrcLength & 1)
	{
		*(pDst-2) = 'F';     // 补'F'
		nDstLength++;        // 目标串长度加1
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nDstLength;
}

// 两两颠倒的字符串转换为正常顺序的字符串
// 如："683158812764F8" --> "8613851872468"
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gsmSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;   // 目标字符串长度
	char ch;          // 用于保存一个字符
	int i;

	// 复制串长度
	nDstLength = nSrcLength;

	// 两两颠倒
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;        // 保存先出现的字符
		*pDst++ = *pSrc++;   // 复制后出现的字符
		*pDst++ = ch;        // 复制先出现的字符
	}

	// 最后的字符是'F'吗？
	if(*(pDst-1) == 'F')
	{
		pDst--;
		nDstLength--;        // 目标字符串长度减1
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nDstLength;
}

// PDU编码，用于编制、发送短消息
// pSrc: 源PDU参数指针
// pDst: 目标PDU串指针
// 返回: 目标PDU串长度
int gsmEncodePdu(const PDU_PARAM* pSrc, char* pDst)
{
	int nLength;             // 内部用的串长度
	int nDstLength;          // 目标PDU串长度
	unsigned char buf[256];  // 内部用的缓冲区

	// SMSC地址信息段
	nLength = strlen(pSrc->SCA);    // SMSC地址字符串的长度    
	buf[0] = (char)((nLength & 1) == 0 ? nLength : nLength + 1) / 2 + 1;    // SMSC地址信息长度
	buf[1] = 0x91;        // 固定: 用国际格式号码
	nDstLength = gsmBytes2String(buf, pDst, 2);        // 转换2个字节到目标PDU串
	nDstLength += gsmInvertNumbers(pSrc->SCA, &pDst[nDstLength], nLength);    // 转换SMSC到目标PDU串

	// TPDU段基本参数、目标地址等
	nLength = strlen(pSrc->TPA);    // TP-DA地址字符串的长度
	buf[0] = 0x11;            // 是发送短信(TP-MTI=01)，TP-VP用相对格式(TP-VPF=10)
	buf[1] = 0;               // TP-MR=0
	buf[2] = (char)nLength;   // 目标地址数字个数(TP-DA地址字符串真实长度)
	buf[3] = 0x91;            // 固定: 用国际格式号码
	nDstLength += gsmBytes2String(buf, &pDst[nDstLength], 4);  // 转换4个字节到目标PDU串
	nDstLength += gsmInvertNumbers(pSrc->TPA, &pDst[nDstLength], nLength); // 转换TP-DA到目标PDU串

	// TPDU段协议标识、编码方式、用户信息等
	nLength = strlen(pSrc->TP_UD);    // 用户信息字符串的长度
	buf[0] = pSrc->TP_PID;        // 协议标识(TP-PID)
	buf[1] = pSrc->TP_DCS;        // 用户信息编码方式(TP-DCS)
	buf[2] = 0;            // 有效期(TP-VP)为5分钟
	if(pSrc->TP_DCS == GSM_7BIT)    
	{
		// 7-bit编码方式
		buf[3] = nLength;            // 编码前长度
		nLength = gsmEncode7bit(pSrc->TP_UD, &buf[4], nLength+1) + 4;    // 转换TP-DA到目标PDU串
	}
	else if(pSrc->TP_DCS == GSM_UCS2)
	{
		// UCS2编码方式
		buf[3] = gsmEncodeUcs2(pSrc->TP_UD, &buf[4], nLength);    // 转换TP-DA到目标PDU串
		nLength = buf[3] + 4;        // nLength等于该段数据长度
	}
	else
	{
		// 8-bit编码方式
		buf[3] = gsmEncode8bit(pSrc->TP_UD, &buf[4], nLength);    // 转换TP-DA到目标PDU串
		nLength = buf[3] + 4;        // nLength等于该段数据长度
	}
	nDstLength += gsmBytes2String(buf, &pDst[nDstLength], nLength);        // 转换该段数据到目标PDU串

	// 返回目标字符串长度
	return nDstLength;
}

// PDU解码，用于接收、阅读短消息
// pSrc: 源PDU串指针
// pDst: 目标PDU参数指针
// 返回: 用户信息串长度
int gsmDecodePdu(const char* pSrc, PDU_PARAM* pDst)
{
	int nDstLength;          // 目标PDU串长度
	unsigned char tmp;       // 内部用的临时字节变量
	unsigned char buf[256];  // 内部用的缓冲区

	// SMSC地址信息段
	gsmString2Bytes(pSrc, &tmp, 2);    // 取长度
	tmp = (tmp - 1) * 2;    // SMSC号码串长度
	pSrc += 4;              // 指针后移
	gsmSerializeNumbers(pSrc, pDst->SCA, tmp);    // 转换SMSC号码到目标PDU串
	pSrc += tmp;        // 指针后移

	// TPDU段基本参数、回复地址等
	gsmString2Bytes(pSrc, &tmp, 2);    // 取基本参数
	pSrc += 2;        // 指针后移
		
	// 包含回复地址，取回复地址信息
	gsmString2Bytes(pSrc, &tmp, 2);    // 取长度
	if(tmp & 1) tmp += 1;    // 调整奇偶性
	pSrc += 4;          // 指针后移
	gsmSerializeNumbers(pSrc, pDst->TPA, tmp);    // 取TP-RA号码
	pSrc += tmp;        // 指针后移

	// TPDU段协议标识、编码方式、用户信息等
	gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_PID, 2);    // 取协议标识(TP-PID)
	pSrc += 2;        // 指针后移
	gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_DCS, 2);    // 取编码方式(TP-DCS)
	pSrc += 2;        // 指针后移
	gsmSerializeNumbers(pSrc, pDst->TP_SCTS, 14);        // 服务时间戳字符串(TP_SCTS) 
	pSrc += 14;       // 指针后移
	gsmString2Bytes(pSrc, &tmp, 2);    // 用户信息长度(TP-UDL)
	pSrc += 2;        // 指针后移
	if(pDst->TP_DCS == GSM_7BIT)    
	{
		// 7-bit解码
		nDstLength = gsmString2Bytes(pSrc, buf, tmp & 7 ? (int)tmp * 7 / 4 + 2 : (int)tmp * 7 / 4);  // 格式转换
		gsmDecode7bit(buf, pDst->TP_UD, nDstLength);    // 转换到TP-DU
		nDstLength = tmp;
	}
	else if(pDst->TP_DCS == GSM_UCS2)
	{
		// UCS2解码
		nDstLength = gsmString2Bytes(pSrc, buf, tmp * 2);        // 格式转换
		nDstLength = gsmDecodeUcs2(buf, pDst->TP_UD, nDstLength);    // 转换到TP-DU
	}
	else
	{
		// 8-bit解码
		nDstLength = gsmString2Bytes(pSrc, buf, tmp * 2);        // 格式转换
		nDstLength = gsmDecode8bit(buf, pDst->TP_UD, nDstLength);    // 转换到TP-DU
	}

	// 返回目标字符串长度
	return nDstLength;
}

#if 0
// 发送短消息
// pSrc: 源PDU参数指针
BOOL gsmSendMessage(const PDU_PARAM* pSrc)
{
	int nPduLength;        // PDU串长度
	unsigned char nSmscLength;    // SMSC串长度
	int nLength;           // 串口收到的数据长度
	char cmd[16];          // 命令串
	char pdu[512];         // PDU串
	char ans[128];         // 应答串

	nPduLength = gsmEncodePdu(pSrc, pdu);    // 根据PDU参数，编码PDU串
	strcat(pdu, "\x01a");        // 以Ctrl-Z结束

	gsmString2Bytes(pdu, &nSmscLength, 2);    // 取PDU串中的SMSC信息长度
	nSmscLength++;        // 加上长度字节本身

	// 命令中的长度，不包括SMSC信息长度，以数据字节计
	sprintf(cmd, "AT+CMGS=%d\r", nPduLength / 2 - nSmscLength);    // 生成命令

	WriteComm2(cmd, strlen(cmd));    // 先输出命令串
	Sleep(1000);
	nLength = ReadComm2(ans, 128);   // 读应答数据

	// 根据能否找到"\r\n> "决定成功与否
	if(nLength != 0 && strstr(ans, ">") != NULL)
	{
		WriteComm2(pdu, strlen(pdu));        // 得到肯定回答，继续输出PDU串
		Sleep(1000);
		char ans1[128];
		nLength = ReadComm2(ans1, 128);       // 读应答数据

		// 根据能否找到"+CMS ERROR"决定成功与否
		if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// 读取短消息
// 用+CMGL代替+CMGR，可一次性读出全部短消息
// pMsg: 短消息缓冲区，必须足够大
// 返回: 短消息条数
int gsmReadMessage(PDU_PARAM* pMsg)
{
	int nLength;        // 串口收到的数据长度
	int nMsg;           // 短消息计数值
	char* ptr;          // 内部用的数据指针
	char cmd[16];       // 命令串
	char ans[1024];     // 应答串

	nMsg = 0;
	ptr = ans;

	sprintf(cmd, "AT+CMGL\r");    // 生成命令

	WriteComm1(cmd, strlen(cmd));    // 输出命令串
	Sleep(10000);
	nLength = ReadComm1(ans, 1024);    // 读应答数据
	ans[nLength]='\0';
	printf("ans: %s\n", ans);
	// 根据能否找到"+CMS ERROR"决定成功与否
	if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
	{
		// 循环读取每一条短消息, 以"+CMGL:"开头
		while((ptr = strstr(ptr, "+CMGL:")) != NULL)
		{
			ptr += 6;        // 跳过"+CMGL:"
			sscanf(ptr, "%d", &pMsg->index);    // 读取序号
//			TRACE("  index=%d\n",pMsg->index);

			ptr = strstr(ptr, "\r\n");    // 找下一行
			ptr += 2;        // 跳过"\r\n"

			gsmDecodePdu(ptr, pMsg);    // PDU串解码
			pMsg++;        // 准备读下一条短消息
			nMsg++;        // 短消息计数加1
		}
	}

	return nMsg;
}

// 删除短消息
// index: 短消息序号，从1开始
BOOL gsmDeleteMessage(const int index)
{
	int nLength;          // 串口收到的数据长度
	char cmd[16];         // 命令串
	char ans[128];        // 应答串

	sprintf(cmd, "AT+CMGD=%d\r", index);    // 生成命令

	// 输出命令串
	WriteComm1(cmd, strlen(cmd));

	// 读应答数据
	nLength = ReadComm1(ans, 128);

	// 根据能否找到"+CMS ERROR"决定成功与否
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
	sprintf(cmd, "AT+CSCA?\r");    // 生成命令
	WriteComm1(cmd, strlen(cmd));
	nLength = ReadComm1(ans, 128);   // 读应答数据
	return FALSE;
}

BOOL gsmGetMessage(PDU_PARAM* pMsg, int index){
	int nLength;        // 串口收到的数据长度
	char* ptr;          // 内部用的数据指针
	char cmd[16];       // 命令串
	char ans[1024];     // 应答串

	ptr = ans;

	sprintf(cmd, "AT+CMGR=%d\r", index);    // 生成命令

	WriteComm1(cmd, strlen(cmd));    // 输出命令串
	Sleep(500);
	nLength = ReadComm1(ans, 1024);    // 读应答数据
	ans[nLength]='\0';

	// 根据能否找到"+CMS ERROR"决定成功与否
	if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
	{
		ptr = strstr(ptr, "+CMGR:");
//		printf("%s\n", ptr);
		ptr += 6;        // 跳过"+CMGL:"

		char tmp = *(ptr+1);
		if (tmp == '1')
			return false;

		sscanf(ptr, "%d", &pMsg->index);    // 读取序号
//			TRACE("  index=%d\n",pMsg->index);

		ptr = strstr(ptr, "\r\n");    // 找下一行
		ptr += 2;        // 跳过"\r\n"

		gsmDecodePdu(ptr, pMsg);    // PDU串解码
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
		printf("短信发送成功！\n");
		printf("-------------------------------------------------\n");
	}
	else{
		printf("短信发送失败！\n");
		printf("-------------------------------------------------\n");
	}
}
#endif
