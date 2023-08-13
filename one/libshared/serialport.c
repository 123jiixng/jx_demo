/*
 * $Id$ --
 *
 *   Serial port routines
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/04/2010
 * Author: Jianliang Zhang
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>

#ifndef WIN32
#include <termios.h>
#endif

#include "shared.h"
#include "ih_serial.h"

////////////////////////////////////////////////////////////////////////////////////////////
//		serial device IO
////////////////////////////////////////////////////////////////////////////////////////////
long serial_get_baudrate(long baudRate)
{
    long BaudR = 0;

#ifndef WIN32
    switch(baudRate)
    {
	case 4000000:
		BaudR=B4000000;
		break;
	case 460800:
		BaudR=B460800;
		break;
	case 230400:
		BaudR=B230400;
		break;
	case 115200:
		BaudR=B115200;
		break;
	case 57600:
		BaudR=B57600;
		break;
	case 38400:
		BaudR=B38400;
		break;
	case 19200:
		BaudR=B19200;
		break;
	case 9600:
		BaudR=B9600;
		break;
	case 4800:
		BaudR=B4800;
		break;
	case 2400:
		BaudR=B2400;
		break;
	case 1200:
		BaudR=B1200;
		break;
	case 600:
		BaudR=B600;
		break;
	case 300:
		BaudR=B300;
		break;
	case 110:
		BaudR=B110;
		break;

	default:
		BaudR=B19200;
    }
#endif
    return BaudR;
}

/*** at brief  设置串口通信速率
 *@param  fd     类型 int  打开串口的文件句柄
 *@param  speed  类型 int  串口速度
 *@return  void*/
int serial_set_speed(int fd, int speed)
{
#ifndef WIN32
	int   status;
	struct termios   Opt;

	tcgetattr(fd, &Opt);

	 cfsetispeed(&Opt, serial_get_baudrate(speed));
	 cfsetospeed(&Opt, serial_get_baudrate(speed));
	 status = tcsetattr(fd, TCSANOW, &Opt);

	 if  (status != 0){
		printf("set serial baudrate failed! error=(%d,%s)\n",
			errno, strerror(errno));
		 return -1;
	 }

	tcflush(fd,TCIOFLUSH);

#endif//WIN32

	return 0;
}

/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型  int  打开的串口文件句柄*
*@param  databits 类型  int 数据位   取值 为 7 或者8*
*@param  stopbits 类型  int 停止位   取值为 1 或者2*
*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*@param  ctsrts  类型  int  硬件流控 取值为1或0
*@param  xonoff  类型  int  软件流控 取值为1或0
*/
int serial_set_parity(int fd,int databits,int stopbits,int parity, int crtscts, int xonoff)
{
#ifndef WIN32
	struct termios options;

	if( tcgetattr( fd,&options)  !=  0) {
		printf("set serial parity failed! Cannot get attribute! error=(%d,%s)\n",
			errno, strerror(errno));
		return -1;
	}

//	printf("old term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	//cfmakeraw(&options);
    options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                         |INLCR|IGNCR|ICRNL|IXON);
	if(xonoff) options.c_iflag |= IXON;
    options.c_oflag = OPOST;
    options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    options.c_cflag &= ~(CSIZE|PARENB);
    //options.c_cflag |= CS8;

	options.c_cflag &= ~CSIZE;

//	printf("new 1 term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	switch (databits) /*设置数据位数*/
	{
	case 5:
		options.c_cflag |= CS5;
		break;
	case 6:
		options.c_cflag |= CS6;
		break;
	case 7:
		options.c_cflag |= CS7;
		//options.c_cflag &= ~CS8;
		break;
	case 8:
		options.c_cflag |= CS8;
		//options.c_cflag &= ~CS7;
		break;
	default:
		printf("Bad data bit : %d\n", databits);
//		return -2;
		options.c_cflag |= CS8;
	}

//	printf("new 2 term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	switch (parity)
	{
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* disable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/
		options.c_iflag |= INPCK;             /* Enable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
		options.c_iflag |= INPCK;       /* Enable parity checking */
		break;
	case 'S':
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;   //FIXME: why clear stop bit?
		options.c_iflag &= ~INPCK;     /* disable parity checking */
	break;
	default:
		printf("Bad parity : %c\n", parity);
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* disable parity checking */
//		return -3;
	}

//	printf("new 3 term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	/* 设置停止位*/
	switch (stopbits)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		printf("Bad stop bit : %c\n", stopbits);
