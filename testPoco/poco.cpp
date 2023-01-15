#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/URI.h"
#include <fstream>

using Poco::URI;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::ServerSocket;
using Poco::Util::ServerApplication;

class MyRequestHandler : public HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest &request, HTTPServerResponse &response)
    {
        URI uri(request.getURI());
        std::string path = uri.getPath();
        std::string fileName = "." + path;
        if (path == "/")
        {
            fileName = "./index.html";
        }
        if (path == "/app.vue")
        {
            fileName = "./app.vue";
            response.setContentType("application/javascript");
        }
        std::ifstream file(fileName, std::ios::in | std::ios::binary);
        if (!file)
        {
            response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
            response.setContentLength(0);
            response.send();
            return;
        }
        file.seekg(0, std::ios::end);
        std::streamsize length = file.tellg();
        file.seekg(0, std::ios::beg);
        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentLength(length);
        response.send() << file.rdbuf();
    }
};

class MyRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
    HTTPRequestHandler *createRequestHandler(const HTTPServerRequest &request)
    {
        return new MyRequestHandler;
    }
};

class MyServerApp : public ServerApplication
{
protected:
    int main(const std::vector<std::string> &args)
    {
        ServerSocket socket(8080);
        Poco::Net::HTTPServerParams *pParams = new Poco::Net::HTTPServerParams;
        HTTPServer server(new MyRequestHandlerFactory, socket, pParams);
        server.start();
        waitForTerminationRequest();
        server.stop();
        return Application::EXIT_OK;
    }
};

int main(int argc, char **argv)
{
    MyServerApp app;
    return app.run(argc, argv);
}