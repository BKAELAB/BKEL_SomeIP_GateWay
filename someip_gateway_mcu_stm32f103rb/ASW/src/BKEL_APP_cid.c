/*
 * BKEL_APP_cid.c
 *
 *  Created on: Jan 4, 2026
 *      Author: sjkang
 *
 *      CID[15:0] : UIDMIX[15:4]SEQNUM[3:0]
 */

#include "BKEL_APP_cid.h"

static uint8_t seq4 = 0;

static uint16_t make_node_id_12bit(void);
static uint8_t next_seq4(void);

uint16_t make_cid(void)
{
    static uint16_t node_id = 0;
    static uint8_t  init = 0;

    if (!init)
    {
        node_id = make_node_id_12bit();
        if (node_id == 0) node_id = 1;
        init = 1;
    }

    uint8_t seq = next_seq4();

    return (uint16_t)((node_id << 4) | seq);
}

static uint16_t make_node_id_12bit(void)
{
    uint32_t u0 = *(uint32_t *)(UID_BASE + 0);
    uint32_t u1 = *(uint32_t *)(UID_BASE + 4);
    uint32_t u2 = *(uint32_t *)(UID_BASE + 8);

    uint32_t mixed = u0 ^ u1 ^ u2;

    return (uint16_t)(mixed & 0x0FFF);  // 12bit
}

static uint8_t next_seq4(void)
{
    seq4 = (seq4 + 1) & 0x0F;  // 0~15
    return seq4;
}
