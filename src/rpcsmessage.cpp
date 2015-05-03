// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "rpcserver.h"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "smessage.h"
#include "init.h"
#include "util.h"

using namespace json_spirit;
using namespace std;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);


static inline std::string GetTimeString(int64_t time) {
    return DateTimeStrFormat("%Y-%m-%d %H:%M:%S", time);
}

Value smsgenable(const Array& params, bool fHelp) {

    if (fHelp || params.size() != 0)
        throw runtime_error(
            "smsgenable \n"
            "Enable secure messaging.");
    
    if (fSecMsgEnabled)
        throw runtime_error("Secure messaging is already enabled.");
    
    Object result;
    if (!SecureMsgEnable()) {
        result.push_back(Pair("result", "Failed to enable secure messaging."));
    }
    else {
        result.push_back(Pair("result", "Enabled secure messaging."));
    }
    return result;
}

Value smsgdisable(const Array& params, bool fHelp) {

    if (fHelp || params.size() != 0)
        throw runtime_error(
            "smsgdisable \n"
            "Disable secure messaging.");
    
    if (!fSecMsgEnabled)
        throw runtime_error("Secure messaging is already disabled.");
    
    Object result;
    if (!SecureMsgDisable()) {
        result.push_back(Pair("result", "Failed to disable secure messaging."));
    }
    else {
        result.push_back(Pair("result", "Disabled secure messaging."));
    }
    return result;
}

Value smsgoptions(const Array& params, bool fHelp) {

    if (fHelp || params.size() > 3)
        throw runtime_error(
            "smsgoptions [list|set <optname> <value>]\n"
            "List and manage options.");
    
    std::string mode = "list";
    if (params.size() > 0) {
        mode = params[0].get_str();
    }
    
    Object result;
    
    if (mode == "list") {
        result.push_back(Pair("option", std::string("newAddressRecv = ") + (smsgOptions.fNewAddressRecv ? "true" : "false")));
        result.push_back(Pair("option", std::string("newAddressAnon = ") + (smsgOptions.fNewAddressAnon ? "true" : "false")));
        
        result.push_back(Pair("result", "Success."));
    }
    else if (mode == "set") {
        if (params.size() < 3) {
            result.push_back(Pair("result", "Too few parameters."));
            result.push_back(Pair("expected", "set <optname> <value>"));
            return result;
        }
        
        std::string optname = params[1].get_str();
        bool is_bool = params[2].type() == bool_type;
		bool value = is_bool && params[2].get_bool();
        
        if (optname == "newAddressRecv") {
            if (is_bool) {
                smsgOptions.fNewAddressRecv = value;
            }
            else {
                result.push_back(Pair("result", "Unknown value."));
                return result;
            }
            result.push_back(Pair("set option", std::string("newAddressRecv = ") + (smsgOptions.fNewAddressRecv ? "true" : "false")));
        }
        else if (optname == "newAddressAnon") {
            if (is_bool) {
                smsgOptions.fNewAddressAnon = value;
            }
            else {
                result.push_back(Pair("result", "Unknown value."));
                return result;
            }
            result.push_back(Pair("set option", std::string("newAddressAnon = ") + (smsgOptions.fNewAddressAnon ? "true" : "false")));
        }
        else {
            result.push_back(Pair("result", "Option not found."));
            return result;
        }
    }
    else {
        result.push_back(Pair("result", "Unknown Mode."));
        result.push_back(Pair("expected", "smsgoption [list|set <optname> <value>]"));
    }
    return result;
}

