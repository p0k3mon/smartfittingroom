#include "IotHttpClient.h"
#include "vmssl.h"
#include "LTask.h"

#include "hmac.h"
#include "sha256.h"
#include "IotUtils.h"

int delayTime = 500;

IotHttpClient::IotHttpClient()
{
}

struct MtkHttpContext
{
    const char *request;
    const char *serverUrl;
    VMINT port;
    String response;
    VMINT data_sent;
    VMINT data_read;
};

MtkHttpContext *pContext;

char* IotHttpClient::send(const char* request, const char* serverUrl, int port) {
    // TODO: probably not the best way to detect protocol...
    switch(port)
    {
        case 80:
            return sendHTTP(request, serverUrl, port);
        case 443:
            return sendHTTPS(request, serverUrl, port);
        default:
            return sendHTTP(request, serverUrl, port);
    }
}

char* IotHttpClient::sendHTTP(const char *request, const char* serverUrl, int port)
{
    /* Arduino String to build the response with. */
    String responseBuilder = "";
    if (client.connect(serverUrl, port)) {
        /* Send the requests */
        client.println(request);
        client.println();
        /* Read the request into responseBuilder. */
        delay(delayTime);
        while (client.available()) {
            char c = client.read();
            responseBuilder.concat(c);
        }
        client.stop();
    } else {
        client.stop();
        /* Error connecting. */
        return 0;
    }
    /* Copy responseBuilder into char* */
    int len = responseBuilder.length();
    char* response = new char[len + 1]();
    responseBuilder.toCharArray(response, len + 1);
    return response;
}

int IotHttpClient::SendAzureHttpsData(std::string data)
{
    // Calculate the HMAC Signature based on AZURE_HOST and AZURE_UTC_2020_01_01.

    std::string hmac_msg(AZURE_HOST"\n"AZURE_UTC_2020_01_01);
    std::string hmac_key(AZURE_KEY);

    std::string hmac_sig = hmac<SHA256>(hmac_msg, hmac_key);
    // Serial.println(hmac_sig.c_str());

    hmac_sig = IotUtils::HexStringToBinary(hmac_sig);
    // Serial.println(hmac_sig.c_str());
    hmac_sig = IotUtils::Base64Encode(hmac_sig);
    // Serial.println(hmac_sig.c_str());
    hmac_sig = IotUtils::UrlEncode(hmac_sig);
    // Serial.println(hmac_sig.c_str());

    // Build the JSON data with event hub parameters.
    std::string header_auth;

    header_auth += "Authorization: SharedAccessSignature ";
    header_auth += "sr="AZURE_SERVICE_BUS_NAME_SPACE".servicebus.windows.net";
    header_auth += "&sig=" + hmac_sig;
    header_auth += "&se="AZURE_UTC_2020_01_01;
    header_auth += "&skn="AZURE_POLICY_NAME;

    // Build HTTP POST request

    std::string http_string;

    http_string += "POST "AZURE_URL" HTTP/1.1\n";
    http_string += "Host: "AZURE_HOST"\n";
    http_string += header_auth + "\n";
    http_string += "Content-Length: " + IotUtils::IntToString(data.length()) + "\n";
    http_string += "Connection: Close\n";
    http_string += "Content-Type: application/atom+xml;type=entry;charset=utf-8\n";
    http_string += "\n";
    http_string += data + "\n";
    http_string += "\n";

    Serial.println("=== HTTP Headers ===");
    Serial.println(http_string.c_str());

    // Submit the  HTTP POST request to the remote server.
    char *response = sendHTTPS(http_string.c_str(), "melisaehprojects-ns.servicebus.windows.net", 443);

    Serial.println("=== HTTP Response ===");
    std::string results(response);
    delete response;

    Serial.print(results.c_str());

   // Extract the HTTP status code from the returned HTTP response.

    // 201 - Success.
    // 401 - Authorization failure.
    // 500 - Internal error.

    std::string pattern("HTTP/1.1 ");
    std::string http_code("0");

    int ix = results.find(pattern);

    if (ix >= 0)
    {
         // The status code is found.
         http_code = results.substr(pattern.length(), 3);
    }

    return atoi(http_code.c_str());
}

