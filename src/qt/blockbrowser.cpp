#include "blockbrowser.h"
#include "ui_blockbrowser.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"
#include "clientmodel.h"
#include "rpcserver.h"
#include "transactionrecord.h"

#include <sstream>
#include <string>

#include <boost/exception/to_string.hpp>


double getBlockHardness(int height)
{
    const CBlockIndex* blockindex = getBlockIndex(height);

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

int getBlockHashrate(int height)
{
    int lookup = height;

    double timeDiff = getBlockTime(height) - getBlockTime(1);
    double timePerBlock = timeDiff / lookup;

    return (boost::uint64_t)(((qlonglong)getBlockHardness(height) * pow(2.0, 32)) / timePerBlock);
}

const CBlockIndex* getBlockIndex(int height)
{
    std::string hex = getBlockHash(height);
    uint256 hash(hex);
    return mapBlockIndex[hash];
}

std::string getBlockHash(int Height)
{
    if(Height > chainActive.Tip()->nHeight) { return "0df7a63994eb66317fd82c16ee5160adb83acb45f72d5bf359b88e635d7301b8"; }
    if(Height < 0) { return "0df7a63994eb66317fd82c16ee5160adb83acb45f72d5bf359b88e635d7301b8"; }
    int desiredheight;
    desiredheight = Height;
    if (desiredheight < 0 || desiredheight > chainActive.Height())
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = chainActive[Height];
    while (pblockindex->nHeight > desiredheight)
        pblockindex = pblockindex->pprev;
    return pblockindex->phashBlock->GetHex();
}

int getBlockTime(int Height)
{
    std::string strHash = getBlockHash(Height);
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    return pblockindex->nTime;
}

std::string getBlockMerkle(int Height)
{
    std::string strHash = getBlockHash(Height);
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    return pblockindex->hashMerkleRoot.ToString().substr(0,10).c_str();
}

int getBlocknBits(int Height)
{
    std::string strHash = getBlockHash(Height);
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    return pblockindex->nBits;
}

int getBlockNonce(int Height)
{
    std::string strHash = getBlockHash(Height);
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    return pblockindex->nNonce;
}

std::string getBlockDebug(int Height)
{
    std::string strHash = getBlockHash(Height);
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    return pblockindex->ToString();
}

int blocksInPastHours(int hours)
{
    int wayback = hours * 3600;
    bool check = true;
    int height = chainActive.Tip()->nHeight;
    int heightHour = chainActive.Tip()->nHeight;
    int utime = (int)time(NULL);
    int target = utime - wayback;

    while(check)
    {
        if(getBlockTime(heightHour) < target)
        {
            check = false;
            return height - heightHour;
        } else {
            heightHour = heightHour - 1;
        }
    }

    return 0;
}

double getTxTotalValue(std::string txid)
{
    uint256 hash;
    hash.SetHex(txid);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock, true))
        return GetBlockValue(0, 0);

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;

    double value = 0;
    double buffer = 0;
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];

        buffer = value + convertCoins(txout.nValue);
        value = buffer;
    }

    return value;
}

double convertCoins(int64_t amount)
{
    return (double)amount / (double)COIN;
}

std::string getOutputs(std::string txid)
{
    uint256 hash;
    hash.SetHex(txid);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock, true))
        return "fail";

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;

    std::string str = "";
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];
        CTxDestination source;
        ExtractDestination(txout.scriptPubKey, source);
        CBitcreditAddress addressSource(source);
        std::string lol7 = addressSource.ToString();
        double buffer = convertCoins(txout.nValue);
        std::string amount = boost::to_string(buffer);
        str.append(lol7);
        str.append(": ");
        str.append(amount);
        str.append(" BCR");
        str.append("\n");
    }

    return str;
}

std::string getInputs(std::string txid)
{
    uint256 hash;
    hash.SetHex(txid);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock, true))
        return "fail";

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;

    std::string str = "";
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        uint256 hash;
        const CTxIn& vin = tx.vin[i];
        hash.SetHex(vin.prevout.hash.ToString());
        CTransaction wtxPrev;
        uint256 hashBlock = 0;
        if (!GetTransaction(hash, wtxPrev, hashBlock, true))
             return "fail";

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << wtxPrev;

        CTxDestination source;
        ExtractDestination(wtxPrev.vout[vin.prevout.n].scriptPubKey, source);
        CBitcreditAddress addressSource(source);
        std::string lol6 = addressSource.ToString();
        const CScript target = wtxPrev.vout[vin.prevout.n].scriptPubKey;
        double buffer = convertCoins(getInputValue(wtxPrev, target));
        std::string amount = boost::to_string(buffer);
        str.append(lol6);
        str.append(": ");
        str.append(amount);
        str.append("BCR");
        str.append("\n");
    }

    return str;
}