Value smsglocalkeys(const Array& params, bool fHelp) {

    if (fHelp || params.size() > 3)
        throw runtime_error(
            "smsglocalkeys [whitelist|all|wallet|recv <+/-> <address>|anon <+/-> <address>]\n"
            "List and manage keys.");
    
    if (!fSecMsgEnabled)
        throw runtime_error("Secure messaging is disabled.");
    
    Object result;
    
    std::string mode = "whitelist";
    if (params.size() > 0) {
        mode = params[0].get_str();
    }
    
    char cbuf[256];
    
    if (mode == "whitelist" || mode == "all") {
        uint32_t nKeys = 0;
        int all = mode == "all" ? 1 : 0;
        
        for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it) {
            if (!all && !it->fReceiveEnabled)
                continue;
            
            CBitcreditAddress coinAddress(it->sAddress);
            if (!coinAddress.IsValid())
                continue;
            
            std::string sPublicKey;
            
            CKeyID keyID;
            if (!coinAddress.GetKeyID(keyID))
                continue;
            
            CPubKey pubKey;
            if (!pwalletMain->GetPubKey(keyID, pubKey))
                continue;
            if (!pubKey.IsValid() || !pubKey.IsCompressed()) {
                continue;
            };
            
            sPublicKey = EncodeBase58(&pubKey[0], &pubKey[pubKey.size()]);
            
            std::string sLabel = pwalletMain->mapAddressBook[keyID].name;
            std::string sInfo;
            if (all)
                sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on,  " : "off, ");
            sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
            result.push_back(Pair("key", it->sAddress + " - " + sPublicKey + " " + sInfo + " - " + sLabel));
            
            nKeys++;
        }
         
        snprintf(cbuf, sizeof(cbuf), "%u keys listed.", nKeys);
        result.push_back(Pair("result", std::string(cbuf))); 
    }
    else if (mode == "recv") {
        if (params.size() < 3) {
            result.push_back(Pair("result", "Too few parameters."));
            result.push_back(Pair("expected", "recv <+/-> <address>"));
            return result;
        }
        
        std::string op      = params[1].get_str();
        std::string addr    = params[2].get_str();
        
        std::vector<SecMsgAddress>::iterator it;
        for (it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it) {
            if (addr != it->sAddress)
                continue;
            break;
        }
        
        if (it == smsgAddresses.end()) {
            result.push_back(Pair("result", "Address not found."));
            return result;
        }
        
        if (op == "+" || op == "on"  || op == "add" || op == "a") {
            it->fReceiveEnabled = true;
        }
        else if (op == "-" || op == "off" || op == "rem" || op == "r") {
            it->fReceiveEnabled = false;
        }
        else {
            result.push_back(Pair("result", "Unknown operation."));
            return result;
        }
        
        std::string sInfo;
        sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on, " : "off,");
        sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
        result.push_back(Pair("result", "Success."));
        result.push_back(Pair("key", it->sAddress + " " + sInfo));
        return result;    
    }
    else if (mode == "anon") {
        if (params.size() < 3) {
            result.push_back(Pair("result", "Too few parameters."));
            result.push_back(Pair("expected", "anon <+/-> <address>"));
            return result;
        }
        std::string op      = params[1].get_str();
        std::string addr    = params[2].get_str();
        
        std::vector<SecMsgAddress>::iterator it;
        for (it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it) {
            if (addr != it->sAddress)
                continue;
            break;
        }
        
        if (it == smsgAddresses.end()) {
            result.push_back(Pair("result", "Address not found."));
            return result;
        }
        
        if (op == "+" || op == "on"  || op == "add" || op == "a") {
            it->fReceiveAnon = true;
        }
        else if (op == "-" || op == "off" || op == "rem" || op == "r") {
            it->fReceiveAnon = false;
        }
        else {
            result.push_back(Pair("result", "Unknown operation."));
            return result;
        }
        
        std::string sInfo;
        sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on, " : "off,");
        sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
        result.push_back(Pair("result", "Success."));
        result.push_back(Pair("key", it->sAddress + " " + sInfo));
        return result;   
    }
    else if (mode == "wallet") {
        uint32_t nKeys = 0;
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& entry, pwalletMain->mapAddressBook) {
            if (!IsMine(*pwalletMain, entry.first))
                continue;
            
            CBitcreditAddress coinAddress(entry.first);
            if (!coinAddress.IsValid())
                continue;
            
            std::string address;
            std::string sPublicKey;
            address = coinAddress.ToString();
            
            CKeyID keyID;
            if (!coinAddress.GetKeyID(keyID))
                continue;
            
            CPubKey pubKey;
            if (!pwalletMain->GetPubKey(keyID, pubKey))
                continue;
            if (!pubKey.IsValid()
                || !pubKey.IsCompressed()) {
                continue;
            }
            
            sPublicKey = EncodeBase58(&pubKey[0], &pubKey[pubKey.size()]);
            
            result.push_back(Pair("key", address + " - " + sPublicKey + " - " + entry.second.name));
            nKeys++;
        };
        
        snprintf(cbuf, sizeof(cbuf), "%u keys listed from wallet.", nKeys);
        result.push_back(Pair("result", std::string(cbuf)));
    }
    else {
        result.push_back(Pair("result", "Unknown Mode."));
        result.push_back(Pair("expected", "smsglocalkeys [whitelist|all|wallet|recv <+/-> <address>|anon <+/-> <address>]"));
    }
    
    return result;
}

Value smsgscanchain(const Array& params, bool fHelp) {

    if (fHelp || params.size() != 0)
        throw runtime_error(
            "smsgscanchain \n"
            "Look for public keys in the block chain.");
    
    if (!fSecMsgEnabled)
        throw runtime_error("Secure messaging is disabled.");
    
    Object result;
    if (!SecureMsgScanBlockChain()) {
        result.push_back(Pair("result", "Scan Chain Failed."));
    }
    else {
        result.push_back(Pair("result", "Scan Chain Completed."));
    }
    return result;
}

