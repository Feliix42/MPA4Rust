#include "matching.hpp"


// TODO: make arguments const?
std::forward_list<std::pair<MessagingNode*, MessagingNode*>> analyzeNodes(std::forward_list<MessagingNode>& sends, std::forward_list<MessagingNode>& recvs) {
    std::forward_list<std::pair<MessagingNode*, MessagingNode*>> matched {};

    for (MessagingNode& send: sends)
        for (MessagingNode& recv: recvs) {
            // there are various options for matching here:
            //      - either the types have the same length and are of the same type, or
            //      - the two types are of different length and the program has to check whether
            //        the last len(n) characters of the longer string match the shorter one.
            //        This is due to namespacing and can be illustrated using the following example:
            //              sent: weatherstation::Weather
            //              recv:                 Weather
            //
            //        The types are the same, but the names are different due to namespacing.
            long compared = send.type.length() - recv.type.length();
            if (compared == 0) {
                if (send.type == recv.type) {
                    matched.push_front(std::make_pair(&send, &recv));
                }
            }
            else if (compared < 0) {
                // send is shorter than recv
                if (recv.type.substr(recv.type.length() - send.type.length()) == send.type) {
                    matched.push_front(std::make_pair(&send, &recv));
                }
            }
            else {
                // recv is shorter than send
                if (send.type.substr(send.type.length() - recv.type.length()) == recv.type) {
                    matched.push_front(std::make_pair(&send, &recv));
                }
            }
        }

    return matched;
}


// TODO: keep this?
std::forward_list<std::pair<MessagingNode*, MessagingNode*>> analyzeNodes(std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>>& data) {
    std::forward_list<MessagingNode> sends, recvs;
    std::tie(sends, recvs) = data;
    return analyzeNodes(sends, recvs);
}
