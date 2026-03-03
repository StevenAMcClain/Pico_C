/*
 * controlbox.h
 *
 *  Created on: Oct. 9, 2025
 *      Author: Steven
 */

#ifndef CONTROLBOX_H_
#define CONTROLBOX_H_

typedef enum
{
    CB_TYPE_ENC = BIT(7),
    CB_TYPE_POT = BIT(6),
    CB_TYPE_PB  = BIT(5),

} CB_TYPE;

#define CB_VALUE_MASK 0x00FFFFFF
#define CB_DEVICE_MASK 0x0F
#define CB_TYPE_MASK 0xF0
#define CB_TYPEDEV_SHIFT 24

typedef enum
{
    CB_PB_LPB = 1,
    CB_PB_RPB,
    CB_PB_LENC,
    CB_PB_RENC,
    CB_PB_SW

} CB_PB;


#define CB_PB_PRESSED  1
#define CB_PB_RELEASED 2

extern void CB_Post_Event(CB_TYPE /*type*/, int /*device*/, int /*value*/);

#endif /* CONTROLBOX_H_ */