Value smsgscanbuckets(const Array& params, bool fHelp) {

    if (fHelp || params.size() != 0)
        throw runtime_error(
            "smsgscanbuckets \n"
            "Force rescan of all messages in the bucket store.");
    
    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Secure messaging is disabled.");
    
    Object result;
    std::string error;
    int ret = SecureMsgScanBuckets(error);
    if (ret) {
        throw JSONRPCError(ret, error);
    }
    return result;
}

Value smsgaddkey(const Array& params, bool fHelp) {

    if (fHelp || params.size() != 2)
        throw runtime_error(
            "smsgaddkey <address> <pubkey>\n"
            "Add address, pubkey pair to database.");
    
    if (!fSecMsgEnabled)
        throw runtime_error("Secure messaging is disabled.");
    
    std::string addr = params[0].get_str();
    std::string pubk = params[1].get_str();
    
    Object result;
    int rv = SecureMsgAddAddress(addr, pubk);
    if (rv != 0) {
        result.push_back(Pair("result", "Public key not added to db."));
        switch (rv) {
            case 2:     result.push_back(Pair("reason", "publicKey is invalid."));                  break;
            case 3:     result.push_back(Pair("reason", "publicKey does not match address."));      break;
            case 4:     result.push_back(Pair("reason", "address is already in db."));              break;
            case 5:     result.push_back(Pair("reason", "address is invalid."));                    break;
            default:    result.push_back(Pair("reason", "error."));                                 break;
        }
    }
    else {
        result.push_back(Pair("result", "Added public key to db."));
    }
    
    return result;
}

Value smsggetpubkey(const Array& params, bool fHelp) {

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "smsggetpubkey <address>\n"
            "Return the base58 encoded compressed public key for an address.\n"
            "Tests localkeys first, then looks in public key db.\n");
    
    if (!fSecMsgEnabled)
        throw runtime_error("Secure messaging is disabled.");
    
    
    std::string address   = params[0].get_str();
    std::string publicKey;
    
    Object result;
    int rv = SecureMsgGetLocalPublicKey(address, publicKey);
    switch (rv) {
        case 0:
            result.push_back(Pair("result", "Success."));
            result.push_back(Pair("address in wallet", address));
            result.push_back(Pair("compressed public key", publicKey));
            return result; // success, don't check db
        case 2:
        case 3:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Invalid address."));
            return result;
        case 4:
            break; // check db
        //case 1:
        default:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Error."));
            return result;
    }
    
    CBitcreditAddress coinAddress(address);
    
    CKeyID keyID;
    if (!coinAddress.GetKeyID(keyID)) {
        result.push_back(Pair("result", "Failed."));
        result.push_back(Pair("message", "Invalid address."));
        return result;
    }
    
    CPubKey cpkFromDB;
    rv = SecureMsgGetStoredKey(keyID, cpkFromDB);
    
    switch (rv) {
        case 0:
            if (!cpkFromDB.IsValid() || !cpkFromDB.IsCompressed()) {
                result.push_back(Pair("result", "Failed."));
                result.push_back(Pair("message", "Invalid address."));
            }
            else {
                //cpkFromDB.SetCompressedPubKey(); // make sure key is compressed
                publicKey = EncodeBase58(&cpkFromDB[0], &cpkFromDB[cpkFromDB.size()]);
                
                result.push_back(Pair("result", "Success."));
                result.push_back(Pair("peer address in DB", address));
                result.push_back(Pair("compressed public key", publicKey));
            }
            break;
        case 2:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Address not found in wallet or db."));
            return result;
        //case 1:
        default:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Error, GetStoredKey()."));
            return result;
    }
    
    return result;
}

Value smsgsend(const Array& params, bool fHelp) {

    if (fHelp || params.size() != 3)
        throw runtime_error(
            "smsgsend <addrFrom> <addrTo> <message>\n"
            "Send an encrypted message from addrFrom to addrTo.");
    
    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Secure messaging is disabled.");
    
    std::string addrFrom  = params[0].get_str();
    std::string addrTo    = params[1].get_str();
    std::string msg       = params[2].get_str();
     
    Object result;
    
    std::string sError;
    int error = SecureMsgSend(addrFrom, addrTo, msg, sError);
    sError = "Sending secure message failed - " + sError;
    if (error > 0) {
        throw runtime_error(sError);
    }
    else if (error < 0) {
        throw JSONRPCError(error, sError);
    }

    return Value::null;
}

