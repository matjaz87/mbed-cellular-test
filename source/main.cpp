/*

 */

#include "mbed.h"
#include "CellularNonIPSocket.h"
#include "UDPSocket.h"
#include "TCPSocket.h"
#include "cellular_demo_tracing.h"
#include <cstdint>
#include <cstdio>


using namespace std::chrono;


/** Connects to the Cellular Network */
bool connect_cellular(NetworkInterface &net)
{
    uint8_t RETRY_COUNT = 3;
    printf("Establishing connection\n");

    /* check if we're already connected */
    if (net.get_connection_status() == NSAPI_STATUS_GLOBAL_UP) {
        return true;
    }

    nsapi_error_t ret;

    for (uint8_t retry = 0; retry <= RETRY_COUNT; retry++) {
        ret = net.connect();

        if (ret == NSAPI_ERROR_OK) {
            printf("Connection Established.\n");
            return true;
        } else if (ret == NSAPI_ERROR_AUTH_FAILURE) {
            printf("Authentication Failure.\n");
            return false;
        } else {
            printf("Couldn't connect: %d, will retry\n", ret);
        }
    }

    printf("Fatal connection failure: %d\n", ret);

    return false;
}

nsapi_size_or_error_t ret;
TCPSocket socket;
SocketAddress socket_address;
//const char* target_ip = "echo.mbedcloudtesting.com";
const char* target_ip = "212.183.159.230";  //http://xcal1.vodafone.co.uk/
uint16_t port = 80;

int main() 
{
#ifdef MBED_DEBUG
    HAL_DBGMCU_EnableDBGSleepMode();
#endif
    printf("\nmbed-os-cellular started\n");

    trace_open();

    NetworkInterface *net = CellularContext::get_default_instance();

    if (!net) 
    {
        printf("Failed to get_default_instance()\n");
        return 0;
    }

    net->set_default_parameters();

    if(!connect_cellular(*net))
    {
        printf("Failed to connect cellular.");
        net->disconnect();
        return 0;
    }


    SocketAddress my_ip;
    ret = net->get_ip_address(&my_ip);
    if(ret < 0)
    {
        printf("Failed to get ip address. Error code:%d\n", ret);        
    }
    else 
    {
        printf("My ip:%s\n", my_ip.get_ip_address());        
    }

    ret = net->gethostbyname(target_ip, &socket_address);
    if ( ret < 0)
    {
        printf("Failed to get hostname. Error code:%d\n", ret);
        net->disconnect();
        return 0;
    }

    socket_address.set_port(port);

    ret = socket.open(net);
    if ( ret < 0)
    {
        printf("Failed to open socket. Error code:%d\n", ret);
        net->disconnect();
        return 0;
    }

    ret = socket.connect(socket_address);
    if ( ret < 0)
    {
        printf("Failed to open socket. Error code:%d\n", ret);
        net->disconnect();
        return 0;
    }    

    char get_http[] = "GET /5MB.zip HTTP/1.1\r\nHost: 212.183.159.230\r\n\r\n";

    ret = socket.send(get_http, strlen(get_http));
    if (ret < 0) 
    {
        printf("Failed to send TCP msg. Error code:%d\n", ret);

    } else {
        printf("TCP msg sent to %s\n", socket_address.get_ip_address());
    }

    ThisThread::sleep_for(500ms);

    // Wait for a response
    char receive_buff[1024];
    int index = 0;
    Timer t;
    t.start();

    while (true)
    {
        ret = socket.recv(receive_buff, sizeof(receive_buff));
        if(ret < 0)
        {
            printf("Failed to receive TCP response msg. Error code:%d\n", ret); 
            net->disconnect();
            return 0;            
        }
        index++;
        if(index == 1957)
        {
            t.stop();
            printf("Index:%d, ret:%d, time:%lld\n", index, ret, duration_cast<seconds>(t.elapsed_time()).count());
            break;
        }

        //printf("TCP response from:%s\n", socket_address.get_ip_address());
        //printf("TCP response data: %.*s\n", ret, receive_buff);
    }

    // Close the socket and the network interface
    ret = socket.close();
    if(ret < 0)
    {
        printf("Failed to close socket. Error code:%d\n", ret); 
        net->disconnect();
        return 0;            
    }
    ret = net->disconnect();
    if(ret < 0)
    {
        printf("Failed to disconect network interface. Error code:%d\n", ret);
        return 0;
    }
    trace_close();

    printf("mbed-os-cellular finished\n");

    return 0;
}