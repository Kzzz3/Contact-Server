#include "Server/Server.h"
int main()
{
    Server serv(10087,"./log/Contact.log",100000000,24,"127.0.0.1",3306,"Contact","root","Root123!@#qaq",10,10);
    serv.start();
    
    return 0;
}