Value smsgsendanon(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "smsgsendanon <addrTo> <message>\n"
            "Send an anonymous encrypted message to addrTo.");
    
    if (!fSecMsgEnabled)
        throw runtime_error("Secure messaging is disabled.");
    
    std::string addrFrom  = "anon";
    std::string addrTo    = params[0].get_str();
    std::string msg       = params[1].get_str();
    
    
    Object result;
    std::string sError;
    int ret = SecureMsgSend(addrFrom, addrTo, msg, sError);
    sError = "Sending anonymous massage failed - " + sError;
    if (ret > 0) {
        throw runtime_error(sError);
    }
    else if (ret < 0) {
        throw JSONRPCError(ret, sError);
    }

    return Value::null;
}

Value smsginbox(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1) // defaults to read
        throw runtime_error("smsginbox ( \"all|unread|clear\" )\n"
                            "\nDecrypt and display received secure messages in the inbox.\n"
                            "Warning: clear will delete all messages.");
    
    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Secure messaging is disabled.");
    
    if (pwalletMain->IsLocked())
        throw runtime_error("Wallet is locked.");
    
    std::string mode = "unread";
    if (params.size() > 0)
    {
        mode = params[0].get_str();
    } 
    
    Array result;
     
    {
        LOCK(cs_smsgDB);
        
        SecMsgDB dbInbox;
        
        if (!dbInbox.Open("cr+"))
            throw runtime_error("Could not open DB.");
        
        uint32_t nMessages = 0;
        
        std::string sPrefix("im");
        unsigned char chKey[18];
        
        if (mode == "clear")
        {
            dbInbox.TxnBegin();
            
            leveldb::Iterator* it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbInbox.NextSmesgKey(it, sPrefix, chKey))
            {
                dbInbox.EraseSmesg(chKey);
                nMessages++;
            };
            delete it;
            dbInbox.TxnCommit();
            
            ostringstream oss;
            oss << nMessages << " messages deleted";
            return oss.str();
        }
        else if (mode == "all" || mode == "unread") {
            int fCheckReadStatus = mode == "unread" ? 1 : 0;
            
            SecMsgStored smsgStored;
            MessageData msg;
            
            dbInbox.TxnBegin();
            
            leveldb::Iterator* it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbInbox.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                if (fCheckReadStatus
                    && !(smsgStored.status & SMSG_MASK_UNREAD))
                    continue;
                
                Object objM;
                std::string errorMsg;
                int error = SecureMsgDecrypt(smsgStored, msg, errorMsg);
                if (!error) {
                    objM.push_back(Pair("received", GetTimeString(smsgStored.timeReceived)));
                    objM.push_back(Pair("sent", GetTimeString(msg.timestamp)));
                    objM.push_back(Pair("from", msg.sFromAddress.c_str()));
                    objM.push_back(Pair("to", msg.sToAddress.c_str()));
                    objM.push_back(Pair("text", msg.sMessage.c_str()));
                }
                else {
                    ostringstream oss;
                    oss << error;
                    objM.push_back(Pair("error", "Could not decrypt (" + oss.str() + ")"));
                }
                result.push_back(objM);
                
                if (fCheckReadStatus) {
                    smsgStored.status &= ~SMSG_MASK_UNREAD;
                    dbInbox.WriteSmesg(chKey, smsgStored);
                }
                nMessages++;
            }
            delete it;
            dbInbox.TxnCommit();
            
            return result;
        }
    }
    return smsginbox(params, true);
};

