struct request
{
    char user[10];
    int itemNumber;
    int qty;
    int price;
    char type;      //B v S
    int id;
};

struct userLog      //completed transactions (partially and wholly)
{
    char buyer[10];
    char seller[10];
    int itemNumber;
    int price;
    int qty;
    int buyRequestID;
    int sellRequestID;
};

struct request userRequests[5][500];        //initial requests by users

int nRequest[5] = {0};

struct userLog tradeLog[5000];              //transactions completed, wholly or partially

int nTradeLog = 0;

int requestID=0;
struct request buyQueue[10][500];

struct request sellQueue[10][500];

int buyHead[10]={0}, buyTail[10]={0}, sellHead[10]={0}, sellTail[10]={0};

void sortInsert(struct request);       //on basis of price then id

void removeItem(struct request);