//		return -4;
		options.c_cflag &= ~CSTOPB;
	}

//	printf("new 4 term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	options.c_cc[VTIME] = 0; // 15 seconds
	options.c_cc[VMIN] = 1;

//	printf("new 5 term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	if(crtscts) options.c_cflag |= CRTSCTS; //enable hw flow control
	else options.c_cflag &= ~CRTSCTS; //disable hw flow control

	options.c_cflag |= CLOCAL;

//	printf("new 6 term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */

//	printf("new term io: iflag: 0x%x, oflag: 0x%x, cflag: 0x%x, lfag: 0x%x\n",
//		options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag);

	if (tcsetattr(fd,TCSANOW,&options) != 0){
		printf("set serial parity failed! Cannot set attribute! error=(%d,%s)\n",
			errno, strerror(errno));
		return -5;
	}

#endif//WIN32

	return 0;
}

/**
 *@brief   设置串口参数（波特率，数据位，停止位和效验位）
 *@param  dev     类型  char*  设备文件名
 *@param  config  类型  char*  波特率 数据位-校验位-停止位
 *@param  crtcrts  类型  int  硬件流控 取值为1或0
 *@param  xonoff  类型  int  软件流控 取值为1或0
 */
int serial_config(char* dev, char *config, int crtscts, int xonoff)
{
	int fd;
	int baud = 19200;
	int data = 8;
	int parity = 'N';
	int stop = 1;
	char *p;

	if(dev[0]=='/'){
		fd = open(dev, O_RDWR);
	}else{
		char fname[64];

		snprintf(fname, sizeof(fname), "/dev/%s", dev);

		fd = open(fname, O_RDWR);
	}

	if(fd==-1) return -1;

	baud = atoi(config);
	p = strchr(config, ' ');
	if(p){
		p++;
		data = *p - '0';
		p++;
		if(*p){
			parity = *p;
			p++;
			if(*p){
				stop = *p - '0';
			}
		}
	}

	serial_set_speed(fd, baud);
	serial_set_parity(fd, data, stop, parity, crtscts, xonoff);

	close(fd);

	return 0;
}

SERIAL_SPEED serial_speed[]={{460800, B460800},{230400, B230400},{115200, B115200},{57600, B57600}, {38400, B38400},{19200, B19200},{9600, B9600},{4800, B4800},{2400, B2400},{1200, B1200},{600, B600},{300, B300},{110, B110}};

int set_serial_port_speed(int fd, int speed)
{
	int i;
	struct termios newio;

	if(tcgetattr(fd, &newio)!=0){
		syslog(LOG_ERR,"serial speed: get attr err");
		return -1;
	}

	for(i = 0; i<sizeof(serial_speed)/sizeof(SERIAL_SPEED); i++){
		if(speed == serial_speed[i].speed){
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&newio, serial_speed[i].bspeed);
			cfsetospeed(&newio, serial_speed[i].bspeed);
			if(tcsetattr(fd, TCSANOW, &newio) ==-1){
				syslog(LOG_ERR,"serial port set speed err");
				return -1;
			}
			tcflush(fd, TCIOFLUSH);
		}
	}

	return 0;
}

int set_serial_port_parity(int fd, int parity)
{
	struct termios newio;

	if(tcgetattr(fd, &newio)!=0){
		syslog(LOG_ERR,"serial parity: get attr err");
		return -1;
	}

	switch(parity){
		case SERIAL_PARITY_SPACE:
		case SERIAL_PARITY_NONE:{
			newio.c_cflag &= ~PARENB;
			newio.c_iflag &= ~INPCK;
			break;
		}
		case SERIAL_PARITY_ODD:{
			newio.c_cflag |= PARENB;
			//newio.c_iflag |= (INPCK|ISTRIP);
			newio.c_iflag |= INPCK;
			newio.c_cflag |= PARODD;
			break;
		}
		case SERIAL_PARITY_EVEN:{
			newio.c_cflag |= PARENB;
			//newio.c_iflag |= (INPCK|ISTRIP);
			newio.c_iflag |= INPCK;
			newio.c_cflag &= ~PARODD;
			break;
		}
		default:{
			syslog(LOG_WARNING,"serial port parity bits is out of scope");
			return 0;
		}
	} 

	tcflush(fd, TCIFLUSH);

	if(tcsetattr(fd, TCSANOW, &newio) ==-1){
		syslog(LOG_ERR,"serial port set parity err");
		return -1;
	}

	return 0;
}

