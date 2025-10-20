
#include <iostream>
#include <csignal>
#include "Logger.hpp"
#include "Config.hpp"
#include "Server.hpp"
#include "Utils.hpp"

bool isYamlFile(const std::string& filename)
{
    return Utils::endsWith(filename, ".yml") || Utils::endsWith(filename, ".yaml");
}

int main(int argc, char **argv)
{
    Logger::info("Webserv starting...");
    
    // Check arguments
    if (argc != 2)
    {
        Logger::error("Usage: " + std::string(argv[0]) + " <config_file>");
        return 1;
    }
    
    std::string configFile = argv[1];
    Config config;
    
    if (isYamlFile(configFile)) {
        Logger::info("Detected YAML configuration format");
        if (!config.parseYaml(configFile)) {
            Logger::error("Failed to parse YAML configuration file");
            return 1;
        }
    } else {
        Logger::info("Detected NGINX configuration format");
        if (!config.parseNginx(configFile)) {
            Logger::error("Failed to parse NGINX configuration file");
            return 1;
        }
    }
    
    if (!config.isValid())
    {
        Logger::error("Configuration validation failed");
        return 1;
    }
    
    Logger::info("Configuration parsed successfully");
    Logger::info("Found " + Utils::intToString(config.getServerCount()) + " server(s)");
    
    // Setup signal handlers
    signal(SIGINT, Server::signalHandler);
    signal(SIGTERM, Server::signalHandler);
    signal(SIGPIPE, SIG_IGN);
    
    // Create and start server
    Server server;
    Server::setSignalInstance(&server);
    
    if (!server.init(&config))
    {
        Logger::error("Failed to initialize server");
        Server::setSignalInstance(NULL);
        return 1;
    }
    
    if (!server.start())
    {
        Logger::error("Failed to start server");
        Server::setSignalInstance(NULL);
        return 1;
    }
    
    // Logger::setLevel(DEBUG);
    Logger::setLevel(INFO);
    
    server.run();
    
    server.stop();
    
    Server::setSignalInstance(NULL);
    
    Logger::info("Webserv shutdown complete");
    
    return 0;
}
