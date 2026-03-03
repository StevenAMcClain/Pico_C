/*
 * ComPort.h
 *
 *  Created on: Oct. 9, 2025
 *      Author: Steven
 */

#ifndef COMPORT_H_
#define COMPORT_H_

//#define INCLUDE_SLAVE
#define INCLUDE_MASTER

#include <stdint.h>

#ifdef INCLUDE_SLAVE
extern void SSI_Init_Slave(void);
extern uint32_t SPI_Slave_Receive(void);
#endif

#ifdef INCLUDE_MASTER
extern void SSI_Init_Master(void);
extern void SSI_sendData(uint32_t val);
#endif

#endif /* COMPORT_H_ */
