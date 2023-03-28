//
// Created by Joerg on 30.09.2022.
//

#ifndef MULTIREWARDER_CONTROLLERESPNOW_H
#define MULTIREWARDER_CONTROLLERESPNOW_H
#include "message.h"
void InitESPNow();
void SearchBTHubs();
int getNumberBTHubs();
void sendToBTHubs(message currentStatus);

#endif //MULTIREWARDER_CONTROLLERESPNOW_H