char* IotHttpClient::sendHTTPS(const char *request, const char* serverUrl, int port)
{
    //Serial.print("Req=");
    //Serial.println(request);
    //Serial.print("URL=");
    //Serial.println(serverUrl);
    //Serial.print("Port=");
    //Serial.println(port);

    // This method is invoked in Arduino thread
    //
    // Use LTask.remoteCall to invoke the sendHttps()
    // function in LinkIt main thread. vm_ssl APIs must be
    // executed in LinkIt main thread.

    MtkHttpContext context;
    context.data_read = context.data_sent = 0;
    context.request = request;
    context.serverUrl = serverUrl;
    context.port = port;
    context.response = "";
    pContext = &context;
    LTask.remoteCall(sendHTTPS_remotecall, &context);

    // Build the response - TODO: when is this response released?
    int len = context.response.length();
    char* response = new char[len + 1]();
    context.response.toCharArray(response, len + 1);

    //Serial.println("returned response:");
    //Serial.println(response);

    return response;
}

boolean IotHttpClient::sendHTTPS_remotecall(void* user_data)
{
    // This function should be executed in LinkIt main thread.

    // Initialize SSL connection
    vm_ssl_cntx ssl_cntx = {0};
    ssl_cntx.authmod = VM_SSL_VERIFY_NONE;  // Do not limit the encryption type of the server.
    ssl_cntx.connection_callback = sendHTTPS_ssl_callback;   // SSL event handler callback.
    ssl_cntx.host = (VMCHAR*)pContext->serverUrl;
    ssl_cntx.port = pContext->port;
    ssl_cntx.ua = NULL;
    vm_ssl_connect(&ssl_cntx);

    // By returning false in this function,
    // LTask.remoteCall will block the execution
    // of the Arduino thread until LTask.post_signal() is called.
    // We shall invoke LTask.post_signal() in
    // the event handler callback.
    return false;

}

void IotHttpClient::sendHTTPS_ssl_callback(VMINT handle, VMINT event)
{
    // This callback is invoked in LinkIt main thread

    VMINT ret;
    VMCHAR buf[52] = {0,};

    // Serial.print("sslCb event=");
    // Serial.println(event);

    switch(event) {
    case VM_SSL_EVT_CONNECTED:{
        // Serial.println("VM_SSL_EVT_CONNECTED");
    }
        case VM_SSL_EVT_CAN_WRITE:
        {
            // Serial.println("VM_SSL_EVT_CAN_WRITE");
            const size_t requestLength = strlen(pContext->request);

            ret = vm_ssl_write(handle, (VMUINT8*)pContext->request + pContext->data_sent, requestLength);
            if(ret >= 0) {
                pContext->data_sent += ret;
            }
            break;
        }
        case VM_SSL_EVT_CAN_READ:
            // make sure there is an terminating NULL
            // Serial.println("VM_SSL_EVT_CAN_READ");
            ret = vm_ssl_read(handle, (VMUINT8*)buf, sizeof(buf) - 1);
            while(ret > 0) {
                pContext->response.concat(buf);
                memset(buf, 0, sizeof(buf));
                // make sure there is an terminating NULL
                ret = vm_ssl_read(handle, (VMUINT8*)buf, sizeof(buf) - 1);
                pContext->data_read += ret;
            }

            if(ret == VM_TCP_READ_EOF) {
                vm_ssl_close(handle);

                // Allow LTask.remoteCall() to return
                LTask.post_signal();
            }
            break;
        case VM_SSL_EVT_PIPE_BROKEN:
        {Serial.println("VM_SSL_EVT_CAN_READ"); }
        case VM_SSL_EVT_HOST_NOT_FOUND:
        {Serial.println("VM_SSL_EVT_HOST_NOT_FOUND"); }
        case VM_SSL_EVT_PIPE_CLOSED:
        {Serial.println("VM_SSL_EVT_PIPE_CLOSED"); }
        case VM_SSL_EVT_HANDSHAKE_FAILED:
        {Serial.println("VM_SSL_EVT_HANDSHAKE_FAILED"); }
        case VM_SSL_EVT_CERTIFICATE_VALIDATION_FAILED:
        {Serial.println("VM_SSL_EVT_CERTIFICATE_VALIDATION_FAILED"); }
            vm_ssl_close(handle);

            // Allow LTask.remoteCall() to return
            LTask.post_signal();
            break;
        default:
            break;
    }
}
