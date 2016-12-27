#include "stdio.h"
#include <stdlib.h>
#include "netdb.h"
#include "netinet/in.h"
#include "string.h"
#include "server.h"


void writeSocket( int sockfd, char *msg )
{
    int n = write( sockfd, msg, strlen(msg) );
    if (n < 0) {
         perror("ERROR writing to socket");
         
      }
}

char **split(char *buffer)
{
   int l = strlen(buffer), i, tokens=1;
   for(i=0; i<l; i++)
      if(buffer[i]=='\n')
         tokens++;
   
   char *tok;
   char **res;
   res = (char **)malloc(tokens * sizeof(char *));
   tok = strtok(buffer, "\n");
   i=0;
   while(tok!=NULL)
   {
      res[i] = (char *)malloc(sizeof(char)*strlen(tok));
      strcpy(res[i], tok);
      i++;
      tok = strtok(NULL, "\n");
   }
   return res;
}

// 1 means user and pass both match
// 2 means correct user, wrong pass
// 3 means both incorrect
int authValid( char *username, char *pass, int *userNum )
{
    FILE *fp = fopen("login.txt", "r");
    char buff[50];
    *userNum=0;
    while( fgets(  buff, sizeof(buff), fp) != NULL )
    {
        *userNum += 1;
        char *user1 = strtok(buff, ":");
        char *pass1 = strtok(NULL, "\n");
        if( strcmp( username, user1) == 0 )
        {
            if( strcmp(pass, pass1)==0)
               return 1;
            else 
               return 2;
        }
    }
    return 3;
}

// compare requests
int rlessthanr(struct request l, struct request r) {
   if(l.price < r.price) return 1;
   if(l.price == r.price) {
      return (l.id < r.id);
   }
   return 0;
}

// swap requests
void swap_requests(struct request *a, struct request *b) {
   struct request temp;
   temp = *a;
   *a = *b;
   *b = temp;
}

// Insert request in sorted order into the rigth queue
void sortInsert(struct request req) {
   struct request *arr = ( (req.type=='B')?(buyQueue[req.itemNumber]):(sellQueue[req.itemNumber]) );
   int *head = ( (req.type == 'B') ? (buyHead + req.itemNumber) : (sellHead + req.itemNumber) );
   int *tail = ( (req.type == 'B') ? (buyTail + req.itemNumber) : (sellTail + req.itemNumber) );
   // empty queue?
   if(*head == *tail) {
      arr[*head] = req;
      *tail = (*tail + 1)%500;
   }
   else {
      int idx = *head, i;
      while(!rlessthanr(req, arr[idx]) && idx != *tail) {
         idx = (idx+1)%500;
      }
      // insert element at idx
      // shift all elements from idx forward
      struct request temp = req;
      for(i = idx; i != *tail; i = (i+1)%500 ) {
         swap_requests(arr+i, &temp);
      }
      arr[*tail] = temp;
      *tail = (*tail + 1)%500;
   }
   
}

// check is sequence of bytes is present in memory buffer
int memsubstr(const char *buf, int len, const char *seq) {
   int i;
   for(i=0; i+strlen(seq) < len ; i++) {
      if(memcmp(buf + i, seq, strlen(seq)) == 0) {
         return 1;
      }
   }
   return 0;
}


