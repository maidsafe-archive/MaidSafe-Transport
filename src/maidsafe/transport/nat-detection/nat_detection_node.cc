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

#include "maidsafe/transport/nat-detection/node.h"
#include "maidsafe/transport/nat-detection/nat_detection_node.h"

namespace maidsafe {

namespace transport {

namespace detection {
  
NatDetectionNode::NatDetectionNode() : node_(new Node),
                                       node_type_(kUnknown) {}

void NatDetectionNode::SetUpClient(const fs::path& bootstrap) {
  if (node_type_ != detection::kUnknown) {
    std::cout << "Node is already set up." << std::endl;
    return;
  }
  if (!node_->StartListening()) {
    std::cout << "Failed to start listening." << std::endl;
    return;
  }
  if (!node_->SetLiveContacts(bootstrap)) {
    std::cout << "Failed to retrieve contacts from bootstrap file." << std::endl;
    return;
  }
  node_type_ = detection::kClient;
}

int16_t NatDetectionNode::Detect() {
  if (node_type_ != detection::kClient) {
    std::cout << "Node is not set up as client" << std::endl;
    return detection::kUnknown;
  } else 
    return node_->DetectNatType();
}

void NatDetectionNode::SetUpProxy(const fs::path& bootstrap) {
  if (node_type_ != detection::kUnknown) {
    std::cout << "Node is already set up." << std::endl;
    return;
  }
//   if (!node_->IsDirectlyConnected()) {
//     std::cout << "The node is not publicly accessible." << std::endl;
//     return;
//   }
  if (!node_->StartListening()) {
    std::cout << "Failed to start listening." << std::endl;
    return;
  }
  if (!node_->WriteBootstrapFile(bootstrap)) {
    std::cout << "Failed to create bootstrap file." << std::endl;
    return;
  }
  node_type_ = detection::kProxy;
}

void NatDetectionNode::SetUpRendezvous(const fs::path& proxy_bootstrap,
                                       const fs::path& bootstrap) {
  if (node_type_ != detection::kUnknown) {
    std::cout << "Node is already set up." << std::endl;
    return;
  }
//   if (!node_->IsDirectlyConnected()) {
//     std::cout << "The node is not publicly accessible." << std::endl;
//     return;
//   }
  if (!node_->StartListening()) {
    std::cout << "Failed to start listening." << std::endl;
    return;
  }  
  if (!node_->SetLiveContacts(proxy_bootstrap)) {
    std::cout << "Failed to retrieve contacts from bootstrap file." <<
        std::endl;
    return;
  }
  if (!node_->WriteBootstrapFile(bootstrap)) {
    std::cout << "Failed to create bootstrap file." << std::endl;
    return;
  }
  node_type_ = detection::kRendezvous;
}

} // detection

} // transport

} // maidsafe
  