#include "RdmaImpl.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include "org_apache_hadoop_hbase_ipc_RdmaNative.h"

using namespace std;
int main(int argc, char **argv) {
    bool isServer = true;
    int serverPort = 25543;
    string serverName;
    Java_org_apache_hadoop_hbase_ipc_RdmaNative_rdmaInitGlobal(NULL, NULL);
    if(argc == 1) {
        cout << "server mode" << endl;
        Java_org_apache_hadoop_hbase_ipc_RdmaNative_rdmaBind(NULL, NULL, serverPort);
        CRdmaServerConnectionInfo conn;
        void *dataPtr;
        uint64_t size;
        string responseData;
        string responseData2;
        conn.waitAndAccept();
        cout << "accepted. client addr " << conn.getClientIp() << endl;


        while(!conn.canReadQuery());
        conn.readQuery(dataPtr, size);
        cout << "query:" << (char *)dataPtr << ", with length " << size << endl;
        while(!conn.canReadQuery());
        conn.readQuery(dataPtr, size);
        cout << "NESTED query:" << (char *)dataPtr << ", with length " << size << endl;

        responseData = "fuckFirstPkg";
        responseData2 = "nested fuckFirstPkg";
        while(!conn.canWriteResponse());
        conn.writeResponse(responseData.data(), responseData.size());
        while(!conn.canWriteResponse());
        conn.writeResponse(responseData2.data(), responseData2.size());
        cout << "Sleeping 5 seconds to wait for the client reading response..." << endl;
        sleep(5); // the client is still reading thr response!

        cout << "---- Test the second and 3rd round(parallel read Query & write response)! ----" << endl;
        CRdmaServerConnectionInfo anotherConn;
        anotherConn.waitAndAccept();
        cout << "accepted. client addr " << anotherConn.getClientIp() << endl;

        while(!conn.canReadQuery());
        conn.readQuery(dataPtr, size);
        cout << "query:" << (char *)dataPtr << ", with length " << size << endl;
        while(!anotherConn.canReadQuery());
        anotherConn.readQuery(dataPtr, size);
        cout << "query:" << (char *)dataPtr << ", with length " << size << endl;

        if(anotherConn.canReadQuery()) throw std::runtime_error("Query is readable after actually read!");
 
        responseData = "FFFFFFFFFFFFFFucking!!!";
        conn.writeResponse(responseData.data(), responseData.size());
        cout << "Sleeping 5 seconds to wait for the client reading response..." << endl;
        sleep(5); // the client is still reading thr response!

        cout << "---- Test the third round! ----" << endl;

        responseData = "Ml with you the 3rd time.";
        anotherConn.writeResponse(responseData.data(), responseData.size());
        cout << "Sleeping 5 seconds to wait for the client reading response..." << endl;
        sleep(5); // the client is still reading thr response!
    }
    else {
        cout << "client mode" << endl;
        serverName = argv[1];
        CRdmaClientConnectionInfo conn;
        string queryData;
        string queryData2;
        infinity::memory::Buffer *bufPtr;
        infinity::memory::Buffer *bufPtr2;

        _again:
        try {
            conn.connectToRemote(serverName.c_str(), serverPort);
        }
        catch(std::exception &e) {sleep(1);goto _again;}

        queryData = "hello";
        queryData2 = "nested shit";
        while(!conn.canWriteQuery());
        conn.writeQuery((void *)queryData.data(), queryData.size());
        while(!conn.canWriteQuery());
        conn.writeQuery((void *)queryData2.data(), queryData2.size());
        while(!conn.canReadResponse());
        conn.readResponse(bufPtr);
        while(!conn.canReadResponse());
        conn.readResponse(bufPtr2);
        cout << "response:" << (char *)bufPtr->getData() << ", with length " << bufPtr->getSizeInBytes() << endl;
        cout << "NESTED response:" << (char *)bufPtr2->getData() << ", with length " << bufPtr2->getSizeInBytes() << endl;

        CRdmaClientConnectionInfo anotherConn;
        _again2:
        try {
        cout << "---- connect the 2nd conn ----" << endl;
            anotherConn.connectToRemote(serverName.c_str(), serverPort);
        cout << "---- connect the 2nd conn done ----" << endl;
        }
        catch(std::exception &e) {sleep(1);goto _again2;}

        cout << "---- Test the second 3rd round! ----" << endl;

        queryData = "hello again";
        conn.writeQuery((void *)queryData.data(), queryData.size());

        queryData = "hello third ~";
        anotherConn.writeQuery((void *)queryData.data(), queryData.size());

        while(!conn.canReadResponse()) sleep(1);
        conn.readResponse(bufPtr);
        cout << "response:" << (char *)bufPtr->getData() << ", with length " << bufPtr->getSizeInBytes() << endl;

        while(!anotherConn.canReadResponse()) sleep(1);
        anotherConn.readResponse(bufPtr);
        cout << "response:" << (char *)bufPtr->getData() << ", with length " << bufPtr->getSizeInBytes() << endl;
    }
    Java_org_apache_hadoop_hbase_ipc_RdmaNative_rdmaDestroyGlobal(NULL, NULL);
}
