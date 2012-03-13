/* Copyright (c) 2011 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <signal.h>

#include "boost/program_options.hpp"

#include "maidsafe/transport/nat-detection/nat_detection_node.h"
#include "maidsafe/transport/version.h"
#include "maidsafe/transport/log.h"

namespace po = boost::program_options;
namespace mt = maidsafe::transport;
namespace {

void ConflictingOptions(const po::variables_map &variables_map,
                        const char *opt1,
                        const char *opt2) {
  if (variables_map.count(opt1) && !variables_map[opt1].defaulted()
      && variables_map.count(opt2) && !variables_map[opt2].defaulted()) {
    throw std::logic_error(std::string("Conflicting options '") + opt1 +
                           "' and '" + opt2 + "'.");
  }
}

volatile bool ctrlc_pressed(false);

void CtrlCHandler(int /*a*/) {
  ctrlc_pressed = true;
}
 
}   // unnamed namespace

int main(int argc, char **argv) {
  std::string logfile, bootstrap_file("bootstrap_contacts"),
      proxy_bootstrap_file("proxy_bootstrap_contacts");
  po::options_description options_description("Options");
  mt::detection::NatDetectionNode node;
  options_description.add_options()
      ("help,h", "Print options.")
      ("setup,s", "Setting up")
      ("version,V", "Print program version.")
      ("logfile,l", po::value(&logfile), "Path of log file.")
      ("verbose,v", po::bool_switch(), "Verbose logging to console and file.")        
      ("client,c", po::bool_switch(), "Start the node as a client node.")
      ("rendezvous,r", po::bool_switch(),
           "Start the node as a rendezvous node.")
      ("proxy,p", po::bool_switch(), "Start the node as a proxy node.")
      ("bootstrap,b", po::value<std::string>
          (&bootstrap_file)->default_value(bootstrap_file),
          "Path to file with bootstrap nodes.")
      ("proxy_bootstrap,x", po::value<std::string>
          (&proxy_bootstrap_file)->default_value(proxy_bootstrap_file),
          "Path to file with proxy bootstrap nodes.");
        
    po::variables_map variables_map;
    po::store(po::parse_command_line(argc, argv, options_description),
              variables_map);
    
    if (variables_map.count("help")) {
      std::cout << options_description << std::endl;
      return 0;
    }

    if (variables_map.count("version")) {
      std::cout << "MaidSafe-Transport "
                << maidsafe::GetMaidSafeVersion(MAIDSAFE_TRANSPORT_VERSION)
                << std::endl;
      return 0;
    }
    
    if (variables_map.count("setup")) {
       std::cout << "Setting up involves running three nodes as:" << std::endl;
       std::cout << "  1) A proxy node which will be visible ";
       std::cout << "to rendezvous node," << std::endl;
       std::cout << "  2) A directly connected rendezvous node using ";
       std::cout << "the bootstrap file created by proxy, and" << std::endl;
       std::cout << "  3) A client node using the bootstrap file ";
       std::cout << "created by rendezvous node." << std::endl;
    }

    ConflictingOptions(variables_map, "client", "rendezvous");
    ConflictingOptions(variables_map, "client", "proxy");
    ConflictingOptions(variables_map, "rendezvous", "proxy");
    ConflictingOptions(variables_map, "proxy", "bootstrap");
    ConflictingOptions(variables_map, "client", "proxy_bootstrap");
    
    if (variables_map["verbose"].as<bool>()) {
      FLAGS_ms_logging_common = google::INFO;
      FLAGS_ms_logging_transport = google::INFO;
    } else {
      FLAGS_ms_logging_common = google::FATAL;
      FLAGS_ms_logging_transport = google::FATAL;
    }
    
    FLAGS_log_prefix = variables_map["verbose"].as<bool>();
    FLAGS_ms_logging_user = google::INFO;
    FLAGS_logtostderr = true;
    if (variables_map.count("logfile")) {
      fs::path log_path;
      try {
        log_path = fs::path(variables_map["logfile"].as<std::string>());
        if (!fs::exists(log_path.parent_path()) &&
            !fs::create_directories(log_path.parent_path())) {
          ULOG(ERROR) << "Could not create directory for log file.";
          log_path = fs::temp_directory_path() / "nat_detection_demo.log";
        }
      }
      catch(const std::exception &e) {
        ULOG(ERROR) << "Error creating directory for log file: " << e.what();
        boost::system::error_code error_code;
        log_path =
            fs::temp_directory_path(error_code) / "nat_detection_demo.log";
      }

      ULOG(INFO) << "Log file at " << log_path;
      for (google::LogSeverity severity(google::WARNING);
           severity != google::NUM_SEVERITIES; ++severity) {
        google::SetLogDestination(severity, "");
      }
      google::SetLogDestination(google::INFO, log_path.string().c_str());
      FLAGS_alsologtostderr = true;
    }
    fs::path bootstrap_file_path(bootstrap_file),
        proxy_bootstrap_file_path(proxy_bootstrap_file);
    if (variables_map.count("bootstrap")) {
      bootstrap_file_path =
          fs::path(variables_map["bootstrap"].as<std::string>());
    }
    if (variables_map.count("proxy_bootstrap")) {
      proxy_bootstrap_file_path =
          fs::path(variables_map["proxy_bootstrap"].as<std::string>());
    }
    bool proxy(variables_map["proxy"].as<bool>());
    if (proxy) {
      node.SetUpProxy(proxy_bootstrap_file_path);
    }
    bool rendezvous(variables_map["rendezvous"].as<bool>());
    if (rendezvous) {
      node.SetUpRendezvous(proxy_bootstrap_file_path, bootstrap_file_path);
    }
    bool client(variables_map["client"].as<bool>());
    if (client) {
      node.SetUpClient(bootstrap_file_path);
      std::cout << "Nat type is: " << node.Detect() <<std::endl;
    }

    if (rendezvous || proxy) {
      ULOG(INFO) << "===============================";
      ULOG(INFO) << "     Press Ctrl+C to exit.";
      ULOG(INFO) << "===============================";
      signal(SIGINT, CtrlCHandler);
      while (!ctrlc_pressed)
        maidsafe::Sleep(boost::posix_time::seconds(2));
    }
}