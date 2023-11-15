#include "eventservice/event/messagehandler.h"
#include "eventservice/event/messagequeue.h"

namespace chml {

MessageHandler::~MessageHandler() {
  MessageQueueManager::Clear(this);
}

} // namespace chml
