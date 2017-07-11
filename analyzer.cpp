#include "analyzer.hpp"


// TODO: make arguments const?
std::forward_list<std::pair<MessagingNode, MessagingNode>> analyzeNodes(std::forward_list<MessagingNode>& sends, std::forward_list<MessagingNode>& recvs) {
    std::forward_list<std::pair<MessagingNode, MessagingNode>> matched {};

    for (MessagingNode send: sends)
        for (MessagingNode recv: recvs) {
            long compared = send.type.length() - recv.type.length();
            if (compared == 0) {
                if (send.type == recv.type) {
                    std::cout << send.type << " matches " << recv.type << std::endl;
                    matched.push_front(std::make_pair(send, recv));
                }
            }
            else if (compared < 0) {
                // send is shorter than recv
                // unsigned long diff = recv.type.length() - send.type.length();
                // std::cout << diff << " Comparing " << recv.type.substr(diff) << " to " << send.type;
                if (recv.type.substr(recv.type.length() - send.type.length()) == send.type) {
                    std::cout << send.type << " matches " << recv.type << std::endl;
                    matched.push_front(std::make_pair(send, recv));
                }
                // std::cout << std::endl;
            }
            else {
                // recv is shorter than send
                // std::cout << "2 Comparing " << send.type.substr(send.type.length() - recv.type.length()) << " to " << recv.type;
                if (send.type.substr(send.type.length() - recv.type.length()) == recv.type) {
                    std::cout << send.type << " matches " << recv.type << std::endl;
                    matched.push_front(std::make_pair(send, recv));
                }
                // std::cout << std::endl;
            }
        }

    return matched;
}


// TODO: keep this?
std::forward_list<std::pair<MessagingNode, MessagingNode>> analyzeNodes(std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>>& data) {
    std::forward_list<MessagingNode> sends, recvs;
    std::tie(sends, recvs) = data;
    return analyzeNodes(sends, recvs);
}
