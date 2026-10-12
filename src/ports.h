#ifndef PORTS_H
#define PORTS_H

#include "argparser.h"

void initPorts(Settings s);

void portIO(unsigned int portId, unsigned int write);

#endif /*PORTS_H*/