int set_serial_port_data(int fd, int data)
{
	struct termios newio;

	if(tcgetattr(fd, &newio)!=0){
		syslog(LOG_ERR,"serial data: get attr err");
		return -1;
	}

	newio.c_cflag &= ~CSIZE;
	switch(data){
		case SERIAL_DATA_FIVE:{
			newio.c_cflag |= CS5;			
			break;
		}
		case SERIAL_DATA_SIX:{
			newio.c_cflag |= CS6;			
			break;
		}
		case SERIAL_DATA_SEVEN:{
			newio.c_cflag |= CS7;			
			break;
		}
		case SERIAL_DATA_EIGHT:{
			newio.c_cflag |= CS8;			
			break;
		}
		default:{
			syslog(LOG_WARNING,"serial port data bits is out of scope");
			return 0;
		}
	}
	tcflush(fd, TCIFLUSH);

	if(tcsetattr(fd, TCSANOW, &newio) ==-1){
		syslog(LOG_ERR,"serial port set data err");
		return -1;
	}

	return 0;
}

int set_serial_port_stop(int fd, int stop)
{
	struct termios newio;

	if(tcgetattr(fd, &newio)!=0){
		syslog(LOG_ERR,"serial stop: get attr err");
		return -1;
	}

	switch(stop){
		case SERIAL_STOP_ONE:{
			newio.c_cflag &= ~CSTOPB;
			break;
		}
		case SERIAL_STOP_TWO:{
			newio.c_cflag |= CSTOPB;
			break;
		}
		default:{
			syslog(LOG_WARNING,"serial port stop bits is out of scope");
			return 0;
		}
	}
	tcflush(fd, TCIFLUSH);

	if(tcsetattr(fd, TCSANOW, &newio) ==-1){
		syslog(LOG_ERR,"serial port set stop err");
		return -1;
	}

	return 0;
}

int set_serial_port_xonxoff(int fd, int xonxoff)
{
	struct termios newio;

	if(tcgetattr(fd, &newio)!=0){
		syslog(LOG_ERR,"serial xonxoff: get attr err");
		return -1;
	}

	switch(xonxoff){
		case SERIAL_XONXOFF_ENABLE:{
			newio.c_iflag |= IXON;
			newio.c_iflag |= IXOFF;
			break;
		}
		case SERIAL_XONXOFF_DISABLE:{
			newio.c_iflag &= ~IXON;
			newio.c_iflag &= ~IXOFF;
			break;
		}
		default:{
			syslog(LOG_WARNING,"serial port xonxoff is out of scope");
			return 0;
		}
	}
	tcflush(fd, TCIFLUSH);

	if(tcsetattr(fd, TCSANOW, &newio) ==-1){
		syslog(LOG_ERR,"serial port set xonxoff err");
		return -1;
	}

	return 0;
}

int set_serial_port_rtscts(int fd, int rtscts)
{
	struct termios newio;

	if(tcgetattr(fd, &newio)!=0){
		syslog(LOG_ERR,"serial rtscts: get attr err");
		return -1;
	}

	syslog(LOG_ERR,"serial.rtscts set to %d", rtscts);
	switch(rtscts){
		case 1:{
			newio.c_cflag |= CRTSCTS;
			break;
		}
		case 0:{
			newio.c_cflag &= ~CRTSCTS;
			break;
		}
		default:{
			syslog(LOG_WARNING,"serial port rtscts is out of scope");
			return 0;
		}
	}
	tcflush(fd, TCIFLUSH);

	if(tcsetattr(fd, TCSANOW, &newio) ==-1){
		syslog(LOG_ERR,"serial port set rtscts err:%d(%s)", errno, strerror(errno));
		return -1;
	}

	return 0;
}

void close_serial_port(int *fd)
{
	if(*fd >0){
		if(close(*fd) == -1){
			syslog(LOG_ERR,"close serial port err");
		}else {
			*fd = -1;
		}
	}
}


int get_serial_port_speed(int fd)
{
	int i;
	struct termios newio;
	speed_t i_speed;

	if(tcgetattr(fd, &newio)!=0){
		return -1;
	}

	i_speed = cfgetispeed(&newio);
	for(i = 0; i<sizeof(serial_speed)/sizeof(SERIAL_SPEED); i++){
		if(i_speed == serial_speed[i].bspeed){
			return serial_speed[i].speed;
		}
	}

	return 0;
}