int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno, clilen;
   char buffer[65536];
   struct sockaddr_in serv_addr, cli_addr;
   int  n;
   char ip[] = "127.0.0.1";  

   if(argc<2)
   {
      fprintf(stderr,"usage %s port\n", argv[0]);
      exit(0);
   }

   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   
   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = atoi(argv[1]);
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   
   /* Now bind the host address using bind() call.*/
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }
      
   /* Now start listening for the clients, here process will
      * go in sleep mode and will wait for the incoming connection
   */
   
   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   
   while(1)
   {
        /* Accept actual connection from the client */\
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }
      
        /* If connection is established then start communicating */
        char buff[256];
        int offset = 0;
        
        while(1) {
            memset(buff, '\0', 256);
            int sz = read( newsockfd, buff, 256, 0);
            if(sz < 0) {
                printf("Error reading from socket\n");
                exit(-1);
        }
        memcpy(buffer+offset, buff, sz);
        offset += sz;
        if(memsubstr(buff, 256, "$$")) break;
    }
    buffer[offset] = '\0';
      
      
      
      
    printf("Here is the message:\n%s\n",buffer);
    char **msg = split(buffer);
      
      
    if(strcmp(msg[2], "LOGIN")==0)
    {
        
        int userNum;
        int res = authValid(msg[0], msg[1], &userNum);
        switch( res )
        {
            case 1:     
                writeSocket( newsockfd, "SUCCESS\nLOG IN Successful!\n") ;
                break;
            case 2:     
                writeSocket(newsockfd, "FAIL\nWrong Password!!!\nTry again\n");
                break;
            case 3: 
                writeSocket(newsockfd, "FAIL\nWrong username!!!\nTry again\n");
                break;
        }
    }
    else if(strcmp(msg[2], "BUY")==0)
    {
        int userNum;
        int res = authValid(msg[0], msg[1], &userNum);
        if(res != 1)
        {
            writeSocket( newsockfd, "FAIL\nLOG IN UnSuccessful!\n") ;
            close(newsockfd);
            continue;
        }
        /*construct the buy request structure*/
        int i;
        int item = atoi( msg[3] );
        int qty =  atoi( msg[4] );
        int unitPrice = atoi( msg[5] );
        struct request buy;
        strcpy(buy.user, msg[0]);
        buy.itemNumber = item;
        buy.qty = qty;
        buy.price = unitPrice;
        buy.id = requestID++;
        buy.type = 'B';
        //add to the initial user request log
        userRequests[userNum][ nRequest[ userNum ]++ ] = buy;
        //check if you get at least one seller
        int check =0;
        for(i=sellHead[item]; i != sellTail[item]; i = (i+1)%500)           //check for each pending sell request
        {
            if( sellQueue[item][i].price <= unitPrice )                     //if price is compatible
            {
                if( sellQueue[item][i].qty >= buy.qty )                     //if the item can be fully purchased
                {
                    check = 1;
                    sellQueue[item][i].qty -= buy.qty;                      //update the initial sell request
                    if(sellQueue[item][i].qty == 0)                         //if sell qty is zero remove the request
                        sellHead[item] = (sellHead[item]+1)%500;
                    
                    /*update the transaction log*/
                    
                    struct userLog tempLog; strcpy(tempLog.buyer, msg[0]); strcpy(tempLog.seller, sellQueue[item][i].user); tempLog.itemNumber = item; 
                    tempLog.price = sellQueue[item][i].price; tempLog.qty = buy.qty; tempLog.buyRequestID = buy.id; 
                    tempLog.sellRequestID = sellQueue[item][i].id; tradeLog[nTradeLog++] = tempLog;
                    break;
                }
                else                                                        //the sell qty is less than that of buy
                {
                    sellHead[item] = (sellHead[item]+1)%500;                //remove from sell queue
                    buy.qty -= sellQueue[item][i].qty;                      //update initial request
                    /*update the transaction log*/
                    struct userLog tempLog; strcpy(tempLog.buyer, msg[0]); strcpy(tempLog.seller, sellQueue[item][i].user); tempLog.itemNumber = item; 
                    tempLog.price = sellQueue[item][i].price; tempLog.qty = sellQueue[item][i].qty; tempLog.buyRequestID = buy.id; 
                    tempLog.sellRequestID = sellQueue[item][i].id; tradeLog[nTradeLog++] = tempLog;
                }
            }
        }
        
        //if no seller found, insert in buy queue
        if(check == 0)
            sortInsert(buy);
        
        writeSocket(newsockfd, "SUCCESS\n");
    }
    else if(strcmp(msg[2], "SELL")==0)
    {
        int userNum;
        int res = authValid(msg[0], msg[1], &userNum);
        if(res != 1)
        {
            writeSocket( newsockfd, "FAIL\nLOG IN UnSuccessful!\n") ;
            close(newsockfd);
            continue;
        }
        //construct initial sell request
        int i;
        int item = atoi( msg[3] );
        int qty =  atoi( msg[4] );
        int unitPrice = atoi( msg[5] );
        struct request sell;
        strcpy(sell.user, msg[0]);
        sell.itemNumber = item;
        sell.qty = qty;
        sell.price = unitPrice;
        sell.id = requestID++;
        sell.type = 'S';
        //add to user request log
        userRequests[userNum][ nRequest[ userNum ]++ ] = sell;
        
        while( sell.qty > 0 )
        {
            if(buyHead[item] == buyTail[item])          //if no buyer available, add to sell queue
            {
                sortInsert(sell);
                break;
            }
            int bestSell = buyHead[item];
            //find index in buy queue with highest price in FCFS manner
            for(i=buyHead[item]+1; i != buyTail[item]; i = (i+1)%500)       //check for each sell request pending
            {
                if( (buyQueue[item][i].price > buyQueue[item][bestSell].price) || ( (buyQueue[item][i].price == buyQueue[item][bestSell].price) && (buyQueue[item][i].id < buyQueue[item][bestSell].id) ) )
                    bestSell = i;
            }
            if( buyQueue[item][bestSell].price >= sell.price )              //if the price is compatible
            {
                if( buyQueue[item][bestSell].qty > sell.qty )               //sell whole amount in request
                {
                    buyQueue[item][bestSell].qty -= sell.qty;               //update the buy queue qty
                    /*construct the transaction and update*/
                    struct userLog tempLog; strcpy(tempLog.seller, msg[0]); strcpy(tempLog.buyer, buyQueue[item][bestSell].user); tempLog.itemNumber = item; 
                    tempLog.price = sell.price; tempLog.qty = sell.qty; tempLog.buyRequestID = buyQueue[item][bestSell].id; 
                    tempLog.sellRequestID = sell.id; tradeLog[nTradeLog++] = tempLog;
                    
                    sell.qty = 0;
                    break;
                }
                else                                                        //sell request still remains
                {
                    sell.qty -= buyQueue[item][bestSell].qty;               //update sell request qty
                    /*update the trransaction log*/
                    struct userLog tempLog; strcpy(tempLog.seller, msg[0]); strcpy(tempLog.buyer, buyQueue[item][bestSell].user); tempLog.itemNumber = item; 
                    tempLog.price = sell.price; tempLog.qty = buyQueue[item][bestSell].qty; tempLog.buyRequestID = buyQueue[item][bestSell].id; 
                    tempLog.sellRequestID = sell.id; tradeLog[nTradeLog++] = tempLog;
                    //remove the entry form buy queue
                    for( i=bestSell; i!=buyTail[item]; i=(i+1)%500 )
                        buyQueue[item][i] = buyQueue[item][(i+1)%500];
                    buyTail[item] = (500+buyTail[item] - 1)%500;
                }
            }
            else                                                            //price not compatible
            {
                sortInsert(sell);                                           //add to sell queue
                break;
            }
        }
        writeSocket(newsockfd, "SUCCESS\n");
    }
    /*display best sell and best buy for each item*/
    else if(strcmp(msg[2], "VIEW_ORDERS")==0)
    {
        int i;
        char msg[500] = "\0";
        for(i=0; i<10; i++)
        {
            sprintf(msg+strlen(msg), "Item: %d\n", i);
            strcpy(msg+strlen(msg), "   best sell: ");
            if( sellHead[i] != sellTail[i] )
            {
                sprintf(msg+strlen(msg), "qty- %d",sellQueue[i][sellHead[i]].qty );
                strcpy(msg+strlen(msg), ", ");
                sprintf(msg+strlen(msg), "price- %d", sellQueue[i][sellHead[i]].price);
            }
            else
                strcpy(msg+strlen(msg), "NA");
            strcpy(msg+strlen(msg), "\n");
              
            strcpy(msg+strlen(msg), "   best Buy: ");
            if( buyHead[i] != buyTail[i] )
            {  
                sprintf(msg+strlen(msg), "qty- %d", buyQueue[i][(500+buyTail[i]-1)%500].qty);
                strcpy(msg+strlen(msg), ",");
                sprintf(msg+strlen(msg), "price- %d", buyQueue[i][(500 + buyTail[i]-1)%500].price);
            }
            else
                strcpy(msg+strlen(msg), "NA");
            strcpy(msg+strlen(msg), "\n");
        }
        writeSocket(newsockfd, "SUCCESS\n");
        writeSocket(newsockfd, msg);
    }  
    else if(strcmp(msg[2], "VIEW_TRADES")==0)
    {
        int userNum;
        int res = authValid(msg[0], msg[1], &userNum);
        printf("%s: %d\n", msg[0], nRequest[userNum]);
        if(res != 1)
        {
            writeSocket( newsockfd, "FAIL\nLOG IN UnSuccessful!\n") ;
            close(newsockfd);
            continue;
        }
        char sendMsg[500];
        writeSocket(newsockfd, "SUCCESS\n");
        int i, j;
        for(j=0; j<nRequest[userNum]; j++)                                  //iterate in all initial requests of a given user
        {
            int getBuyID = userRequests[userNum][j].id;                     //get the buy id
            //print the initial request
            sprintf(sendMsg, "INITIAL REQUEST\n");
            writeSocket(newsockfd, sendMsg);
            sprintf(sendMsg, "%s %d %d %d %c %d\n", userRequests[userNum][j].user, userRequests[userNum][j].itemNumber, userRequests[userNum][j].qty, userRequests[userNum][j].price, userRequests[userNum][j].type, userRequests[userNum][j].id);
            writeSocket(newsockfd, sendMsg);
            sprintf(sendMsg, "(BUYER SELLER ITEM PRICE QTY BUY_REQUEST_ID SELL_REQUEST_ID)\n");
            writeSocket(newsockfd, sendMsg);
            for(i=0; i<nTradeLog; i++)                                      //check each transaction log
            {
                if(tradeLog[i].buyRequestID == getBuyID)                    //user in question is buyer
                {
                    sprintf(sendMsg, "%s %s %d %d %d %d %d\n", tradeLog[i].buyer, tradeLog[i].seller, tradeLog[i].itemNumber, tradeLog[i].qty, tradeLog[i].price, tradeLog[i].buyRequestID, tradeLog[i].sellRequestID);
                    writeSocket(newsockfd, sendMsg);
                }
                else if(tradeLog[i].sellRequestID == getBuyID)              //user in question is seller
                {
                    sprintf(sendMsg, "%s %s %d %d %d %d %d\n", tradeLog[i].buyer, tradeLog[i].seller, tradeLog[i].itemNumber, tradeLog[i].qty, tradeLog[i].price, tradeLog[i].buyRequestID, tradeLog[i].sellRequestID);
                    writeSocket(newsockfd, sendMsg);
                }
            }
        }
    }  
    close(newsockfd);
   }
   return 0;
}