
#include <iostream>
#include <csignal>
#include "Logger.hpp"
#include "Config.hpp"
#include "Server.hpp"
#include "Utils.hpp"


Server* g_server = 0;

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        Logger::info("Received shutdown signal");
        if (g_server) {
            g_server->stop();
        }
    }
}

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
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe signals
    
    // Create and start server
    Server server;
    g_server = &server;
    
    if (!server.init(&config))
    {
        Logger::error("Failed to initialize server");
        return 1;
    }
    
    if (!server.start())
    {
        Logger::error("Failed to start server");
        return 1;
    }

    //Logger::setLevel(DEBUG);
    Logger::setLevel(INFO);
    
    server.run();
    
    Logger::info("Webserv shutdown complete");
    return 0;
}