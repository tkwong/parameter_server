#include "server/util/pending_buffer.hpp"

namespace csci5570 {

std::vector<Message> PendingBuffer::Pop(const int clock) {
    std::vector<Message> reply;
  
    for (auto it=buffer.begin(); it!=buffer.end();)
    {
        if (it->first == clock)
        {
            reply.push_back(it->second);
            it = buffer.erase(it);
        }
        else
            it++;
    }
    
    return reply;
}

void PendingBuffer::Push(const int clock, Message& msg) {
    buffer.insert(std::make_pair(clock, msg));
}

int PendingBuffer::Size(const int progress) {
    return buffer.count(progress);
}

}  // namespace csci5570