int64_t getInputValue(CTransaction tx, CScript target)
{
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];
        if(txout.scriptPubKey == target)
        {
            return txout.nValue;
        }
    }
    return 0;
}

double getTxFees(std::string txid)
{
    uint256 hash;
    hash.SetHex(txid);


    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock, true))
        return 0.0001;

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;

    double value = 0;
    double buffer = 0;
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];

        buffer = value + convertCoins(txout.nValue);
        value = buffer;
    }

    double value0 = 0;
    double buffer0 = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        uint256 hash0;
        const CTxIn& vin = tx.vin[i];
        hash0.SetHex(vin.prevout.hash.ToString());
        CTransaction wtxPrev;
        uint256 hashBlock0 = 0;
        if (!GetTransaction(hash0, wtxPrev, hashBlock0, true))
             return 0;
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << wtxPrev;
        const CScript target = wtxPrev.vout[vin.prevout.n].scriptPubKey;
        buffer0 = value0 + convertCoins(getInputValue(wtxPrev, target));
        value0 = buffer0;
    }

    return value0 - value;
}


BlockBrowser::BlockBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BlockBrowser)
{
    ui->setupUi(this);
      
    ui->addrFrame->hide();
    ui->blockFrame->hide();
    ui->txFrame->hide();
    
    connect(ui->blockButton, SIGNAL(pressed()), this, SLOT(blockClicked()));
    connect(ui->txButton, SIGNAL(pressed()), this, SLOT(txClicked()));
    connect(ui->addrButton, SIGNAL(pressed()), this, SLOT(addrClicked()));
}


void BlockBrowser::txClicked()
{
    ui->blockFrame->hide();
    ui->txFrame->show();
    ui->addrFrame->hide();
    
    std::string txid = ui->txBox->text().toUtf8().constData();
    double value = getTxTotalValue(txid);
    double fees = getTxFees(txid);
    std::string outputs = getOutputs(txid);
    std::string inputs = getInputs(txid);
    QString QValue = QString::number(value, 'f', 6);
    QString QID = QString::fromUtf8(txid.c_str());
    QString QOutputs = QString::fromUtf8(outputs.c_str());
    QString QInputs = QString::fromUtf8(inputs.c_str());
    QString QFees = QString::number(fees, 'f', 6);
    ui->valueBox->setText(QValue + " BCR");
    ui->txID->setText(QID);
    ui->outputBox->setText(QOutputs);
    ui->inputBox->setText(QInputs);
    ui->feesBox->setText(QFees + " BCR");
}

void BlockBrowser::blockClicked()
{
    ui->blockFrame->show();
    ui->txFrame->hide();
    ui->addrFrame->hide();
    
    int height = ui->heightBox->value();
    if (height > chainActive.Tip()->nHeight)
    {
        ui->heightBox->setValue(chainActive.Tip()->nHeight);
        height = chainActive.Tip()->nHeight;
    }
    int Pawrate = getBlockHashrate(height);
    double Pawrate2 = 0.000000;
    Pawrate2 = ((double)Pawrate);
    std::string hash = getBlockHash(height);
    std::string merkle = getBlockMerkle(height);
    int nBits = getBlocknBits(height);
    int nNonce = getBlockNonce(height);
    int atime = getBlockTime(height);
    double hardness = getBlockHardness(height);
    QString QHeight = QString::number(height);
    QString QHash = QString::fromUtf8(hash.c_str());
    QString QMerkle = QString::fromUtf8(merkle.c_str());
    QString QBits = QString::number(nBits);
    QString QNonce = QString::number(nNonce);
    QString QTime = QString::number(atime);
    QString QHardness = QString::number(hardness, 'f', 8);
    QString QPawrate = QString::number(Pawrate2, 'f', 6);
    ui->heightLabel->setText(QHeight);
    ui->hashBox->setText(QHash);
    ui->merkleBox->setText(QMerkle);
    ui->bitsBox->setText(QBits);
    ui->nonceBox->setText(QNonce);
    ui->timeBox->setText(QTime);     
    ui->hardBox->setText(QHardness);
    ui->pawBox->setText(QPawrate + " H/s");
}

void BlockBrowser::addrClicked()
{
    ui->blockFrame->hide();
    ui->txFrame->hide();
    ui->addrFrame->show();
    //QString Address = ui->addrBox->text();
    //ui->trustrating->setText("Investigating: " + Address);
}

void BlockBrowser::setModel(ClientModel *model)
{
    this->model = model;
}

BlockBrowser::~BlockBrowser()
{
    delete ui;
}
