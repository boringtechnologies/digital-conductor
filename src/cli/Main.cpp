#include "licensing/LicenseManager.hpp"
#include "auto-discovery/DiscoveryManager.hpp"
#include <cxxopts.hpp>
#include <thread>

using namespace dc;

void discoveryTask(DiscoveryManager dm, std::string group, std::string type)
{
    while (true) {
        if (dm.registered == false) {
            dm.connectToServer("autodiscovery.boring.tech");
        }
        dm.pingServer(group, type, dm.client_socket);
        dm.receiveGroupList(dm.client_socket);
        std::chrono::milliseconds duration( 1000 );
        std::this_thread::sleep_for( duration );
    }
    dm.unregisterWithServer(group, type, dm.client_socket);
}


int main(int argc, char *argv[])
{

    LicenseManager lm;
    DiscoveryManager dm;
  
    cxxopts::Options options("./digitalconductor", "MSPX Digital Conductor Usage");

    options.add_options()
        ("t,type", "Client / Sender", cxxopts::value<std::string>())
        ("g,group", "Group Name", cxxopts::value<std::string>())
        ("d,debug", "Enable Debug Mode", cxxopts::value<bool>()->default_value("false"))
        ("generate_token", "Generate Token", cxxopts::value<bool>()->default_value("false"))
        ("register_license", "Register License", cxxopts::value<std::string>())
        ("generate_license", "Generate License", cxxopts::value<std::string>())
        ("expiry_date", "License Expiration Date", cxxopts::value<std::string>())
        ("h,help", "Print usage")
    ;

    cxxopts::ParseResult result = options.parse(argc, argv);

    bool debug;
    std::string type;
    std::string group;

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    if (result.count("debug"))
        debug = result["debug"].as<bool>();  

    if (result.count("generate_token"))
    {
        std::cout << lm.GenerateDeviceToken() << std::endl;
        exit(0);
    }    
    if (result.count("register_license"))
    {
        std::string register_license = result["register_license"].as<std::string>();
        lm.RegisterLicense(register_license);
        std::cout << "License is registered!" << std::endl;
        exit(0);
    }    
    if (result.count("generate_license") && result.count("expiry_date"))
    {
        std::string generatedKey = result["generate_license"].as<std::string>();
        std::string expiry_date = result["expiry_date"].as<std::string>();
        std::string license = lm.GenerateLicenseKey(generatedKey, 1, 0, 0);
        std::cout << license << std::endl;
        exit(0);
    }


    if (result.count("type")) {
        type = result["type"].as<std::string>(); 
    }
    else {
        std::cout << "Must provide a type (sender/receiver)" << std::endl;
        exit(0);
    }

    if (result.count("group")) {
        group = result["group"].as<std::string>();
    }
    else {
        std::cout << "Must provide group" << std::endl;
        exit(0);
    }


    if (lm.LicenseCheck() == false) {
        std::cout << "digitalconductor.lic not found. Please contact product owner to obtain a license key." << std::endl;
    }

    std::thread discovery(discoveryTask, dm, group, type);

    discovery.join();

    return 0;
}
