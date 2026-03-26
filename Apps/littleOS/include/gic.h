#ifndef _GIC_H
#define _GIC_H

#include <stdint.h>

void gic_init(void);
uint32_t gic_acknowledge_interrupt(void);
void gic_end_of_interrupt(uint32_t iar);

#endif