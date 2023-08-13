//add for InDTU900 serial config tool
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include "serialconf.h"

uns16 crc16(uns8 *data, uns16 data_len)
{
	unsigned char uchCRCHi = 0xFF ; /* CRC 的高字节初始化 */
	unsigned char uchCRCLo = 0xFF ; /* CRC 的低字节初始化 */
	unsigned uIndex ; /* CRC 查询表索引 */

	while (data_len--) /* 完成整个报文缓冲区 */
	{
		uIndex = uchCRCLo^(*data++); /* 计算 CRC */
		uchCRCLo = uchCRCHi ^auchCRCHi[uIndex] ;
		uchCRCHi = auchCRCLo[uIndex] ;
	}

	return (uchCRCHi << 8 | uchCRCLo) ;
}

/*
* Calculate a new fcs given the current fcs and the new data.
*/
uns16 pppfcs16(uns16 fcs, uns8 *cp, uns16 len)
{
       while (len--){
           fcs = (fcs >> 8) ^ fcsTab[(fcs ^ *cp++) & 0xff];
       }

       return (fcs);
}

/*
* calculate the crc
*/
uns16 calculate_crc(uns16 fcs, uns8 *cp,uns16 len)
{
	uns16 temp_crc = 0;

	temp_crc = pppfcs16(fcs, cp, len);
	temp_crc ^=0xffff;

	return temp_crc;
}

uns16 add_crc(uns8 *cp, uns16 len)
{
	return htons(calculate_crc(0xFFFF, cp, len));
}

/*传入的包必须是网络字节序,返回的crc也是网络字节序*/
uns16 cal_crc16_pkt(CONFIG_PKT *pkt)
{
	uns16 crc = 0;

	crc = pppfcs16(0xFFFF, (uns8 *)&(pkt->header),sizeof(pkt->header));

	crc = pppfcs16(crc, (uns8 *)&(pkt->len),sizeof(pkt->len));
	
	crc = pppfcs16(crc, pkt->body,ntohs(pkt->len));
	crc ^= 0xFFFF; 
	//crc = htons(crc);
	return crc;
}

int get_config_pkt(uns8 *buf, int buf_len, uns8 *dest_buf, int dst_buf_size, int *dst_buf_len)
{
	CONFIG_PKT pkt;
	uns16 tmp_crc = 0;
	uns16 i = 0;
	uns16 conf_offset = 0;
	uns16 conf_len = 0;
	uns8 *conf_buf = NULL;
	
	memset(&pkt,0,sizeof(CONFIG_PKT));

	if(buf_len < 9){// 55AA55AA(4B) cmd(1B) len(2B) ... CRC(2B) 
		//syslog(LOG_DEBUG, "buffer length too short");

		return PKT_INVALID;
	}

	for(i = 0; i <= (buf_len-9); i++){
		pkt.header 	= *(uns32*)(buf+i);
		pkt.header	= ntohl(pkt.header);			
		if(pkt.header == 0x55AA55AA){
			conf_offset = i;
			break;
		}
	}

	if(i > (buf_len-9)){
		//syslog(LOG_WARNING, "can not match 0x55AA55AA");

		return PKT_INVALID;
	}

	if(buf_len-conf_offset < 9){// xxxx 55AA55AA(4B) lack cmd(1B) len(2B) ... CRC(2B)
		memcpy(dest_buf, buf + conf_offset, buf_len-conf_offset);
		*dst_buf_len = buf_len-conf_offset;

		return PKT_CONTINUE;
	}
	
	conf_buf = buf + conf_offset;

	pkt.len = (conf_buf[5]<<8) + conf_buf[6];
	if(pkt.len > (buf_len-conf_offset-9)) {// xxxx 55AA55AA(4B) cmd(1B) len(2B) ...lack tlv(xB) CRC(2B)
		memcpy(dest_buf, buf + conf_offset, buf_len-conf_offset);
		*dst_buf_len = buf_len-conf_offset;

		return PKT_CONTINUE;
	}

	if(pkt.len > (PKT_MAX_SIZE - 9)){
		syslog(LOG_WARNING, "config pkt len too large");

		return PKT_INVALID;
	}

	//receive a full pkt
	conf_len = pkt.len + 9;

	pkt.crc = (conf_buf[conf_len - 2]<<8) + conf_buf[conf_len - 1];

	tmp_crc = add_crc(conf_buf, conf_len - 2);
	if(pkt.crc != tmp_crc){
		syslog(LOG_WARNING, "config pkt CRC false(rcv: 0x%x|cal: 0x%x)", pkt.crc, tmp_crc);

		return PKT_INVALID;
	}

 	if(dst_buf_size >= conf_len){
		memcpy(dest_buf, conf_buf, conf_len);
		*dst_buf_len = conf_len;
 	}

	syslog(LOG_DEBUG, "len:%d,conf_len:%d,conf_offset:%d",buf_len,conf_len,conf_offset);
	
	return PKT_FULL;
}

int config_ack_write(int fd, uns8 *buf, int buf_len)
{
	int ret = -1;
	int sent = 0, sum = 0;

	if(fd <= 0) {
		return -1;
	}

	if(!buf) {
		return -1;
	}

	sent = buf_len;
	while(sent > 0) {
		ret = write(fd, buf + sum, sent);
		usleep(30*1000);
		if ((ret == 0) && (errno != EINTR)) {
			break;
		}

		if (ret < 0) {
			break;
		}

		sum += ret;
		sent -= ret;
	}

	return sum;
}
