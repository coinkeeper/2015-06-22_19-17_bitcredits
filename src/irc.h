// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcredit developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCREDIT_IRC_H
#define BITCREDIT_IRC_H

#include "allocators.h" /* for SecureString */
#include "addrman.h"

void ThreadIRCSeed(void* parg);
bool Send(std::string text);

extern int nGotIRCAddresses;

#endif
