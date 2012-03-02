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
#include "maidsafe/transport/nat-detection/nat_detection.h"

namespace maidsafe {

namespace transport {

namespace detection {
  
NatDetection::NatDetection() : node(new Node) {}

void NatDetection::SetUpClient() {
}

void NatDetection::SetUpProxy() {
  if (!node_->IsDirectlyConnected()) {
    std::cout << "The node is not publicly accessible." << std::endl;
    return;
  }
  if (!node_->StartListening()) {
    std::cout << "Failed to start listening." << std::endl;
    return;
  }
  if (!node_->WriteBootstrapFile()) {
    std::cout << "Failed to create bootstrap file." << std::endl;    
  }
}

void NatDetection::SetUpRendezvous(const fs::path& bootstrap) {
  std::vector<Contact> contacts;
  if (!node_->IsDirectlyConnected()) {
    std::cout << "The node is not publicly accessible." << std::endl;
    return;
  }
  if (!node_->StartListening()) {
    std::cout << "Failed to start listening." << std::endl;
    return;
  }  
  if (node_->SetLiveContacts(bootstrap, &contacts)) {
    std::cout << "Failed to retrieve contacts from bootstrap file." << std::endl;
  }
  node_->set_live_contacts(contacts.at(0));
}

} // detection

} // transport

} // maidsafe
  