Value smsgoutbox(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1) // defaults to read
        throw runtime_error(
            "smsgoutbox ( \"all|clear\" )\n" 
            "Decrypt and display all sent messages.\n"
            "Warning: clear will delete all sent messages.");
    
    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Secure messaging is disabled.");
    
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Wallet is locked, but secure messaging needs an unlocked wallet.");
    
    std::string mode = "all";
    if (params.size() > 0) {
        mode = params[0].get_str();
    }
    
    Array result;
    
    std::string sPrefix("sm");
    unsigned char chKey[18];
    memset(&chKey[0], 0, 18);
    
    {
        LOCK(cs_smsgDB);
        
        SecMsgDB dbOutbox;
        
        if (!dbOutbox.Open("cr+"))
            throw runtime_error("Could not open DB.");
        
        uint32_t nMessages = 0;
        
        if (mode == "clear") {
            dbOutbox.TxnBegin();
            
            leveldb::Iterator* it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbOutbox.NextSmesgKey(it, sPrefix, chKey)) {
                dbOutbox.EraseSmesg(chKey);
                nMessages++;
            }
            delete it;
            dbOutbox.TxnCommit();
            
            return Value::null;
        }
        else if (mode == "all") {
            SecMsgStored smsgStored;
            MessageData msg;
            leveldb::Iterator* it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbOutbox.NextSmesg(it, sPrefix, chKey, smsgStored)) {
                const unsigned char* pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];
                SecureMessageHeader smsg(&smsgStored.vchMessage[0]);
                memcpy(smsg.hash, Hash(&pPayload[0], &pPayload[smsg.nPayload]).begin(), 32);
                int error = SecureMsgDecrypt(false, smsgStored.sAddrOutbox, smsg, pPayload, msg);
                if (!error) {
                    Object objM;
                    objM.push_back(Pair("sent", GetTimeString(msg.timestamp)));
                    objM.push_back(Pair("from", msg.sFromAddress.c_str()));
                    objM.push_back(Pair("to", smsgStored.sAddrTo.c_str()));
                    objM.push_back(Pair("text", msg.sMessage.c_str()));
                    
                    result.push_back(objM);
                }
                else if (error < 0) {
                    throw JSONRPCError(error, "Could not decrypt.");
                }
                else
                    throw runtime_error("Could not decrypt.");
                nMessages++;
            }
            delete it;
            
            return result; 
        }
    }
    
    return smsgoutbox(params, true);
};


Value smsgbuckets(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "smsgbuckets ( \"stats|dump\" )\n"
            "Display some statistics.");
    
    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Secure messaging is disabled.");
    
    std::string mode = "stats";
    if (params.size() > 0) {
        mode = params[0].get_str();
    }
    
    Array result;
    
    if (mode == "stats")
    {
        uint64_t nBuckets = 0;
        uint64_t nMessages = 0;
        uint64_t nBytes = 0;
        {
            LOCK(cs_smsg);
            std::map<int64_t, SecMsgBucket>::iterator it;
            it = smsgBuckets.begin();
            
            for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it)
            {
                std::set<SecMsgToken>& tokenSet = it->second.setTokens;
                
                std::string sBucket = boost::lexical_cast<std::string>(it->first);
                std::string sFile = sBucket + "_01.dat";
                
                nBuckets++;
                nMessages += tokenSet.size();
                
                Object objM;
                objM.push_back(Pair("bucket", (uint64_t) it->first));
                objM.push_back(Pair("time", GetTimeString(it->first)));
                objM.push_back(Pair("no. messages", (uint64_t)tokenSet.size()));
                objM.push_back(Pair("hash", (uint64_t) it->second.hash));
                objM.push_back(Pair("last changed", GetTimeString(it->second.timeChanged)));
                
                boost::filesystem::path fullPath = GetDataDir() / "smsgStore" / sFile;


                if (!boost::filesystem::exists(fullPath))
                {
                    // -- If there is a file for an empty bucket something is wrong.
                    if (tokenSet.size() == 0)
                        objM.push_back(Pair("file size", "Empty bucket."));
                    else
                        objM.push_back(Pair("file size, error", "File not found."));
                } else
                {
                    try {
                        
                        uint64_t nFBytes = 0;
                        nFBytes = boost::filesystem::file_size(fullPath);
                        nBytes += nFBytes;
                        objM.push_back(Pair("file size", nFBytes));
                    } catch (const boost::filesystem::filesystem_error& ex)
                    {
                        objM.push_back(Pair("file size, error", ex.what()));
                    };
                };
                
                result.push_back(objM);
            };
        }; // LOCK(cs_smsg);
        
        Object objM;
        objM.push_back(Pair("buckets", result));
        objM.push_back(Pair("no. buckets", nBuckets));
        objM.push_back(Pair("no. messages", nMessages));
        objM.push_back(Pair("size", nBytes));

        return objM;
    }
    else if (mode == "dump") {
        {
            LOCK(cs_smsg);
            std::map<int64_t, SecMsgBucket>::iterator it;
            it = smsgBuckets.begin();
            
            for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it) {
                std::string sFile = boost::lexical_cast<std::string>(it->first) + "_01.dat";
                
                try {
                    boost::filesystem::path fullPath = GetDataDir() / "smsgStore" / sFile;
                    boost::filesystem::remove(fullPath);
                }
                catch (const boost::filesystem::filesystem_error& ex) {
                    //objM.push_back(Pair("file size, error", ex.what()));
                    printf("Error removing bucket file %s.\n", ex.what());
                }
            }
            smsgBuckets.clear();
        } // LOCK(cs_smsg);
        
        return Value::null;
    }

    return smsgbuckets(params, true);
};
